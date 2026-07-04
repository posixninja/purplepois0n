import type { AptPackage } from "../lib/packages";

export function PackageCard({
  pkg,
  installed,
  onClick,
}: {
  pkg: AptPackage;
  installed?: boolean;
  onClick: () => void;
}) {
  const initial = pkg.package.charAt(0).toUpperCase();
  return (
    <button type="button" className="package-card" onClick={onClick}>
      <div className="package-icon" aria-hidden>
        {initial}
      </div>
      <div className="package-body">
        <div className="package-name-row">
          <div className="package-name">{pkg.package}</div>
          {installed && <span className="package-installed-badge">Installed</span>}
        </div>
        <div className="package-meta">
          {pkg.version}
          {pkg.architecture ? ` · ${pkg.architecture}` : ""}
        </div>
        <div className="package-desc">{pkg.description || "No description"}</div>
      </div>
    </button>
  );
}
