import { useEffect, useState } from "react";
import { Breadcrumb } from "../components/Breadcrumb";
import { CopyButton } from "../components/CopyButton";
import { SplitPanel } from "../components/SplitPanel";
import { StepLog } from "../components/StepLog";
import { useDevice } from "../context/DeviceContext";
import { agentHealth, type AgentHealth } from "../lib/bridge";
import { useAgentStream } from "../hooks/useAgentStream";

export function AgentPage() {
  const { agentDevices, selectedUdid } = useDevice();
  const [health, setHealth] = useState<AgentHealth | null>(null);
  const agent = useAgentStream();

  const refresh = async () => {
    const h = await agentHealth();
    setHealth(h);
  };

  useEffect(() => {
    refresh();
  }, []);

  const controls = (
    <>
      <Breadcrumb section="Host" />
      <h1>Host service</h1>
      <p className="lead">
        Advanced — host bridge to your device. Run <code>make agent</code>{" "}
        {!health?.ok && <CopyButton text="make agent" />}
      </p>
      <div className={`status-card inline ${health?.ok ? "ok" : "off"}`}>
        {health?.ok ? "Host service connected" : "Host service offline"}
      </div>
      {health?.ok && health.plugins === false && (
        <p className="warn-text">
          Jailbreak execute requires <code>make plugins</code> <CopyButton text="make plugins" />
        </p>
      )}
      <div className="toolbar">
        <button type="button" onClick={refresh}>
          Refresh
        </button>
        <button
          type="button"
          disabled={agent.running || !health?.ok}
          onClick={() => agent.runTestConnection(selectedUdid ?? undefined)}
        >
          Test connection
        </button>
      </div>
      {agentDevices.length > 0 && (
        <ul className="granted-list">
          {agentDevices.map((d) => (
            <li key={d.udid}>
              <code className="mono">{d.udid}</code>
              <span className="muted"> · {d.state}</span>
            </li>
          ))}
        </ul>
      )}
    </>
  );

  return (
    <div className="page">
      <SplitPanel controls={controls} log={<StepLog lines={agent.log} />} />
    </div>
  );
}
