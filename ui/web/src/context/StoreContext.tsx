import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from "react";
import { agentHealth, fetchStorePackages } from "../lib/bridge";
import {
  fetchPackagesFromUrl,
  normalizePackagesUrl,
  parseDefaultRepoConfig,
  parsePackages,
  type AptPackage,
} from "../lib/packages";

export interface AptSource {
  id: string;
  label: string;
  url: string;
  builtin?: boolean;
}

interface StoreContextValue {
  packages: AptPackage[];
  sources: AptSource[];
  activeSourceId: string | null;
  loading: boolean;
  error: string;
  searchQuery: string;
  setSearchQuery: (q: string) => void;
  categories: string[];
  activeCategory: string;
  setActiveCategory: (c: string) => void;
  filteredPackages: AptPackage[];
  loadFromAgent: () => Promise<void>;
  addSource: (url: string, label?: string) => Promise<void>;
  removeSource: (id: string) => void;
  selectSource: (id: string) => Promise<void>;
}

const StoreContext = createContext<StoreContextValue | null>(null);

const DEFAULT_SOURCES: AptSource[] = [
  { id: "agent", label: "Host store", url: "agent:", builtin: true },
  ...parseDefaultRepoConfig().map((source, index) => ({
    id: `default-${index}`,
    label: source.label,
    url: source.url,
    builtin: true,
  })),
];

export function StoreProvider({ children }: { children: ReactNode }) {
  const [packages, setPackages] = useState<AptPackage[]>([]);
  const [sources, setSources] = useState<AptSource[]>(DEFAULT_SOURCES);
  const [activeSourceId, setActiveSourceId] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [searchQuery, setSearchQuery] = useState("");
  const [activeCategory, setActiveCategory] = useState("All");

  const loadText = useCallback((text: string, source: AptSource) => {
    setPackages(parsePackages(text));
    setActiveSourceId(source.id);
    setSources((prev) => {
      if (prev.some((s) => s.id === source.id)) return prev;
      return [...prev, source];
    });
    setError("");
  }, []);

  const loadAgentSource = useCallback(async (): Promise<boolean> => {
    setLoading(true);
    setError("");
    try {
      const h = await agentHealth();
      if (!h?.ok) throw new Error("Start host service: make agent");
      const text = await fetchStorePackages(h.storeRoot);
      loadText(text, {
        id: "agent",
        label: "Host store",
        url: `agent:${h.storeRoot ?? "store"}`,
        builtin: true,
      });
      return true;
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      setPackages([]);
      return false;
    } finally {
      setLoading(false);
    }
  }, [loadText]);

  const loadFromAgent = useCallback(async () => {
    await loadAgentSource();
  }, [loadAgentSource]);

  const loadFromUrlSource = useCallback(async (source: AptSource): Promise<boolean> => {
    setLoading(true);
    setError("");
    try {
      const pkgs = await fetchPackagesFromUrl(source.url);
      setPackages(pkgs);
      setActiveSourceId(source.id);
      return true;
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      return false;
    } finally {
      setLoading(false);
    }
  }, []);

  const addSource = useCallback(
    async (url: string, label?: string) => {
      setLoading(true);
      setError("");
      try {
        const normalizedUrl = normalizePackagesUrl(url);
        const pkgs = await fetchPackagesFromUrl(normalizedUrl);
        const source: AptSource = {
          id: `url-${Date.now()}`,
          label: label || (() => { try { return new URL(normalizedUrl).hostname; } catch { return "Source"; } })(),
          url: normalizedUrl,
        };
        setPackages(pkgs);
        setActiveSourceId(source.id);
        setSources((prev) => [...prev.filter((s) => s.id !== source.id), source]);
      } catch (e) {
        setError(e instanceof Error ? e.message : String(e));
      } finally {
        setLoading(false);
      }
    },
    [],
  );

  const selectSource = useCallback(
    async (id: string) => {
      const source = sources.find((s) => s.id === id);
      if (!source) return;
      if (source.id === "agent") {
        await loadAgentSource();
        return;
      }
      await loadFromUrlSource(source);
    },
    [sources, loadAgentSource, loadFromUrlSource],
  );

  const removeSource = useCallback((id: string) => {
    const source = sources.find((entry) => entry.id === id);
    if (source?.builtin) {
      return;
    }
    setSources((prev) => prev.filter((s) => s.id !== id));
    if (activeSourceId === id) {
      setActiveSourceId(null);
      setPackages([]);
    }
  }, [activeSourceId, sources]);

  useEffect(() => {
    if (activeSourceId !== null || packages.length > 0) return;
    let cancelled = false;
    const loadDefaults = async () => {
      const builtins = DEFAULT_SOURCES;
      for (const source of builtins) {
        const ok = source.id === "agent" ? await loadAgentSource() : await loadFromUrlSource(source);
        if (ok || cancelled) {
          return;
        }
      }
    };
    void loadDefaults();
    return () => {
      cancelled = true;
    };
  }, [activeSourceId, packages.length, loadAgentSource, loadFromUrlSource]);

  const categories = useMemo(() => {
    const secs = new Set<string>();
    for (const p of packages) {
      if (p.section) secs.add(p.section);
    }
    return ["All", ...Array.from(secs).sort()];
  }, [packages]);

  const filteredPackages = useMemo(() => {
    let list = packages;
    if (activeCategory !== "All") {
      list = list.filter((p) => p.section === activeCategory);
    }
    const q = searchQuery.trim().toLowerCase();
    if (q) {
      list = list.filter(
        (p) =>
          p.package.toLowerCase().includes(q) ||
          p.description.toLowerCase().includes(q) ||
          p.maintainer.toLowerCase().includes(q),
      );
    }
    return list;
  }, [packages, activeCategory, searchQuery]);

  const value = useMemo(
    () => ({
      packages,
      sources,
      activeSourceId,
      loading,
      error,
      searchQuery,
      setSearchQuery,
      categories,
      activeCategory,
      setActiveCategory,
      filteredPackages,
      loadFromAgent,
      addSource,
      removeSource,
      selectSource,
    }),
    [
      packages,
      sources,
      activeSourceId,
      loading,
      error,
      searchQuery,
      categories,
      activeCategory,
      filteredPackages,
      loadFromAgent,
      addSource,
      removeSource,
      selectSource,
    ],
  );

  return <StoreContext.Provider value={value}>{children}</StoreContext.Provider>;
}

export function useStore() {
  const ctx = useContext(StoreContext);
  if (!ctx) throw new Error("useStore outside StoreProvider");
  return ctx;
}
