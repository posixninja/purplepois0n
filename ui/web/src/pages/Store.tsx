import { useEffect, useState } from "react";
import { useSearchParams } from "react-router-dom";
import { CopyButton } from "../components/CopyButton";
import { PackageCard } from "../components/PackageCard";
import { PackageDetail } from "../components/PackageDetail";
import { SourceChips } from "../components/SourceChips";
import { useStore } from "../context/StoreContext";
import { agentHealth, storePublish } from "../lib/bridge";
import type { AptPackage } from "../lib/packages";

export function StorePage() {
  const {
    filteredPackages,
    sources,
    activeSourceId,
    loading,
    error,
    categories,
    activeCategory,
    setActiveCategory,
    loadFromAgent,
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

  const publishViaAgent = async () => {
    try {
      const h = await agentHealth();
      if (!h?.ok) throw new Error("Start host service: make agent");
      const r = await storePublish(h.storeRoot);
      alert(`Published to ${r.publishRoot}\nServe that directory over HTTPS for devices.`);
    } catch (e) {
      alert(e instanceof Error ? e.message : String(e));
    }
  };

  return (
    <div className="page store-page">
      <section className="store-hero">
        <h1>purplepois0n packages</h1>
        <p className="lead">Browse your default package repos and install on device after jailbreak.</p>
        <div className="toolbar">
          <button type="button" disabled={loading} onClick={loadFromAgent}>
            Refresh catalog
          </button>
          <button type="button" onClick={publishViaAgent}>
            Publish
          </button>
        </div>
      </section>

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

      {loading && <p className="muted">Loading packages…</p>}

      {!loading && filteredPackages.length === 0 && !error && (
        <div className="empty-state">
          <p>No packages loaded.</p>
          <p className="muted">Refresh the built-in sources or add another Packages URL.</p>
          <button type="button" onClick={loadFromAgent}>
            Refresh catalog
          </button>
        </div>
      )}

      <div className="package-grid">
        {filteredPackages.map((p) => (
          <PackageCard key={`${p.package}-${p.version}`} pkg={p} onClick={() => setSelected(p)} />
        ))}
      </div>

      {selected && <PackageDetail pkg={selected} onClose={() => setSelected(null)} />}
    </div>
  );
}
