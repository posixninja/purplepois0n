import { useState } from "react";
import { Breadcrumb } from "../components/Breadcrumb";
import { SplitPanel } from "../components/SplitPanel";
import { StepLog } from "../components/StepLog";
import { isPongoDevice, openPongo, pongoExec, pongoProbe, requestPongoDevice } from "../webusb/pongo";
import { webUsbSupported } from "../webusb/apple";

export function PongoPage() {
  const [log, setLog] = useState<string[]>([]);
  const [device, setDevice] = useState<USBDevice | null>(null);
  const [cmd, setCmd] = useState("help");

  const append = (line: string) => setLog((p) => [...p, line]);

  const connect = async () => {
    if (!webUsbSupported()) {
      append("USB unavailable in this browser");
      return;
    }
    try {
      const dev = await requestPongoDevice();
      await openPongo(dev);
      setDevice(dev);
      append(`Connected: ${dev.productName ?? "device"}`);
      if (!isPongoDevice(dev)) append("Unexpected device type");
      try {
        const pong = await pongoProbe(dev);
        if (pong) append(pong.trim());
      } catch {
        append("No response");
      }
    } catch (e) {
      append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
    }
  };

  const runCmd = async () => {
    if (!device) return;
    try {
      append(`> ${cmd}`);
      await pongoExec(device, cmd);
      append("(sent)");
    } catch (e) {
      append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
    }
  };

  const controls = (
    <>
      <Breadcrumb section="Boot console" />
      <h1>Boot console</h1>
      <p className="lead">Advanced — low-level boot shell over USB.</p>
      <div className="toolbar">
        <button type="button" onClick={connect}>
          Connect
        </button>
      </div>
      <div className="repl-bar">
        <span className="repl-prompt">&gt;</span>
        <input
          value={cmd}
          onChange={(e) => setCmd(e.target.value)}
          onKeyDown={(e) => e.key === "Enter" && runCmd()}
          className="text-input repl-input"
          disabled={!device}
          placeholder="help"
        />
        <button type="button" disabled={!device} onClick={runCmd}>
          Send
        </button>
      </div>
    </>
  );

  return (
    <div className="page">
      <SplitPanel controls={controls} log={<StepLog lines={log} />} />
    </div>
  );
}
