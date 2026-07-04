import { useDevice } from "../context/DeviceContext";

const ITEMS = [
  { key: "usb", label: "USB", ok: (d: ReturnType<typeof useDevice>) => d.webUsbAvailable },
  { key: "host", label: "Host", ok: (d: ReturnType<typeof useDevice>) => d.agentOk },
  { key: "device", label: "Device", ok: (d: ReturnType<typeof useDevice>) => d.hasDevice },
] as const;

export function StatusPills() {
  const ctx = useDevice();

  return (
    <div className="status-pills" role="status" aria-label="Connection status">
      {ITEMS.map(({ key, label, ok }) => {
        const active = ok(ctx);
        return (
          <span
            key={key}
            className={`pill ${active ? "ok" : key === "usb" && !ctx.webUsbAvailable ? "warn" : "off"}`}
            title={label}
          >
            <span className="pill-dot" aria-hidden />
            {label}
          </span>
        );
      })}
    </div>
  );
}
