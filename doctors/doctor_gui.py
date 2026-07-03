#!/usr/bin/env python3
"""
PurpleDoctor — one-button doctors GUI for macOS, Linux, and Windows.

Spawns purplepois0n --doctor-run and renders JSON request/response steps.
Set PURPLEPOIS0N_BIN to override the CLI path.
Set PURPLEPOIS0N_DOCTOR_EXECUTE=1 to pass --jailbreak-execute (requires make plugins).
"""

from __future__ import annotations

import json
import os
import queue
import subprocess
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, ttk

REPO_ROOT = Path(__file__).resolve().parent.parent

DFU_STEPS = [
    "Get ready for DFU",
    "Press and hold sleep (2 sec)",
    "Hold sleep + press and hold home (10 sec)",
    "Release sleep; keep holding home (15 sec)",
]


def find_purplepois0n() -> str:
    override = os.environ.get("PURPLEPOIS0N_BIN")
    if override and Path(override).is_file():
        return override
    candidates = [
        REPO_ROOT / "build/bin/purplepois0n",
        REPO_ROOT / "build/bin/purplepois0n.exe",
        Path("/usr/local/bin/purplepois0n"),
    ]
    for path in candidates:
        if path.is_file():
            return str(path)
    return str(candidates[0])


class PurpleDoctorApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("PurpleDoctor")
        self.geometry("520x520")
        self.minsize(420, 420)
        self.resizable(True, True)

        self._proc: subprocess.Popen[str] | None = None
        self._queue: queue.Queue[tuple[str, object]] = queue.Queue()
        self._running = False
        self._complete = False
        self._progress = 0

        self._build_ui()
        self.after(100, self._poll_queue)

    def _build_ui(self) -> None:
        pad = {"padx": 12, "pady": 6}

        header = ttk.Label(self, text="purplepois0n doctor", font=("Helvetica", 18, "bold"))
        header.pack(**pad)

        self.subtitle = ttk.Label(self, text="Connect your device, then press Jailbreak")
        self.subtitle.pack(**pad)

        self.jailbreak_btn = ttk.Button(self, text="Jailbreak", command=self._on_jailbreak)
        self.jailbreak_btn.pack(pady=12)

        self.progress = ttk.Progressbar(self, mode="determinate", maximum=100)
        self.progress.pack(fill="x", **pad)

        dfu_frame = ttk.LabelFrame(self, text="DFU timing (if needed)")
        dfu_frame.pack(fill="x", **pad)
        self.dfu_label = ttk.Label(dfu_frame, text=DFU_STEPS[0], wraplength=460)
        self.dfu_label.pack(**pad)

        log_frame = ttk.LabelFrame(self, text="Steps")
        log_frame.pack(fill="both", expand=True, **pad)
        self.log = tk.Text(log_frame, height=14, wrap="word", state="disabled")
        scroll = ttk.Scrollbar(log_frame, command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        self.log.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")

    def _append_log(self, line: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", line + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _set_progress(self, value: int) -> None:
        self._progress = max(0, min(100, value))
        self.progress["value"] = self._progress

    def _on_jailbreak(self) -> None:
        if self._complete:
            self.destroy()
            return
        if self._running:
            return

        bin_path = find_purplepois0n()
        if not Path(bin_path).is_file():
            messagebox.showerror(
                "purplepois0n not found",
                f"Build the CLI first:\n  make release\n\nExpected:\n  {bin_path}",
            )
            return

        cmd = [bin_path, "--doctor-run"]
        if os.environ.get("PURPLEPOIS0N_DOCTOR_EXECUTE") == "1":
            cmd.extend(["--jailbreak-execute", "--i-understand-jailbreak"])

        self._running = True
        self._complete = False
        self.jailbreak_btn.configure(state="disabled", text="Running…")
        self.subtitle.configure(text="Doctor flow in progress")
        self._set_progress(5)
        self._append_log("$ " + " ".join(cmd))

        thread = threading.Thread(target=self._run_subprocess, args=(cmd,), daemon=True)
        thread.start()

    def _run_subprocess(self, cmd: list[str]) -> None:
        try:
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
            )
            self._proc = proc
            assert proc.stdout is not None
            for raw in proc.stdout:
                line = raw.rstrip("\n")
                if line.startswith("{") and line.endswith("}"):
                    try:
                        self._queue.put(("json", json.loads(line)))
                        continue
                    except json.JSONDecodeError:
                        pass
                self._queue.put(("line", line))
            code = proc.wait()
            self._queue.put(("exit", code))
        except OSError as exc:
            self._queue.put(("error", str(exc)))

    def _poll_queue(self) -> None:
        while True:
            try:
                kind, payload = self._queue.get_nowait()
            except queue.Empty:
                break
            if kind == "json":
                self._handle_event(payload)
            elif kind == "line":
                self._append_log(payload)
            elif kind == "error":
                messagebox.showerror("Doctor failed", str(payload))
                self._finish(False, "Launch error")
            elif kind == "exit":
                self._finish(payload == 0, "Exit code " + str(payload))

        self.after(100, self._poll_queue)

    def _handle_event(self, event: dict) -> None:
        etype = event.get("type")
        phase = event.get("phase", "")
        detail = event.get("detail", "")

        if etype == "step":
            step_id = event.get("id", "?")
            arrow = "→" if phase == "request" else "←"
            ok = event.get("success")
            suffix = ""
            if phase == "response" and ok is not None:
                suffix = " OK" if ok else " FAIL"
            self._append_log(f"  {arrow} {step_id}{suffix}: {detail}")
            if phase == "response":
                bump = {"detect": 15, "syringe-connect": 30, "identify": 45, "jailbreak": 80}.get(
                    step_id, 0
                )
                if bump:
                    self._set_progress(bump)
        elif etype == "syringe":
            arrow = "TX" if phase == "request" else "RX"
            cmd = event.get("command", detail)
            self._append_log(f"    [{arrow}] {cmd or detail}")
        elif etype == "complete":
            success = bool(event.get("success"))
            self._finish(success, detail or ("Complete" if success else "Failed"))

    def _finish(self, success: bool, detail: str) -> None:
        if self._complete:
            return
        self._complete = True
        self._running = False
        self._proc = None
        self._set_progress(100)
        if success:
            self.jailbreak_btn.configure(state="normal", text="Done — Quit")
            self.subtitle.configure(text=detail or "Jailbreak complete")
        else:
            self.jailbreak_btn.configure(state="normal", text="Quit (Retry)")
            self.subtitle.configure(text=detail or "Jailbreak failed")


def main() -> int:
    app = PurpleDoctorApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
