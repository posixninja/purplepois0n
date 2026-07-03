import { useEffect, useState } from "react";
import { Breadcrumb } from "../components/Breadcrumb";
import { DeviceCard } from "../components/DeviceCard";
import { DfuGuide } from "../components/DfuGuide";
import { SplitPanel } from "../components/SplitPanel";
import { StepLog } from "../components/StepLog";
import { useDevice } from "../context/DeviceContext";
import { useWebUsbDevice } from "../hooks/useWebUsbDevice";
import { deviceLabel } from "../webusb/apple";

export function DfuPage() {
  const { identity } = useDevice();
  const usb = useWebUsbDevice();
  const [dfuStep, setDfuStep] = useState(0);

  useEffect(() => {
    usb.loadGranted();
  }, [usb.loadGranted]);

  const controls = (
    <>
      <Breadcrumb section="USB" />
      <h1>USB connection</h1>
      <p className="lead">Advanced — direct USB diagnostics.</p>
      <DeviceCard identity={identity} />
      <DfuGuide step={dfuStep} />
      <div className="toolbar">
        <button type="button" disabled={usb.busy} onClick={usb.connect}>
          Connect device
        </button>
        <button type="button" disabled={!usb.device} onClick={usb.disconnect}>
          Disconnect
        </button>
        <button type="button" onClick={() => setDfuStep((s) => Math.min(s + 1, 4))}>
          Next recovery step
        </button>
      </div>
      {usb.granted.length > 0 && (
        <ul className="granted-list">
          {usb.granted.map((d) => (
            <li key={`${d.vendorId}-${d.productId}-${d.serialNumber}`}>
              <button type="button" disabled={usb.busy} onClick={() => usb.reconnect(d)}>
                {deviceLabel(d)}
              </button>
            </li>
          ))}
        </ul>
      )}
    </>
  );

  return (
    <div className="page">
      <SplitPanel controls={controls} log={<StepLog lines={usb.log} />} />
    </div>
  );
}
