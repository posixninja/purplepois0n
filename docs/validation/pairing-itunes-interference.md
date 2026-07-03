# Pairing and iTunes / Finder interference

Host-side notes for absinthe-era and Gen 0–1 workflows where **Apple Mobile Device Service** or **Finder/iTunes** holds an exclusive lockdown session.

## Symptoms

| Symptom | Likely cause |
|---------|----------------|
| `idevice connect failed` in Normal mode | Finder/iTunes has active pairing session |
| `mobilebackup2_client_start_service failed` | Backup/sync daemon owns the device |
| Recovery/DFU works but Normal probes fail | USB mode OK; lockdown contended |
| Intermittent `-d UDID` failures after plug-in | Auto-sync starting in background |

## Mitigations (host)

1. **Quit Finder/iTunes** before running Normal-mode probes (`--gen0`, `--afc-list`, `--install-ipa`, mobilebackup2 probe).
2. **Disable automatic sync** for the device in Finder (device → General → uncheck “Show this iPhone when on Wi-Fi” / sync options as applicable).
3. **Trust the host** once; unlock the device screen for first pairing.
4. **Use Recovery or DFU** for boot-chain work when Normal lockdown is blocked — purplepois0n Recovery/DFU paths do not require iTunes.

## Optional out-of-tree helper

Historical absinthe bundles shipped an **iTunesKiller**-style helper (see `legacy/Chronic-Dev/absinthe-2.0` mirrors). purplepois0n does **not** ship or invoke process killers.

If you maintain a local helper script:

```bash
# Example only — not shipped; review before use on your OS
osascript -e 'tell application "Finder" to quit' 2>/dev/null || true
killall AMPDevicesAgent 2>/dev/null || true
```

Prefer orderly quit of Finder/iTunes over forced kills when possible.

## Validation checklist

| Step | Command | Expected | Status |
|------|---------|----------|--------|
| 1 | Quit Finder/iTunes | No AMPDevicesAgent lock | Manual |
| 2 | `./build/bin/purplepois0n -l` | Device listed in Normal | **Not run** |
| 3 | `./build/bin/purplepois0n --gen0 -d UDID --report /tmp/pp.json` | Normal probe + MB2 connect | **Not run** |
| 4 | Re-run with Finder open | Document failure mode for your OS | **Not run** |

## Related docs

- [SUPPORT.md](../SUPPORT.md) — capability matrix
- [normal-mode-afc-backup.md](../book/deep/normal-mode-afc-backup.md) — trust boundary
- [INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) Phase **7.8**
