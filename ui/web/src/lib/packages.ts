export interface AptPackage {
  package: string;
  version: string;
  architecture: string;
  description: string;
  filename: string;
  size: number;
  sha256: string;
  section: string;
  maintainer: string;
  depends: string;
}

export interface DefaultRepoSource {
  label: string;
  url: string;
}

const DEFAULT_PACKAGES_PATH = "/dists/purplepois0n/main/binary-iphoneos-arm64/Packages";

export function parsePackages(text: string): AptPackage[] {
  const blocks = text.split(/\n\n+/);
  const out: AptPackage[] = [];
  for (const block of blocks) {
    if (!block.trim()) continue;
    const fields: Record<string, string> = {};
    for (const line of block.split("\n")) {
      const idx = line.indexOf(":");
      if (idx < 0) continue;
      const key = line.slice(0, idx).trim();
      const val = line.slice(idx + 1).trim();
      fields[key] = val;
    }
    if (!fields.Package) continue;
    out.push({
      package: fields.Package,
      version: fields.Version ?? "",
      architecture: fields.Architecture ?? "",
      description: fields.Description ?? "",
      filename: fields.Filename ?? "",
      size: parseInt(fields.Size ?? "0", 10) || 0,
      sha256: fields.SHA256 ?? "",
      section: fields.Section ?? "",
      maintainer: fields.Maintainer ?? "",
      depends: fields.Depends ?? "",
    });
  }
  return out;
}

export async function fetchPackagesFromUrl(url: string): Promise<AptPackage[]> {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`HTTP ${res.status} for ${url}`);
  const text = await res.text();
  return parsePackages(text);
}

export function normalizePackagesUrl(input: string): string {
  const trimmed = input.trim();
  if (!trimmed) return trimmed;
  if (/\/Packages(?:\.(?:gz|bz2))?$/i.test(trimmed)) return trimmed;
  return trimmed.replace(/\/+$/, "") + DEFAULT_PACKAGES_PATH;
}

export function parseDefaultRepoConfig(): DefaultRepoSource[] {
  const sources: DefaultRepoSource[] = [];
  const purpleRoot = import.meta.env.VITE_PURPLEPOIS0N_REPO_URL as string | undefined;
  if (purpleRoot) {
    sources.push({
      label: "purplepois0n",
      url: normalizePackagesUrl(purpleRoot),
    });
  }

  const raw = import.meta.env.VITE_DEFAULT_DPKG_REPOS as string | undefined;
  if (!raw) return sources;

  const entries = raw
    .split(/\r?\n|,/)
    .map((entry) => entry.trim())
    .filter(Boolean);
  for (const entry of entries) {
    const eq = entry.indexOf("=");
    const label = eq > 0 ? entry.slice(0, eq).trim() : "";
    const url = eq > 0 ? entry.slice(eq + 1).trim() : entry;
    if (!url) continue;
    sources.push({
      label: label || (() => {
        try {
          return new URL(url).hostname;
        } catch {
          return "Default repo";
        }
      })(),
      url: normalizePackagesUrl(url),
    });
  }
  return sources;
}

export function packageCategories(packages: AptPackage[]): string[] {
  const secs = new Set<string>();
  for (const p of packages) {
    if (p.section) secs.add(p.section);
  }
  return Array.from(secs).sort();
}
