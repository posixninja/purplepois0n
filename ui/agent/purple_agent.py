#!/usr/bin/env python3
"""
purplepois0n localhost agent — bridges the web UI to the native CLI.

  PURPLEPOIS0N_BIN=./build/bin/purplepois0n python3 ui/agent/purple_agent.py

Endpoints:
  GET  /health
  GET  /devices
  GET  /device/plan?udid=       JSON profile + jailbreak strategy (AI/planner input)
  GET  /store/packages?storeRoot=
  GET  /store/installed?udid=
  POST /doctor              {"execute": false, "udid": ""}  -> NDJSON stream (probe; no jailbreak step)
  POST /jailbreak             {"auto": true, "execute": true, "udid": ""} -> NDJSON stream (doctor auto-plan)
  POST /checkm8               {"udid": ""}                    -> NDJSON stream
  POST /dfu-jailbreak         {"udid": ""}                    -> NDJSON stream
  POST /external-jailbreak    {"udid": "", "already_jailbroken": false} -> NDJSON
  POST /store/sync            {"udid": "", "storeRoot": "", "syncMode": "file"}
  POST /store/install         {"udid": "", "package": "", "storeRoot": ""}
  POST /store/publish         {"storeRoot": "", "publishRoot": ""}
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Iterator
from urllib.parse import parse_qs, urlparse

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BIN = REPO_ROOT / "build" / "bin" / "purplepois0n"
DEFAULT_STORE = REPO_ROOT / "store"
PORT = int(os.environ.get("PURPLEPOIS0N_AGENT_PORT", "7749"))
DFU_STATES = frozenset({"dfu", "recovery", "unknown"})


def resolve_bin() -> Path:
    override = os.environ.get("PURPLEPOIS0N_BIN")
    if override:
        return Path(override)
    return DEFAULT_BIN


def resolve_store_root(override: str | None) -> Path:
    if override:
        return Path(override)
    env = os.environ.get("PURPLEPOIS0N_STORE_ROOT")
    if env:
        return Path(env)
    return DEFAULT_STORE


def ssh_env(base: dict[str, str] | None = None) -> dict[str, str]:
    env = dict(base or os.environ)
    env["PURPLEPOIS0N_RAMDISK_AUTO_IPROXY"] = "1"
    env["PURPLEPOIS0N_NORMAL_SSH"] = "1"
    return env


def agent_env(base: dict[str, str] | None = None) -> dict[str, str]:
    """Default env for device plan, doctor, and jailbreak streams."""
    return ssh_env(base)


def udid_from_body(body: dict) -> str | None:
    udid = (body.get("udid") or "").strip()
    if udid:
        return udid
    env = os.environ.get("PURPLEPOIS0N_DEVICE_UDID", "").strip()
    return env or None


def cors_headers(handler: BaseHTTPRequestHandler) -> None:
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
    handler.send_header("Access-Control-Allow-Headers", "Content-Type")


def iter_ndjson_lines(proc: subprocess.Popen[str]) -> Iterator[str]:
    assert proc.stdout is not None
    for line in proc.stdout:
        stripped = line.rstrip("\n")
        if stripped.startswith("{") and stripped.endswith("}"):
            yield stripped + "\n"
        elif stripped:
            yield json.dumps({"type": "log", "detail": stripped}) + "\n"


def parse_list_output(out: str) -> list[dict[str, Any]]:
    devices: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None
    for raw in out.splitlines():
        line = raw.strip()
        if not line or line.startswith("["):
            continue
        if re.match(r"Device \d+:", line):
            if current and current.get("udid"):
                devices.append(current)
            current = {}
            continue
        if current is None:
            continue
        if line.startswith("UDID:"):
            udid = line.split(":", 1)[1].strip()
            if udid and udid != "N/A":
                current["udid"] = udid
        elif line.startswith("ECID:"):
            current["ecid"] = line.split(":", 1)[1].strip()
        elif line.startswith("State:"):
            current["state"] = line.split(":", 1)[1].strip().lower()
        elif line.startswith("Type:"):
            device_type = line.split(":", 1)[1].strip()
            if device_type != "N/A":
                current["deviceType"] = device_type
        elif line.startswith("Firmware:"):
            firmware = line.split(":", 1)[1].strip()
            if firmware != "N/A":
                current["firmware"] = firmware
        elif line.startswith("CPID:"):
            current["cpid"] = line.split(":", 1)[1].strip()
    if current and current.get("udid"):
        devices.append(current)
    return devices


def list_devices() -> list[dict[str, Any]]:
    bin_path = resolve_bin()
    if not bin_path.is_file():
        return []
    try:
        out = subprocess.check_output(
            [str(bin_path), "--list"],
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            timeout=30,
        )
    except subprocess.CalledProcessError as e:
        out = e.output or ""
    return parse_list_output(out)


def cli_device_plan(bin_path: Path, udid: str | None = None) -> dict[str, Any]:
    cmd = build_cli_cmd(["--device-plan"], udid)
    try:
        out = subprocess.check_output(
            cmd,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            env=agent_env(),
            timeout=60,
        )
        return json.loads(out.strip())
    except subprocess.CalledProcessError as e:
        return {"error": e.output or str(e)}
    except json.JSONDecodeError as e:
        return {"error": f"invalid plan json: {e}"}


def cli_capabilities(bin_path: Path) -> dict[str, Any]:
    try:
        out = subprocess.check_output(
            [str(bin_path), "--capabilities"],
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            timeout=10,
        )
        return json.loads(out.strip())
    except Exception:
        return {"plugins": False, "doctor": True, "store": True, "normalSsh": True}


def build_cli_cmd(args: list[str], udid: str | None = None) -> list[str]:
    bin_path = resolve_bin()
    cmd = [str(bin_path)]
    if udid:
        cmd.extend(["-d", udid])
    cmd.extend(args)
    return cmd


def pick_jailbreak_args(body: dict, devices: list[dict[str, Any]]) -> list[str]:
    mode = (body.get("mode") or "").strip().lower()
    udid = udid_from_body(body)
    if not mode and udid:
        for device in devices:
            if device.get("udid") == udid:
                mode = (device.get("state") or "").lower()
                break
    if body.get("auto") or body.get("use_plan"):
        args = ["--doctor-run", "--normal-ssh"]
        if body.get("execute") or body.get("i_understand_jailbreak"):
            args.extend(["--jailbreak-execute", "--i-understand-jailbreak"])
        if body.get("post_jb_store"):
            args.append("--post-jb-store")
        return args
    if body.get("already_jailbroken"):
        args = ["--external-jailbreak", "--already-jailbroken", "--normal-ssh"]
        if body.get("post_jb_store"):
            args.append("--post-jb-store")
        return args
    if body.get("experimental_dfu") and mode in DFU_STATES:
        return ["--dfu-jailbreak", "--i-understand-jailbreak"]
    args = ["--external-jailbreak", "--i-understand-jailbreak", "--normal-ssh"]
    if body.get("post_jb_store"):
        args.append("--post-jb-store")
    return args


class AgentHandler(BaseHTTPRequestHandler):
    server_version = "purplepois0n-agent/0.3"

    def log_message(self, fmt: str, *args: Any) -> None:
        sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        cors_headers(self)
        self.end_headers()

    def _json(self, code: int, payload: dict) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(code)
        cors_headers(self)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _text(self, code: int, body: str, content_type: str = "text/plain") -> None:
        data = body.encode("utf-8")
        self.send_response(code)
        cors_headers(self)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _read_json_body(self) -> dict | None:
        length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw.decode("utf-8") or "{}")
        except json.JSONDecodeError:
            return None

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/health":
            bin_path = resolve_bin()
            caps = cli_capabilities(bin_path) if bin_path.is_file() else {}
            self._json(
                200,
                {
                    "ok": bin_path.is_file(),
                    "bin": str(bin_path),
                    "version": "0.3.0",
                    "storeRoot": str(resolve_store_root(None)),
                    "plugins": bool(caps.get("plugins")),
                    "capabilities": caps,
                },
            )
            return

        if path == "/devices":
            self._json(200, {"devices": list_devices()})
            return

        if path == "/device/plan":
            qs = parse_qs(parsed.query)
            udid = (qs.get("udid") or [""])[0].strip() or os.environ.get("PURPLEPOIS0N_DEVICE_UDID", "").strip()
            bin_path = resolve_bin()
            if not bin_path.is_file():
                self._json(500, {"error": f"binary not found: {bin_path}"})
                return
            plan = cli_device_plan(bin_path, udid or None)
            code = 200 if "error" not in plan else 404
            self._json(code, plan)
            return

        if path == "/store/packages":
            qs = parse_qs(parsed.query)
            store_root = (qs.get("storeRoot") or [""])[0]
            ok, content = store_packages(store_root)
            if not ok:
                self._json(404, {"error": content})
                return
            self._text(200, content, "text/plain")
            return

        if path == "/store/installed":
            qs = parse_qs(parsed.query)
            udid = (qs.get("udid") or [""])[0].strip() or os.environ.get("PURPLEPOIS0N_DEVICE_UDID", "").strip()
            ok, packages = store_installed(udid or None)
            if not ok:
                self._json(500, {"error": packages if isinstance(packages, str) else "probe failed"})
                return
            self._json(200, {"packages": packages})
            return

        self._json(404, {"error": "not found"})

    def do_POST(self) -> None:
        body = self._read_json_body()
        if body is None:
            self._json(400, {"error": "invalid json"})
            return

        if self.path == "/doctor":
            self._stream_doctor(bool(body.get("execute")), udid_from_body(body))
            return
        if self.path == "/jailbreak":
            self._stream_jailbreak(body)
            return
        if self.path == "/checkm8":
            self._stream_cli(["--checkm8"], udid_from_body(body))
            return
        if self.path == "/dfu-jailbreak":
            self._stream_cli(
                ["--dfu-jailbreak", "--i-understand-jailbreak"],
                udid_from_body(body),
            )
            return
        if self.path == "/external-jailbreak":
            args = ["--external-jailbreak", "--i-understand-jailbreak", "--normal-ssh"]
            if body.get("already_jailbroken"):
                args = ["--external-jailbreak", "--already-jailbroken", "--normal-ssh"]
            if body.get("post_jb_store"):
                args.append("--post-jb-store")
            install_pkg = (body.get("post_jb_store_install") or "").strip()
            if install_pkg:
                args.extend(["--post-jb-store-install", install_pkg])
            self._stream_cli(args, udid_from_body(body))
            return
        if self.path == "/store/sync":
            udid = udid_from_body(body)
            store_root = body.get("storeRoot") or ""
            sync_mode = (body.get("syncMode") or "file").strip() or "file"
            ok, msg = store_sync(udid, store_root, sync_mode)
            self._json(200 if ok else 500, {"ok": ok, "detail": msg} if ok else {"error": msg})
            return
        if self.path == "/store/install":
            udid = udid_from_body(body)
            package = (body.get("package") or "").strip()
            store_root = body.get("storeRoot") or ""
            if not package:
                self._json(400, {"error": "package required"})
                return
            ok, msg = store_install(udid, package, store_root)
            self._json(200 if ok else 500, {"ok": ok, "detail": msg} if ok else {"error": msg})
            return
        if self.path == "/store/publish":
            store_root = body.get("storeRoot") or ""
            publish_root = body.get("publishRoot") or ""
            ok, msg = store_publish(store_root, publish_root)
            self._json(200 if ok else 500, {"publishRoot": msg} if ok else {"error": msg})
            return
        self._json(404, {"error": "not found"})

    def _stream_cli(self, args: list[str], udid: str | None = None) -> None:
        bin_path = resolve_bin()
        if not bin_path.is_file():
            self._json(500, {"error": f"binary not found: {bin_path}"})
            return
        self._stream_subprocess(build_cli_cmd(args, udid), env=agent_env())

    def _stream_doctor(self, execute: bool, udid: str | None) -> None:
        cmd = ["--doctor-run", "--normal-ssh"]
        if execute:
            cmd.extend(["--jailbreak-execute", "--i-understand-jailbreak"])
        self._stream_cli(cmd, udid)

    def _stream_jailbreak(self, body: dict) -> None:
        devices = list_devices()
        args = pick_jailbreak_args(body, devices)
        self._stream_cli(args, udid_from_body(body))

    def _stream_subprocess(self, cmd: list[str], env: dict[str, str] | None = None) -> None:
        self.send_response(200)
        cors_headers(self)
        self.send_header("Content-Type", "application/x-ndjson")
        self.send_header("Transfer-Encoding", "chunked")
        self.end_headers()
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            env=env or agent_env(),
        )
        saw_complete = False
        for line in iter_ndjson_lines(proc):
            chunk = line.encode("utf-8")
            self.wfile.write(f"{len(chunk):x}\r\n".encode())
            self.wfile.write(chunk)
            self.wfile.write(b"\r\n")
            self.wfile.flush()
            stripped = line.strip()
            if stripped.startswith("{"):
                try:
                    ev = json.loads(stripped)
                    if ev.get("type") == "complete":
                        saw_complete = True
                except json.JSONDecodeError:
                    pass
        proc.wait()
        if not saw_complete:
            trailer = json.dumps({"type": "complete", "success": proc.returncode == 0}) + "\n"
            data = trailer.encode("utf-8")
            self.wfile.write(f"{len(data):x}\r\n".encode())
            self.wfile.write(data)
            self.wfile.write(b"\r\n")
        self.wfile.write(b"0\r\n\r\n")
        self.wfile.flush()


def packages_path(store_root: Path) -> Path:
    return (
        store_root
        / "dists"
        / "purplepois0n"
        / "main"
        / "binary-iphoneos-arm64"
        / "Packages"
    )


def store_packages(store_root: str) -> tuple[bool, str]:
    root = resolve_store_root(store_root or None)
    pkg = packages_path(root)
    if not pkg.is_file():
        bin_path = resolve_bin()
        if not bin_path.is_file():
            return False, f"Packages missing at {pkg} and binary not found"
        cmd = [str(bin_path), "--store-root", str(root), "--store-build"]
        try:
            subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True, cwd=str(REPO_ROOT))
        except subprocess.CalledProcessError as e:
            return False, e.output or str(e)
    if not pkg.is_file():
        return False, f"Packages not found at {pkg}"
    return True, pkg.read_text(encoding="utf-8", errors="replace")


def store_sync(udid: str | None, store_root: str, sync_mode: str = "file") -> tuple[bool, str]:
    bin_path = resolve_bin()
    if not bin_path.is_file():
        return False, f"binary not found: {bin_path}"
    if not udid:
        return False, "device UDID required — plug in, unlock, and trust this computer"
    root = store_root or str(resolve_store_root(None))
    mode = sync_mode or "file"
    cmd = build_cli_cmd(
        ["--store-root", root, "--store-sync", "--store-sync-mode", mode, "--normal-ssh"],
        udid,
    )
    try:
        out = subprocess.check_output(
            cmd,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            env=ssh_env(),
            timeout=300,
        )
        return True, out.strip() or "store synced"
    except subprocess.CalledProcessError as e:
        return False, e.output or str(e)


def store_installed(udid: str | None) -> tuple[bool, list[str] | str]:
    bin_path = resolve_bin()
    if not bin_path.is_file():
        return False, f"binary not found: {bin_path}"
    if not udid:
        return False, "device UDID required — plug in, unlock, and trust this computer"
    cmd = build_cli_cmd(["--store-installed", "--normal-ssh"], udid)
    try:
        out = subprocess.check_output(
            cmd,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            env=ssh_env(),
            timeout=120,
        )
        packages = [line.strip() for line in out.splitlines() if line.strip()]
        return True, packages
    except subprocess.CalledProcessError as e:
        return False, e.output or str(e)


def store_install(udid: str | None, package: str, store_root: str) -> tuple[bool, str]:
    bin_path = resolve_bin()
    if not bin_path.is_file():
        return False, f"binary not found: {bin_path}"
    if not udid:
        return False, "device UDID required — plug in, unlock, and trust this computer"
    root = store_root or str(resolve_store_root(None))
    cmd = build_cli_cmd(
        ["--store-root", root, "--store-sync", "--store-install", package, "--normal-ssh"],
        udid,
    )
    try:
        out = subprocess.check_output(
            cmd,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=str(REPO_ROOT),
            env=ssh_env(),
            timeout=600,
        )
        return True, out.strip() or f"installed {package}"
    except subprocess.CalledProcessError as e:
        return False, e.output or str(e)


def store_publish(store_root: str, publish_root: str) -> tuple[bool, str]:
    bin_path = resolve_bin()
    if not bin_path.is_file():
        return False, f"binary not found: {bin_path}"
    cmd = [str(bin_path)]
    root = store_root or str(resolve_store_root(None))
    cmd.extend(["--store-root", root, "--store-publish"])
    if publish_root:
        cmd.append(publish_root)
    try:
        out = subprocess.check_output(cmd, stderr=subprocess.STDOUT, text=True, cwd=str(REPO_ROOT))
    except subprocess.CalledProcessError as e:
        return False, e.output or str(e)
    for line in out.splitlines():
        if "publish ready at" in line.lower() or "published repo to" in line.lower():
            parts = line.split()
            return True, parts[-1]
    if publish_root:
        return True, publish_root
    return True, str(REPO_ROOT / "store-publish")


def main() -> int:
    bin_path = resolve_bin()
    print(f"purplepois0n agent on http://127.0.0.1:{PORT}", file=sys.stderr)
    print(f"  binary: {bin_path} ({'ok' if bin_path.is_file() else 'MISSING'})", file=sys.stderr)
    print(f"  store:  {resolve_store_root(None)}", file=sys.stderr)
    server = ThreadingHTTPServer(("127.0.0.1", PORT), AgentHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
