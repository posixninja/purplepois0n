import type { DeviceIdentity } from "../context/DeviceContext";
import { cpidToSocName } from "../webusb/apple";

function friendlyMode(mode: string): string {
  switch (mode) {
    case "dfu":
      return "Recovery";
    case "recovery":
      return "Recovery";
    case "pongo":
      return "Boot";
    case "unknown":
      return "USB";
    default:
      return mode;
  }
}

function friendlySource(source: string): string {
  return source === "webusb" ? "USB" : "Host";
}

function compatibilityLabel(identity: DeviceIdentity): string {
  if (identity.pwned) return "Ready";
  if (identity.support === "supported") return "Supported";
  if (identity.support === "unsupported") return "Not supported";
  return "Unknown";
}

export function DeviceCard({ identity }: { identity: DeviceIdentity | null }) {
  if (!identity) {
    return (
      <div className="device-card empty">
        <p className="muted">No device connected</p>
      </div>
    );
  }

  return (
    <div className="device-card">
      <h3>{identity.label}</h3>
      <dl className="device-fields">
        <div>
          <dt>Connection</dt>
          <dd>{friendlySource(identity.source)}</dd>
        </div>
        <div>
          <dt>State</dt>
          <dd>{friendlyMode(identity.mode)}</dd>
        </div>
        <div>
          <dt>Chip</dt>
          <dd className="mono">{cpidToSocName(identity.cpid)}</dd>
        </div>
        <div>
          <dt>ECID</dt>
          <dd className="mono">{identity.ecid}</dd>
        </div>
        <div>
          <dt>Compatibility</dt>
          <dd className={identity.support === "supported" || identity.pwned ? "ok" : "warn"}>
            {compatibilityLabel(identity)}
          </dd>
        </div>
        {identity.udid && (
          <div>
            <dt>Device ID</dt>
            <dd className="mono">{identity.udid}</dd>
          </div>
        )}
      </dl>
    </div>
  );
}
