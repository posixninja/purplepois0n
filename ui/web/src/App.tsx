import { Navigate, Route, Routes } from "react-router-dom";
import { AppShell } from "./components/AppShell";
import { DeviceProvider } from "./context/DeviceContext";
import { StoreProvider } from "./context/StoreContext";
import { AgentPage } from "./pages/Agent";
import { DevicePage } from "./pages/Device";
import { DfuPage } from "./pages/Dfu";
import { JailbreakPage } from "./pages/Jailbreak";
import { PongoPage } from "./pages/Pongo";
import { StorePage } from "./pages/Store";
import "./App.css";

export function App() {
  return (
    <DeviceProvider>
      <StoreProvider>
        <Routes>
          <Route element={<AppShell />}>
            <Route path="/" element={<Navigate to="/store" replace />} />
            <Route path="/store" element={<StorePage />} />
            <Route path="/jailbreak" element={<JailbreakPage />} />
            <Route path="/device" element={<DevicePage />} />
            <Route path="/tools/dfu" element={<DfuPage />} />
            <Route path="/tools/pongo" element={<PongoPage />} />
            <Route path="/tools/agent" element={<AgentPage />} />
            {/* legacy redirects */}
            <Route path="/dfu" element={<Navigate to="/tools/dfu" replace />} />
            <Route path="/pongo" element={<Navigate to="/tools/pongo" replace />} />
            <Route path="/agent" element={<Navigate to="/tools/agent" replace />} />
          </Route>
        </Routes>
      </StoreProvider>
    </DeviceProvider>
  );
}
