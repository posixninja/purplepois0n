import { Link } from "react-router-dom";
import { CopyButton } from "../components/CopyButton";
import { DeviceCard } from "../components/DeviceCard";
import { PageHeader } from "../components/PageHeader";
import { identityFromAgent, useDevice } from "../context/DeviceContext";
import { useWebUsbDevice } from "../hooks/useWebUsbDevice";
import { deviceLabel } from "../webusb/apple";

function friendlyState(state: string): string {
  switch (state) {
    case "dfu":
      return "Recovery";
    case "recovery":
      return "Recovery";
    case "normal":
      return "Normal";
    default:
      return state;
  }
}

export function DevicePage() {
  const {
    identity,
    agentOk,
    agentDevices,
    selectedUdid,
    setSelectedUdid,
    webUsbAvailable,
    clearDevice,
    refreshAgent,
  } = useDevice();
  const usb = useWebUsbDevice();

  const agentIdentity =
    selectedUdid && !identity
      ? identityFromAgent(agentDevices.find((d) => d.udid === selectedUdid)!)
      : null;

  return (
    <div className="page">
      <PageHeader
        title="Device"
        description="Connection status for your iPhone or iPad."
      />

      {!identity && agentDevices.length === 0 ? (
        <div className="empty-state">
          <p>No device connected</p>
          <p className="muted">Plug in your device, unlock it, and tap Trust on the device.</p>
          <div className="toolbar">
            <button type="button" disabled={!webUsbAvailable || usb.busy} onClick={usb.connect}>
              Connect device
            </button>
            <button type="button" onClick={refreshAgent}>
              Refresh
            </button>
            <Link to="/jailbreak" className="btn-primary btn-link">
              Jailbreak
            </Link>
          </div>
          {!agentOk && (
            <p className="muted">
              Host service offline — run <code>make agent</code> <CopyButton text="make agent" />
            </p>
          )}
        </div>
      ) : (
        <>
          <DeviceCard identity={identity ?? agentIdentity} />
          {agentDevices.length > 0 && (
            <div className="agent-devices">
              <h2>Connected devices</h2>
              <p className="muted">Select the device used for jailbreak and package install.</p>
              <ul className="device-select-list">
                {agentDevices.map((d) => (
                  <li key={d.udid}>
                    <label>
                      <input
                        type="radio"
                        name="device-udid"
                        checked={selectedUdid === d.udid}
                        onChange={() => setSelectedUdid(d.udid)}
                      />
                      <span>
                        <code className="mono">{d.udid}</code>
                        <span className="muted">
                          {" "}
                          · {friendlyState(d.state)}
                          {d.firmware ? ` · iOS ${d.firmware}` : ""}
                        </span>
                      </span>
                    </label>
                  </li>
                ))}
              </ul>
            </div>
          )}
          {usb.granted.length > 0 && (
            <div className="granted-devices">
              <h2>USB access granted</h2>
              <ul>
                {usb.granted.map((d) => (
                  <li key={`${d.vendorId}-${d.productId}-${d.serialNumber}`}>
                    <code>{deviceLabel(d)}</code>
                    <button type="button" disabled={usb.busy} onClick={() => usb.reconnect(d)}>
                      Reconnect
                    </button>
                  </li>
                ))}
              </ul>
            </div>
          )}
          <div className="toolbar">
            <button type="button" onClick={usb.disconnect}>
              Disconnect
            </button>
            <button type="button" onClick={clearDevice}>
              Clear
            </button>
            <Link to="/jailbreak" className="btn-primary btn-link">
              Jailbreak
            </Link>
          </div>
        </>
      )}
    </div>
  );
}
