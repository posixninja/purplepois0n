import type { AptPackage } from "../lib/packages";

export function PackageCard({ pkg, onClick }: { pkg: AptPackage; onClick: () => void }) {
  const initial = pkg.package.charAt(0).toUpperCase();
  return (
    <button type="button" className="package-card" onClick={onClick}>
      <div className="package-icon" aria-hidden>
        {initial}
      </div>
      <div className="package-body">
        <div className="package-name">{pkg.package}</div>
        <div className="package-meta">
          {pkg.version}
          {pkg.architecture ? ` · ${pkg.architecture}` : ""}
        </div>
        <div className="package-desc">{pkg.description || "No description"}</div>
      </div>
    </button>
  );
}
