"""HTTP client for purplepois0n agent (:7749)."""

from __future__ import annotations

import json
import os
from typing import Any, Callable, Iterator

import httpx

AGENT_BASE = os.environ.get("PURPLEPOIS0N_AGENT_URL", "http://127.0.0.1:7749")


def health() -> dict[str, Any]:
    r = httpx.get(f"{AGENT_BASE}/health", timeout=3.0)
    r.raise_for_status()
    return r.json()


def list_devices() -> list[dict[str, Any]]:
    r = httpx.get(f"{AGENT_BASE}/devices", timeout=10.0)
    r.raise_for_status()
    return r.json().get("devices", [])


def stream_ndjson(path: str, body: dict, on_line: Callable[[dict], None]) -> bool:
    success = False
    with httpx.Client(timeout=None) as client:
        with client.stream("POST", f"{AGENT_BASE}{path}", json=body) as resp:
            resp.raise_for_status()
            buf = ""
            for chunk in resp.iter_text():
                buf += chunk
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    line = line.strip()
                    if not line.startswith("{"):
                        continue
                    ev = json.loads(line)
                    on_line(ev)
                    if ev.get("type") == "complete":
                        success = bool(ev.get("success"))
    return success


def store_packages(store_root: str = "") -> str:
    q = f"?storeRoot={store_root}" if store_root else ""
    r = httpx.get(f"{AGENT_BASE}/store/packages{q}", timeout=30.0)
    r.raise_for_status()
    return r.text


def store_sync(udid: str, store_root: str = "") -> str:
    r = httpx.post(
        f"{AGENT_BASE}/store/sync",
        json={"udid": udid, "storeRoot": store_root},
        timeout=300.0,
    )
    data = r.json()
    if not r.is_success or not data.get("ok"):
        raise RuntimeError(data.get("error") or data.get("detail") or r.text)
    return data.get("detail", "synced")


def store_install(udid: str, package: str, store_root: str = "") -> str:
    r = httpx.post(
        f"{AGENT_BASE}/store/install",
        json={"udid": udid, "package": package, "storeRoot": store_root},
        timeout=600.0,
    )
    data = r.json()
    if not r.is_success or not data.get("ok"):
        raise RuntimeError(data.get("error") or data.get("detail") or r.text)
    return data.get("detail", f"installed {package}")


def parse_packages(text: str) -> list[dict[str, str]]:
    pkgs: list[dict[str, str]] = []
    block: dict[str, str] = {}
    for line in text.splitlines():
        if not line.strip():
            if block.get("Package"):
                pkgs.append(block)
            block = {}
            continue
        if ":" in line:
            k, v = line.split(":", 1)
            block[k.strip()] = v.strip()
    if block.get("Package"):
        pkgs.append(block)
    return pkgs
