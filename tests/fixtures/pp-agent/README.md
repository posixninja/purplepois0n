# pp-agent — reference TCP line protocol server

Minimal reference for the purplepois0n ramdisk agent contract (`RamdiskClient` default transport).

## Protocol (one TCP connection per request)

| Request | Response |
|---------|----------|
| `PING` | `PONG` |
| `EXEC <shell command>` | `OK <exitcode>` then output lines then `.` on its own line |
| `PUT <path> <size>` + raw bytes | `OK` |
| `GET <path>` | `DATA <size>` + bytes, or `ERR <message>` |

Default port: **4444** (forward with `iproxy -u UDID 4444:4444`).

## Build for iOS arm64

Cross-compile on Mac (example with Xcode SDK):

```bash
xcrun -sdk iphoneos clang -arch arm64 -o pp-agent pp-agent.c
purplepois0n --build-ramdisk /tmp/rdsk.dmg \
  --ramdisk-add-macho ./pp-agent:/sbin/pp-agent \
  --ramdisk-overlay ../ramdisk_overlay
```

Stage `sbin/start-pp-agent` from the overlay to launch `/sbin/pp-agent` from your init path.

## Host probe

```bash
purplepois0n -d UDID --ramdisk-probe
purplepois0n -d UDID --ramdisk-exec "uname -a"
```

purplepois0n does not bundle iOS-built agents — supply your own binary or adapt `pp-agent.c`.
