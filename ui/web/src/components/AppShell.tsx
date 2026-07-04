import { useState } from "react";
import { NavLink, Outlet, useLocation, useNavigate } from "react-router-dom";
import { useStore } from "../context/StoreContext";
import { Logo } from "./Logo";
import { SearchBar } from "./SearchBar";
import { StatusPills } from "./StatusPills";

const PRIMARY_NAV = [
  { to: "/store", label: "Store", icon: "store" },
  { to: "/jailbreak", label: "Jailbreak", icon: "jailbreak" },
  { to: "/device", label: "Device", icon: "device" },
] as const;

const TOOLS_NAV = [
  { to: "/tools/dfu", label: "USB console", icon: "usb" },
  { to: "/tools/pongo", label: "Boot console", icon: "boot" },
  { to: "/tools/agent", label: "Host service", icon: "host" },
] as const;

function NavIcon({ name }: { name: string }) {
  switch (name) {
    case "store":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path d="M3 4a1 1 0 011-1h12a1 1 0 011 1v2a1 1 0 01-.293.707L12 11.414V15a1 1 0 01-.553.894l-4 2A1 1 0 016 17v-5.586L3.293 6.707A1 1 0 013 6V4z" />
        </svg>
      );
    case "jailbreak":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path
            fillRule="evenodd"
            d="M11.3 1.046A1 1 0 0112 2v5h4a1 1 0 01.82 1.573l-7 10A1 1 0 018 18v-5H4a1 1 0 01-.82-1.573l7-10a1 1 0 011.12-.38z"
            clipRule="evenodd"
          />
        </svg>
      );
    case "device":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path fillRule="evenodd" d="M7 2a2 2 0 00-2 2v12a2 2 0 002 2h6a2 2 0 002-2V4a2 2 0 00-2-2H7zm3 14a1 1 0 100-2 1 1 0 000 2z" clipRule="evenodd" />
        </svg>
      );
    case "usb":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path d="M8 2a1 1 0 011 1v4.586l1.293-1.293a1 1 0 111.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 111.414-1.414L7 7.586V3a1 1 0 011-1zm5 8a2 2 0 100 4 2 2 0 000-4zM4 14a2 2 0 100 4 2 2 0 000-4zm12 0a2 2 0 100 4 2 2 0 000-4z" />
        </svg>
      );
    case "boot":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path fillRule="evenodd" d="M2 5a2 2 0 012-2h12a2 2 0 012 2v10a2 2 0 01-2 2H4a2 2 0 01-2-2V5zm3.293 1.293a1 1 0 011.414 0L10 9.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" clipRule="evenodd" />
        </svg>
      );
    case "host":
      return (
        <svg viewBox="0 0 20 20" fill="currentColor" aria-hidden>
          <path fillRule="evenodd" d="M2 5a2 2 0 012-2h12a2 2 0 012 2v10a2 2 0 01-2 2H4a2 2 0 01-2-2V5zm3.293 1.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" clipRule="evenodd" />
        </svg>
      );
    default:
      return null;
  }
}

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
        <Logo />
        <div className="header-search">
          <SearchBar value={searchQuery} onChange={onSearchChange} onSubmit={onSearchSubmit} />
        </div>
        <StatusPills />
      </header>

      <div className="app-body">
        <aside className="sidebar">
          <nav className="sidebar-nav" aria-label="Primary">
            <span className="sidebar-section-label">Menu</span>
            {PRIMARY_NAV.map((item) => (
              <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
                <NavIcon name={item.icon} />
                <span className="nav-label">{item.label}</span>
              </NavLink>
            ))}
          </nav>
          <div className="sidebar-divider" />
          <button
            type="button"
            className={`sidebar-tools-toggle ${toolsOpen ? "open" : ""}`}
            onClick={() => setToolsOpen((o) => !o)}
            aria-expanded={toolsOpen}
          >
            <NavIcon name="host" />
            <span className="nav-label">Tools</span>
            <span className="sidebar-chevron" aria-hidden>
              {toolsOpen ? "▾" : "▸"}
            </span>
          </button>
          {toolsOpen && (
            <nav className="sidebar-nav tools" aria-label="Tools">
              {TOOLS_NAV.map((item) => (
                <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
                  <NavIcon name={item.icon} />
                  <span className="nav-label">{item.label}</span>
                </NavLink>
              ))}
            </nav>
          )}
          <div className="sidebar-footer">
            <p className="sidebar-tagline">iOS research toolkit</p>
          </div>
        </aside>

        <main className={`app-main ${isStore ? "wide" : ""}`}>
          <Outlet />
        </main>
      </div>

      <nav className="bottom-nav" aria-label="Mobile">
        {PRIMARY_NAV.map((item) => (
          <NavLink key={item.to} to={item.to} className={({ isActive }) => (isActive ? "active" : "")}>
            <NavIcon name={item.icon} />
            <span>{item.label}</span>
          </NavLink>
        ))}
        <button
          type="button"
          className={moreOpen ? "active" : ""}
          onClick={() => setMoreOpen((o) => !o)}
          aria-label="More tools"
        >
          <NavIcon name="host" />
          <span>More</span>
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
