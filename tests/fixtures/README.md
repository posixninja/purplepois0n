# Backup test fixtures

Synthetic, redacted samples for offline `MobileBackup` smoke checks. **Not** real device backups.

Run all fixture smoke checks:

```bash
make test-fixtures
# or: tests/run_fixtures.sh
```

## mbdb_minimal

iOS 5–9 style tree with `Manifest.mbdb` + `Info.plist`.

```bash
make release
./build/bin/purplepois0n --analyze-backup tests/fixtures/mbdb_minimal
```

Expected: `Manifest: Manifest.mbdb`, one `HomeDomain` entry, UDID `TEST-UDID-MBDB`.

## mbdb_v1_flat

iOS 5–9 **protocol v1** sample: `Manifest.mbdb` magic byte **1**, `Status.plist` Version **2.4**, flat hash storage (no `ab/` shard directory).

```bash
./build/bin/purplepois0n --analyze-backup tests/fixtures/mbdb_v1_flat
```

Expected: `Index: v1`, `Storage: v1 (flat hash paths)`.

## manifest_db_minimal

iOS 10+ **protocol v2** sample: `Manifest.db` + `Status.plist` Version **3.2** (sharded hash paths).

```bash
./build/bin/purplepois0n --analyze-backup tests/fixtures/manifest_db_minimal
```

Expected: `Manifest: Manifest.db`, one `HomeDomain` entry, UDID `TEST-UDID-SQLITE`.

Regenerate SQLite fixture:

```bash
python3 - <<'PY'
import os, sqlite3, plistlib
root = "tests/fixtures/manifest_db_minimal"
os.makedirs(root, exist_ok=True)
info = {"Device Name": "Test iPhone", "IsEncrypted": False,
        "Product Version": "14.0", "Unique Device ID": "TEST-UDID-SQLITE"}
with open(f"{root}/Info.plist", "wb") as f:
    f.write(plistlib.dumps(info, fmt=plistlib.FMT_XML))
db = f"{root}/Manifest.db"
conn = sqlite3.connect(db)
conn.executescript("""
CREATE TABLE IF NOT EXISTS Files (fileID TEXT PRIMARY KEY, domain TEXT,
  relativePath TEXT, flags INTEGER, file BLOB);
CREATE TABLE IF NOT EXISTS Properties (key TEXT PRIMARY KEY, value BLOB);
DELETE FROM Files;
""")
meta = plistlib.dumps({"Size": 128, "LastModified": 1609459200.0,
                       "Birth": 1609455600.0}, fmt=plistlib.FMT_BINARY)
conn.execute("INSERT INTO Files VALUES (?,?,?,?,?)",
  ("a1b2c3d4e5f6789012345678901234567890abcd", "HomeDomain",
   "Library/Preferences/com.test.plist", 1, meta))
conn.commit(); conn.close()
PY
```

## manifest_db_keyed

Same as `manifest_db_minimal` but the `file` BLOB uses **NSKeyedArchiver** encoding (typical of real iOS 10+ backups).

```bash
./build/bin/purplepois0n --analyze-backup tests/fixtures/manifest_db_keyed
```

Expected: `Entries with metadata: 1`, UDID `TEST-UDID-KEYED`.

Regenerate keyed blob + database:

```bash
HB=$(brew --prefix)
g++ -std=c++14 -I"$HB/include" tests/fixtures/gen_keyed_blob.cpp -L"$HB/lib" -lplist-2.0 -o /tmp/gen_keyed_blob
/tmp/gen_keyed_blob tests/fixtures/manifest_db_keyed/file.blob
# then insert blob into Manifest.db (see repo history or manifest_db_keyed setup script)
```

## Mach-O smoke (no fixture)

```bash
./build/bin/purplepois0n --analyze-binary /bin/ls
```

## Limitations

- Encrypted backups: encryption flags detected; no keybag/decrypt in-tree.
- NSKeyedArchiver: NSDictionary-style `$objects` graphs only; exotic archiver classes may skip metadata.
