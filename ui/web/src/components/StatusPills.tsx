import { useDevice } from "../context/DeviceContext";

export function StatusPills() {
  const { webUsbAvailable, agentOk, hasDevice } = useDevice();

  return (
    <div className="status-pills">
      <span className={`pill ${webUsbAvailable ? "ok" : "warn"}`} title="USB in browser">
        USB
      </span>
      <span className={`pill ${agentOk ? "ok" : "off"}`} title="Host service">
        Host
      </span>
      <span className={`pill ${hasDevice ? "ok" : "off"}`} title="Device connected">
        Device
      </span>
    </div>
  );
}
