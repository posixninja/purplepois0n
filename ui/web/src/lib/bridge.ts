const AGENT_BASE =
  import.meta.env.DEV ? "/api" : (import.meta.env.VITE_AGENT_URL ?? "http://127.0.0.1:7749");

export interface AgentCapabilities {
  plugins?: boolean;
  doctor?: boolean;
  store?: boolean;
  normalSsh?: boolean;
  kpf?: {
    built?: boolean;
    moduleBuilt?: boolean;
    testBuilt?: boolean;
    module?: string;
  };
}

export interface AgentHealth {
  ok: boolean;
  version?: string;
  bin?: string;
  storeRoot?: string;
  plugins?: boolean;
  capabilities?: AgentCapabilities;
}

export interface AgentDevice {
  udid: string;
  state: string;
  ecid?: string;
  cpid?: string;
  deviceType?: string;
  firmware?: string;
}

export interface AgentEvent {
  type: string;
  id?: string;
  phase?: string;
  success?: boolean;
  detail?: string;
  command?: string;
  transport?: string;
}

async function streamNdjson(
  path: string,
  body: unknown,
  onEvent: (e: AgentEvent) => void,
): Promise<boolean> {
  const res = await fetch(`${AGENT_BASE}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body ?? {}),
  });
  if (!res.ok || !res.body) {
    throw new Error(`Agent ${path} failed: ${res.status}`);
  }
  const reader = res.body.getReader();
  const dec = new TextDecoder();
  let buf = "";
  let success = false;
  const flushLine = (line: string) => {
    const trimmed = line.trim();
    if (!trimmed.startsWith("{")) return;
    try {
      const ev = JSON.parse(trimmed) as AgentEvent;
      onEvent(ev);
      if (ev.type === "complete") success = !!ev.success;
    } catch {
      onEvent({ type: "log", detail: trimmed });
    }
  };
  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    buf += dec.decode(value, { stream: true });
    const lines = buf.split("\n");
    buf = lines.pop() ?? "";
    for (const line of lines) flushLine(line);
  }
  if (buf.trim()) flushLine(buf);
  return success;
}

export interface DevicePlanDevice {
  state: string;
  cpid?: string;
  ecid?: string;
  soc?: string;
  productType?: string;
  iosVersion?: string;
  udid?: string;
  era?: string;
  checkm8?: boolean;
  usbliter8?: boolean;
}

export interface DevicePlanPayload {
  strategy: string;
  summary: string;
  bootLane?: string;
  canProbe?: boolean;
  canExecute?: boolean;
  useBootromExploit?: boolean;
  useBootDelivery?: boolean;
  useExternalDelegate?: boolean;
  useEraChain?: boolean;
  useRecoveryChain?: boolean;
  alreadyJailbroken?: boolean;
  bootModule?: string;
  ipsw?: string;
  ramdiskSource?: string;
  blockers?: string[];
}

export interface DevicePlan {
  device: DevicePlanDevice;
  plan: DevicePlanPayload;
  error?: string;
}

export async function fetchDevicePlan(udid?: string): Promise<DevicePlan> {
  const q = udid ? `?udid=${encodeURIComponent(udid)}` : "";
  const res = await fetch(`${AGENT_BASE}/device/plan${q}`);
  const data = (await res.json()) as DevicePlan;
  if (!res.ok) {
    throw new Error(data.error ?? `device plan failed: ${res.status}`);
  }
  return data;
}

export async function agentHealth(): Promise<AgentHealth | null> {
  try {
    const res = await fetch(`${AGENT_BASE}/health`, { signal: AbortSignal.timeout(2000) });
    if (!res.ok) return null;
    return (await res.json()) as AgentHealth;
  } catch {
    return null;
  }
}

export async function runDoctor(
  execute: boolean,
  onEvent: (e: AgentEvent) => void,
  udid?: string,
): Promise<boolean> {
  return streamNdjson("/doctor", { execute, udid: udid ?? "" }, onEvent);
}

export async function runJailbreak(
  opts: {
    mode?: string;
    udid?: string;
    alreadyJailbroken?: boolean;
    postJbStore?: boolean;
    auto?: boolean;
    execute?: boolean;
  },
  onEvent: (e: AgentEvent) => void,
): Promise<boolean> {
  return streamNdjson(
    "/jailbreak",
    {
      mode: opts.mode ?? "",
      udid: opts.udid ?? "",
      already_jailbroken: !!opts.alreadyJailbroken,
      post_jb_store: !!opts.postJbStore,
      auto: !!opts.auto,
      execute: !!opts.execute,
      i_understand_jailbreak: !!opts.execute,
    },
    onEvent,
  );
}


export async function runExternalJailbreak(
  opts: { udid?: string; alreadyJailbroken?: boolean; postJbStore?: boolean },
  onEvent: (e: AgentEvent) => void,
): Promise<boolean> {
  return streamNdjson(
    "/external-jailbreak",
    {
      udid: opts.udid ?? "",
      already_jailbroken: !!opts.alreadyJailbroken,
      post_jb_store: !!opts.postJbStore,
    },
    onEvent,
  );
}

export async function runCheckm8(
  onEvent: (e: AgentEvent) => void,
  udid?: string,
): Promise<boolean> {
  return streamNdjson("/checkm8", { udid: udid ?? "" }, onEvent);
}

export async function runDfuJailbreak(
  onEvent: (e: AgentEvent) => void,
  udid?: string,
): Promise<boolean> {
  return streamNdjson("/dfu-jailbreak", { udid: udid ?? "" }, onEvent);
}

export async function listDevices(): Promise<AgentDevice[]> {
  const res = await fetch(`${AGENT_BASE}/devices`);
  if (!res.ok) throw new Error("list devices failed");
  const data = (await res.json()) as { devices: AgentDevice[] };
  return data.devices ?? [];
}

export async function storeSync(
  udid: string,
  storeRoot?: string,
  syncMode = "file",
): Promise<string> {
  const res = await fetch(`${AGENT_BASE}/store/sync`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ udid, storeRoot: storeRoot ?? "", syncMode }),
  });
  const data = (await res.json()) as { ok?: boolean; detail?: string; error?: string };
  if (!res.ok || !data.ok) {
    throw new Error(data.error ?? data.detail ?? `sync failed: ${res.status}`);
  }
  return data.detail ?? "synced";
}

export async function storeInstall(
  udid: string,
  packageName: string,
  storeRoot?: string,
): Promise<string> {
  const res = await fetch(`${AGENT_BASE}/store/install`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ udid, package: packageName, storeRoot: storeRoot ?? "" }),
  });
  const data = (await res.json()) as { ok?: boolean; detail?: string; error?: string };
  if (!res.ok || !data.ok) {
    throw new Error(data.error ?? data.detail ?? `install failed: ${res.status}`);
  }
  return data.detail ?? `installed ${packageName}`;
}

export async function storePublish(
  storeRoot?: string,
  publishRoot?: string,
): Promise<{ publishRoot: string }> {
  const res = await fetch(`${AGENT_BASE}/store/publish`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      storeRoot: storeRoot ?? "",
      publishRoot: publishRoot ?? "",
    }),
  });
  if (!res.ok) throw new Error(await res.text());
  return (await res.json()) as { publishRoot: string };
}

export async function fetchStorePackages(storeRoot?: string): Promise<string> {
  const q = storeRoot ? `?storeRoot=${encodeURIComponent(storeRoot)}` : "";
  const res = await fetch(`${AGENT_BASE}/store/packages${q}`);
  if (!res.ok) throw new Error(await res.text());
  return res.text();
}

export async function fetchStoreInstalled(udid: string): Promise<string[]> {
  const q = `?udid=${encodeURIComponent(udid)}`;
  const res = await fetch(`${AGENT_BASE}/store/installed${q}`);
  const data = (await res.json()) as { packages?: string[]; error?: string };
  if (!res.ok) {
    throw new Error(data.error ?? `installed probe failed: ${res.status}`);
  }
  return data.packages ?? [];
}
