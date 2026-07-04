import { useEffect, useState } from "react";
import { useSearchParams } from "react-router-dom";
import { CopyButton } from "../components/CopyButton";
import { PackageCard } from "../components/PackageCard";
import { PackageDetail } from "../components/PackageDetail";
import { PageHeader } from "../components/PageHeader";
import { SourceChips } from "../components/SourceChips";
import { useDevice } from "../context/DeviceContext";
import { useStore } from "../context/StoreContext";
import { agentHealth, storePublish } from "../lib/bridge";
import type { AptPackage } from "../lib/packages";

export function StorePage() {
  const { selectedUdid } = useDevice();
  const {
    filteredPackages,
    installedPackages,
    sources,
    activeSourceId,
    loading,
    error,
    categories,
    activeCategory,
    setActiveCategory,
    loadFromAgent,
    refreshInstalled,
    addSource,
    removeSource,
    selectSource,
  } = useStore();
  const [selected, setSelected] = useState<AptPackage | null>(null);
  const [params] = useSearchParams();

  useEffect(() => {
    const pkgName = params.get("pkg");
    if (pkgName) {
      const found = filteredPackages.find((p) => p.package === pkgName);
      if (found) setSelected(found);
    }
  }, [params, filteredPackages]);

  useEffect(() => {
    if (selectedUdid) {
      void refreshInstalled(selectedUdid);
    }
  }, [selectedUdid, refreshInstalled, filteredPackages.length]);

  const publishViaAgent = async () => {
    try {
      const h = await agentHealth();
      if (!h?.ok) throw new Error("Start host service: make agent");
      const r = await storePublish(h.storeRoot);
      const repoUrl =
        import.meta.env.VITE_REPO_URL ?? "https://YOUR_HOST/purplepois0n-repo/";
      alert(
        `Published to ${r.publishRoot}\n\n` +
          `Upload that directory over HTTPS, then add on device:\n` +
          `deb [trusted=yes] ${repoUrl} purplepois0n main`,
      );
    } catch (e) {
      alert(e instanceof Error ? e.message : String(e));
    }
  };

  const actions = (
    <>
      <button type="button" className="btn-secondary" disabled={loading} onClick={loadFromAgent}>
        {loading ? "Refreshing…" : "Refresh"}
      </button>
      <button type="button" className="btn-primary" onClick={publishViaAgent}>
        Publish repo
      </button>
    </>
  );

  return (
    <div className="page store-page">
      <PageHeader
        variant="hero"
        title="Package Store"
        description="Browse the host catalog (packages on device show an Installed badge when a UDID is selected)."
        actions={actions}
      />

      <div className="store-stats">
        <div className="stat-card">
          <span className="stat-value">{filteredPackages.length}</span>
          <span className="stat-label">Packages</span>
        </div>
        <div className="stat-card">
          <span className="stat-value">{sources.length}</span>
          <span className="stat-label">Sources</span>
        </div>
      </div>

      <SourceChips
        sources={sources}
        activeId={activeSourceId}
        onSelect={selectSource}
        onRemove={removeSource}
        onAdd={addSource}
      />

      {error && (
        <div className="error-banner">
          {error}
          {error.includes("make agent") && <CopyButton text="make agent" />}
        </div>
      )}

      {categories.length > 1 && (
        <div className="category-chips">
          {categories.map((c) => (
            <button
              key={c}
              type="button"
              className={`chip ${activeCategory === c ? "active" : ""}`}
              onClick={() => setActiveCategory(c)}
            >
              {c}
            </button>
          ))}
        </div>
      )}

      {loading && <p className="muted loading-line">Loading catalog…</p>}

      {!loading && filteredPackages.length === 0 && !error && (
        <div className="empty-state">
          <img src="/logo.svg" alt="" className="empty-logo" width={48} height={48} />
          <p>No packages yet</p>
          <p className="muted">Run <code>legacy/scripts/seed-store.sh</code> on the host, then refresh.</p>
          <button type="button" className="btn-primary" onClick={loadFromAgent}>
            Refresh catalog
          </button>
        </div>
      )}

      <div className="package-grid">
        {filteredPackages.map((p) => (
          <PackageCard
            key={`${p.package}-${p.version}`}
            pkg={p}
            installed={installedPackages.has(p.package)}
            onClick={() => setSelected(p)}
          />
        ))}
      </div>

      {selected && <PackageDetail pkg={selected} onClose={() => setSelected(null)} />}
    </div>
  );
}
