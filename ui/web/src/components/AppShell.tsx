import { useState } from "react";
import { Link, NavLink, Outlet, useLocation, useNavigate } from "react-router-dom";
import { useStore } from "../context/StoreContext";
import { SearchBar } from "./SearchBar";
import { StatusPills } from "./StatusPills";

const PRIMARY_NAV = [
  { to: "/store", label: "Store" },
  { to: "/jailbreak", label: "Jailbreak" },
  { to: "/device", label: "Device" },
];

const TOOLS_NAV = [
  { to: "/tools/dfu", label: "USB" },
  { to: "/tools/pongo", label: "Boot console" },
  { to: "/tools/agent", label: "Host" },
];

export function AppShell() {
  const { searchQuery, setSearchQuery, filteredPackages } = useStore();
  const navigate = useNavigate();
  const location = useLocation();
  const [toolsOpen, setToolsOpen] = useState(location.pathname.startsWith("/tools"));
  const [moreOpen, setMoreOpen] = useState(false);

  const onSearchSubmit = () => {
    if (location.pathname !== "/store") navigate("/store");
    const first = filteredPackages[0];
    if (first) navigate(`/store?pkg=${encodeURIComponent(first.package)}`);
  };

  const onSearchChange = (q: string) => {
    setSearchQuery(q);
    if (q && location.pathname !== "/store") navigate("/store");
  };

  const isStore = location.pathname.startsWith("/store");

  return (
    <div className="app-shell">
      <header className="app-header">
        <Link to="/store" className="brand">
          purplepois0n
        </Link>
        <SearchBar value={searchQuery} onChange={onSearchChange} onSubmit={onSearchSubmit} />
        <StatusPills />
      </header>

      <div className="app-body">
        <aside className="sidebar">
          <nav className="sidebar-nav">
            {PRIMARY_NAV.map((item) => (
              <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
                {item.label}
              </NavLink>
            ))}
          </nav>
          <div className="sidebar-divider" />
          <button
            type="button"
            className={`sidebar-tools-toggle ${toolsOpen ? "open" : ""}`}
            onClick={() => setToolsOpen((o) => !o)}
          >
            Tools
          </button>
          {toolsOpen && (
            <nav className="sidebar-nav tools">
              {TOOLS_NAV.map((item) => (
                <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
                  {item.label}
                </NavLink>
              ))}
            </nav>
          )}
        </aside>

        <main className={`app-main ${isStore ? "wide" : ""}`}>
          <Outlet />
        </main>
      </div>

      <nav className="bottom-nav">
        {PRIMARY_NAV.map((item) => (
          <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
            {item.label}
          </NavLink>
        ))}
        <button
          type="button"
          className={moreOpen ? "active" : ""}
          onClick={() => setMoreOpen((o) => !o)}
        >
          More
        </button>
      </nav>

      {moreOpen && (
        <div className="more-menu">
          {TOOLS_NAV.map((item) => (
            <NavLink key={item.to} to={item.to} onClick={() => setMoreOpen(false)}>
              {item.label}
            </NavLink>
          ))}
        </div>
      )}
    </div>
  );
}
