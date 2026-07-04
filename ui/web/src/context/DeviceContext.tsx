import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from "react";
import { agentHealth, listDevices, type AgentDevice } from "../lib/bridge";
import { cpidSupport, cpidToSocName, guessMode, serialIndicatesPwned, webUsbSupported } from "../webusb/apple";

export type { AgentDevice };

export interface DeviceIdentity {
  source: "webusb" | "agent";
  mode: string;
  cpid: number;
  ecid: string;
  pwned: boolean;
  support: "supported" | "unsupported" | "unknown";
  label: string;
  udid?: string;
}

interface DeviceContextValue {
  webUsbAvailable: boolean;
  agentOk: boolean;
  pluginsEnabled: boolean;
  kpfBuilt: boolean;
  hasDevice: boolean;
  identity: DeviceIdentity | null;
  agentDevices: AgentDevice[];
  selectedUdid: string | null;
  selectedDevice: AgentDevice | null;
  setSelectedUdid: (udid: string | null) => void;
  setIdentity: (identity: DeviceIdentity | null) => void;
  refreshAgent: () => Promise<void>;
  clearDevice: () => void;
}

const DeviceContext = createContext<DeviceContextValue | null>(null);

function parseCpidHex(cpid?: string): number {
  if (!cpid) return 0;
  const m = cpid.match(/0x([0-9a-fA-F]+)/);
  return m ? parseInt(m[1], 16) : 0;
}

export function DeviceProvider({ children }: { children: ReactNode }) {
  const [webUsbAvailable] = useState(() => webUsbSupported());
  const [agentOk, setAgentOk] = useState(false);
  const [pluginsEnabled, setPluginsEnabled] = useState(false);
  const [kpfBuilt, setKpfBuilt] = useState(false);
  const [identity, setIdentity] = useState<DeviceIdentity | null>(null);
  const [agentDevices, setAgentDevices] = useState<AgentDevice[]>([]);
  const [selectedUdid, setSelectedUdid] = useState<string | null>(null);

  const refreshAgent = useCallback(async () => {
    const h = await agentHealth();
    setAgentOk(!!h?.ok);
    setPluginsEnabled(!!h?.plugins);
    setKpfBuilt(!!h?.capabilities?.kpf?.built);
    if (h?.ok) {
      try {
        const devices = await listDevices();
        setAgentDevices(devices);
        setSelectedUdid((prev) => {
          if (prev && devices.some((d) => d.udid === prev)) return prev;
          if (devices.length === 1) return devices[0].udid;
          return prev;
        });
      } catch {
        setAgentDevices([]);
      }
    } else {
      setAgentDevices([]);
    }
  }, []);

  useEffect(() => {
    refreshAgent();
    const id = window.setInterval(refreshAgent, 5000);
    return () => window.clearInterval(id);
  }, [refreshAgent]);

  const clearDevice = useCallback(() => {
    setIdentity(null);
    setSelectedUdid(null);
  }, []);

  const selectedDevice = useMemo(
    () => agentDevices.find((d) => d.udid === selectedUdid) ?? null,
    [agentDevices, selectedUdid],
  );

  const value = useMemo(
    () => ({
      webUsbAvailable,
      agentOk,
      pluginsEnabled,
      kpfBuilt,
      hasDevice: !!identity || agentDevices.length > 0,
      identity,
      agentDevices,
      selectedUdid,
      selectedDevice,
      setSelectedUdid,
      setIdentity,
      refreshAgent,
      clearDevice,
    }),
    [
      webUsbAvailable,
      agentOk,
      pluginsEnabled,
      kpfBuilt,
      identity,
      agentDevices,
      selectedUdid,
      selectedDevice,
      refreshAgent,
      clearDevice,
    ],
  );

  return <DeviceContext.Provider value={value}>{children}</DeviceContext.Provider>;
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error("useDevice outside DeviceProvider");
  return ctx;
}

export function identityFromUsb(
  serial: string,
  cpid: number,
  ecid: bigint | null,
  mode: ReturnType<typeof guessMode>,
  label: string,
): DeviceIdentity {
  return {
    source: "webusb",
    mode,
    cpid,
    ecid: ecid !== null ? "0x" + ecid.toString(16) : "—",
    pwned: serialIndicatesPwned(serial),
    support: cpidSupport(cpid),
    label: `${label} — ${cpidToSocName(cpid)}`,
  };
}

export function identityFromAgent(device: AgentDevice): DeviceIdentity {
  const cpid = parseCpidHex(device.cpid);
  return {
    source: "agent",
    mode: device.state || "unknown",
    cpid,
    ecid: device.ecid ?? "—",
    pwned: device.state === "dfu" || device.state === "recovery",
    support: cpid ? cpidSupport(cpid) : "unknown",
    label: device.deviceType
      ? `${device.deviceType}${cpid ? ` — ${cpidToSocName(cpid)}` : ""}`
      : cpid
        ? cpidToSocName(cpid)
        : "Connected device",
    udid: device.udid,
  };
}
