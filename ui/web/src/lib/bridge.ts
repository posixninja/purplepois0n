const AGENT_BASE =
  import.meta.env.DEV ? "/api" : (import.meta.env.VITE_AGENT_URL ?? "http://127.0.0.1:7749");

export interface AgentCapabilities {
  plugins?: boolean;
  doctor?: boolean;
  store?: boolean;
  normalSsh?: boolean;
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
  opts: { mode?: string; udid?: string },
  onEvent: (e: AgentEvent) => void,
): Promise<boolean> {
  return streamNdjson(
    "/jailbreak",
    { mode: opts.mode ?? "", udid: opts.udid ?? "" },
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

export async function storeSync(udid: string, storeRoot?: string): Promise<string> {
  const res = await fetch(`${AGENT_BASE}/store/sync`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ udid, storeRoot: storeRoot ?? "" }),
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
