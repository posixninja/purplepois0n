# Legacy GitHub mirrors

Local **read-only** `git clone` trees of jailbreak-era GitHub organizations used for lineage research alongside [purplepois0n](../). Contents under `Chronic-Dev/`, `OpenJailbreak/`, and `posixninja/` are **gitignored** at the repo root so hundreds of upstream repos are not committed into purplepois0n.

**Snapshot date:** 2026-06-01

## Layout

| Path | Upstream |
|------|----------|
| `Chronic-Dev/` | https://github.com/Chronic-Dev |
| `OpenJailbreak/` | https://github.com/OpenJailbreak |
| `posixninja/` | https://github.com/posixninja |
| `modern-era/` | Selective Gen 6 clones — see [clone-modern-era.sh](clone-modern-era.sh) |

Policy and integration notes: [docs/ARCHIVES.md](../docs/ARCHIVES.md). Phase rollup: [docs/legacy/PHASE_STATUS.md](../docs/legacy/PHASE_STATUS.md). **Gen 6 study guide:** [docs/legacy/MODERN_ERA_LEARNINGS.md](../docs/legacy/MODERN_ERA_LEARNINGS.md).

## Optional duplicate

You may already maintain a separate Chronic-Dev archive (for example `/Users/posix/Desktop/Companies/Chronic-Dev/`). That tree is independent; this `legacy/` layout keeps mirrors colocated with the purplepois0n workspace. Do not delete the other copy unless you intend to consolidate.

## Refresh

Requires network access. Clones are **idempotent**: skip any directory that already contains a `.git` folder.

### List public repos (GitHub API)

```bash
# Chronic-Dev (org)
curl -sS -H "Accept: application/vnd.github+json" \
  "https://api.github.com/orgs/Chronic-Dev/repos?per_page=100&page=1" \
  | python3 -c "import sys,json; [print(r['full_name']) for r in json.load(sys.stdin)]"

# OpenJailbreak (org)
curl -sS -H "Accept: application/vnd.github+json" \
  "https://api.github.com/orgs/OpenJailbreak/repos?per_page=100&page=1" \
  | python3 -c "import sys,json; [print(r['full_name']) for r in json.load(sys.stdin)]"

# posixninja (user)
curl -sS -H "Accept: application/vnd.github+json" \
  "https://api.github.com/users/posixninja/repos?per_page=100&type=owner&page=1" \
  | python3 -c "import sys,json; [print(r['full_name']) for r in json.load(sys.stdin)]"
```

With [GitHub CLI](https://cli.github.com/) installed:

```bash
gh repo list Chronic-Dev --limit 200 --json nameWithOwner -q '.[].nameWithOwner'
gh repo list OpenJailbreak --limit 200 --json nameWithOwner -q '.[].nameWithOwner'
gh repo list posixninja --limit 200 --json nameWithOwner -q '.[].nameWithOwner'
```

### Clone (example)

```bash
ROOT="$(cd "$(dirname "$0")" && pwd)"
ORG=Chronic-Dev   # or OpenJailbreak, posixninja
mkdir -p "$ROOT/$ORG"
while read -r full; do
  name="${full##*/}"
  dest="$ROOT/$ORG/$name"
  [[ -d "$dest/.git" ]] && continue
  git clone "https://github.com/${full}.git" "$dest"
done < <(curl -sS -H "Accept: application/vnd.github+json" \
  "https://api.github.com/orgs/${ORG}/repos?per_page=100&page=1" \
  | python3 -c "import sys,json; [print(r['full_name']) for r in json.load(sys.stdin)]")
```

For `posixninja`, use `users/posixninja/repos?per_page=100&type=owner&page=1` instead of `orgs/…`.

### Gen 6 modern era (selective)

Not a bulk org mirror — 16 curated repos for Dopamine / PUAF / rootless study:

```bash
./legacy/clone-modern-era.sh
```

See [docs/legacy/MODERN_ERA_LEARNINGS.md](../docs/legacy/MODERN_ERA_LEARNINGS.md) for architecture synthesis and reading order.

### Update existing clones

```bash
find legacy -name .git -type d | while read -r g; do
  (cd "$(dirname "$g")" && git fetch --all --quiet && git pull --ff-only --quiet) || true
done
```

## Snapshot counts (2026-06-01)

| Mirror | Repos cloned |
|--------|----------------|
| Chronic-Dev | 36 |
| OpenJailbreak | 46 |
| posixninja | 29 |
| **Total** | **111** |

## Known failures

- **OpenJailbreak/Chimera13** — GitHub returns HTTP 451 / “Repository access blocked”; not cloneable via public HTTPS as of this snapshot.

## Gen-0 priority set (if rate- or time-limited)

Clone these first when refreshing selectively:

- **Chronic-Dev:** greenpois0n, absinthe, absinthe-2.0, gp2, gprc5, syringe, poison-jb, libirecovery, idevicerestore, apparition, medicine  
- **OpenJailbreak:** greenpois0n, absinthe, absinthe-2.0, libirecovery, libmbdb  
- **posixninja:** libchronic, libirecovery-2.0, anthrax, spirit-linux  

