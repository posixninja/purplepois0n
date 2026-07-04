"""purplepois0n Rich TUI."""

from __future__ import annotations

import sys

from prompt_toolkit.shortcuts import radiolist_dialog
from rich.console import Console
from rich.panel import Panel
from rich.table import Table

from . import api_client

console = Console()
STORE_APP = "purplepois0n-zebra"


def _format_event(ev: dict) -> str | None:
    t = ev.get("type")
    if t == "step":
        arrow = "→" if ev.get("phase") == "request" else "←"
        ok = ""
        if ev.get("phase") == "response" and ev.get("success") is not None:
            ok = " OK" if ev.get("success") else " FAIL"
        return f"  {arrow} {ev.get('id', '?')}{ok}: {ev.get('detail', '')}"
    if t == "log":
        return ev.get("detail")
    if t == "complete":
        return "Done." if ev.get("success") else f"Failed: {ev.get('detail', '')}"
    return None


def show_home() -> None:
    try:
        h = api_client.health()
    except Exception as e:
        console.print(Panel(f"[red]Agent offline[/] — run: make agent\n{e}", title="purplepois0n"))
        return

    table = Table(title="Host")
    table.add_column("Key")
    table.add_column("Value")
    table.add_row("Agent", "ok" if h.get("ok") else "missing")
    table.add_row("Binary", str(h.get("bin", "")))
    table.add_row("Plugins", "yes" if h.get("plugins") else "no (make plugins)")
    table.add_row("Store", str(h.get("storeRoot", "")))
    console.print(table)

    devices = api_client.list_devices()
    if devices:
        dt = Table(title="Devices")
        dt.add_column("UDID")
        dt.add_column("State")
        for d in devices:
            dt.add_row(d.get("udid", ""), d.get("state", ""))
        console.print(dt)
    else:
        console.print("[yellow]No devices — plug in, unlock, trust host[/]")


def pick_udid() -> str | None:
    devices = api_client.list_devices()
    if not devices:
        console.print("[yellow]No devices found[/]")
        return None
    if len(devices) == 1:
        return devices[0]["udid"]
    choices = [(d["udid"], f"{d['udid'][:12]}… ({d.get('state', '?')})") for d in devices]
    return radiolist_dialog(title="Select device", values=choices).run()


def run_jailbreak() -> None:
    udid = pick_udid()
    if not udid:
        return
    console.print(f"[cyan]Jailbreak[/] {udid}")

    def on_ev(ev: dict) -> None:
        line = _format_event(ev)
        if line:
            console.print(line)

    ok = api_client.stream_ndjson("/jailbreak", {"udid": udid, "mode": ""}, on_ev)
    console.print("[green]Success[/]" if ok else "[red]Failed[/]")


def run_store() -> None:
    try:
        text = api_client.store_packages()
        pkgs = api_client.parse_packages(text)
    except Exception as e:
        console.print(f"[red]Catalog error:[/] {e}")
        return

    if not pkgs:
        console.print("[yellow]Empty catalog — run legacy/scripts/seed-store.sh[/]")
        return

    table = Table(title="Packages")
    table.add_column("#")
    table.add_column("Package")
    table.add_column("Version")
    for i, p in enumerate(pkgs, 1):
        table.add_row(str(i), p.get("Package", ""), p.get("Version", ""))
    console.print(table)

    udid = pick_udid()
    if not udid:
        return

    try:
        console.print("[cyan]Syncing store…[/]")
        console.print(api_client.store_sync(udid))
        target = STORE_APP if any(p.get("Package") == STORE_APP for p in pkgs) else pkgs[0].get("Package")
        if target:
            console.print(f"[cyan]Installing {target}…[/]")
            console.print(api_client.store_install(udid, target))
    except Exception as e:
        console.print(f"[red]{e}[/]")


def main_menu() -> None:
    while True:
        choice = radiolist_dialog(
            title="purplepois0n",
            text="Host terminal UI (requires make agent)",
            values=[
                ("home", "Home — status & devices"),
                ("jailbreak", "Jailbreak"),
                ("store", "Store — sync & install"),
                ("quit", "Quit"),
            ],
        ).run()
        if choice is None or choice == "quit":
            break
        if choice == "home":
            show_home()
        elif choice == "jailbreak":
            run_jailbreak()
        elif choice == "store":
            run_store()


def main() -> int:
    try:
        main_menu()
    except KeyboardInterrupt:
        console.print("\nBye.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
