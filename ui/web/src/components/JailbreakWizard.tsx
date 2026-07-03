import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { identityFromAgent, useDevice } from "../context/DeviceContext";
import { useStore } from "../context/StoreContext";
import { storeSync } from "../lib/bridge";
import { useAgentStream } from "../hooks/useAgentStream";
import { useWebUsbDevice } from "../hooks/useWebUsbDevice";
import { ConsentGate } from "./ConsentGate";
import { DeviceCard } from "./DeviceCard";
import { DfuGuide } from "./DfuGuide";
import { StepLog } from "./StepLog";
import { CopyButton } from "./CopyButton";
import { WIZARD_STEPS } from "./wizardSteps";

export function JailbreakWizard() {
  const {
    agentOk,
    pluginsEnabled,
    webUsbAvailable,
    identity,
    agentDevices,
    selectedUdid,
    setSelectedUdid,
    refreshAgent,
  } = useDevice();
  const { loadFromAgent } = useStore();
  const usb = useWebUsbDevice();
  const agent = useAgentStream();
  const [step, setStep] = useState(0);
  const [dfuStep, setDfuStep] = useState(0);
  const [log, setLog] = useState<string[]>([]);
  const [postJbBusy, setPostJbBusy] = useState(false);

  const append = (line: string) => setLog((p) => [...p, line]);

  useEffect(() => {
    usb.loadGranted();
    refreshAgent();
  }, [usb.loadGranted, refreshAgent]);

  useEffect(() => {
    if (identity && step < 3) setStep(3);
  }, [identity, step]);

  const needsRecoveryGuide = identity && (identity.mode === "dfu" || identity.mode === "unknown");
  const jailbreakUdid = selectedUdid ?? identity?.udid;

  return (
    <div className="wizard-layout">
      <div className="wizard-main">
        <div className="wizard-hero">
          <h1>Jailbreak</h1>
          <p className="lead">Connect your device and install packages from the store.</p>
        </div>

        <div className="wizard-steps" role="tablist">
          {WIZARD_STEPS.map((label, i) => (
            <button
              key={label}
              type="button"
              className={`wizard-dot ${i === step ? "active" : i < step ? "done" : ""}`}
              onClick={() => setStep(i)}
              title={label}
            >
              <span className="dot" />
              <span className="label">{label}</span>
            </button>
          ))}
        </div>

        <div className="wizard-card">
          {step === 0 && (
            <div className="wizard-panel">
              <h2>Before you start</h2>
              <ul className="checklist">
                <li className={webUsbAvailable ? "ok" : "warn"}>
                  Chrome or Edge {webUsbAvailable ? "✓" : "— required"}
                </li>
                <li className={agentOk ? "ok" : "warn"}>
                  Host service running{" "}
                  {agentOk ? "✓" : (
                    <>
                      — run <code>make agent</code> <CopyButton text="make agent" />
                    </>
                  )}
                </li>
                <li className={pluginsEnabled ? "ok" : "warn"}>
                  Jailbreak support {pluginsEnabled ? "✓" : (
                    <>
                      — run <code>make plugins</code> <CopyButton text="make plugins" />
                    </>
                  )}
                </li>
                <li>Quit Finder / Apple Mobile Device if connect fails</li>
              </ul>
              <button type="button" onClick={() => setStep(1)}>
                Continue
              </button>
            </div>
          )}

          {step === 1 && (
            <div className="wizard-panel">
              <h2>Connect device</h2>
              <p className="muted">Plug in your iPhone or iPad and allow USB access when prompted.</p>
              <div className="toolbar">
                <button type="button" disabled={usb.busy || !webUsbAvailable} onClick={usb.connect}>
                  Connect device
                </button>
                <button type="button" onClick={refreshAgent}>
                  Refresh
                </button>
              </div>
              {usb.granted.length > 0 && (
                <ul className="granted-list">
                  {usb.granted.map((d) => (
                    <li key={`${d.vendorId}-${d.productId}-${d.serialNumber}`}>
                      <button type="button" disabled={usb.busy} onClick={() => usb.reconnect(d)}>
                        Reconnect {d.serialNumber?.slice(0, 20) ?? "device"}
                      </button>
                    </li>
                  ))}
                </ul>
              )}
              {agentDevices.length > 0 && (
                <div className="agent-devices">
                  <p className="muted">Host sees {agentDevices.length} device(s)</p>
                  <ul className="device-select-list">
                    {agentDevices.map((d) => (
                      <li key={d.udid}>
                        <label>
                          <input
                            type="radio"
                            name="wizard-device"
                            checked={selectedUdid === d.udid}
                            onChange={() => setSelectedUdid(d.udid)}
                          />
                          <code className="mono">{d.udid}</code>
                          <span className="muted"> · {d.state}</span>
                        </label>
                      </li>
                    ))}
                  </ul>
                </div>
              )}
              <button type="button" onClick={() => setStep(identity || selectedUdid ? 3 : 2)}>
                Next
              </button>
            </div>
          )}

          {step === 2 && (
            <div className="wizard-panel">
              <h2>Your device</h2>
              <DeviceCard
                identity={
                  identity ??
                  (selectedUdid
                    ? identityFromAgent(agentDevices.find((d) => d.udid === selectedUdid)!)
                    : null)
                }
              />
              <button type="button" disabled={!identity && !selectedUdid} onClick={() => setStep(needsRecoveryGuide ? 3 : 4)}>
                Next
              </button>
            </div>
          )}

          {step === 3 && (
            <div className="wizard-panel">
              <h2>Recovery mode</h2>
              <p className="muted">Follow these steps if your screen stays black after connecting USB.</p>
              <DfuGuide step={dfuStep} autoTimer />
              <div className="toolbar">
                <button type="button" onClick={() => setDfuStep((s) => Math.min(s + 1, 4))}>
                  Next step
                </button>
                <button type="button" onClick={() => setStep(4)}>
                  Skip
                </button>
              </div>
            </div>
          )}

          {step === 4 && (
            <div className="wizard-panel">
              <h2>Jailbreak</h2>
              <p className="muted">This will modify your device. Make sure you have a backup.</p>
              {!pluginsEnabled && agentOk && (
                <p className="warn-text">
                  Run <code>make plugins</code> and restart the host service before jailbreaking on hardware.
                </p>
              )}
              <ConsentGate label="I understand this will modify my device and I have a backup">
                {(enabled) => (
                  <div className="toolbar">
                    <button
                      type="button"
                      className="btn-jailbreak"
                      disabled={!enabled || !agentOk || !pluginsEnabled || agent.running}
                      onClick={() => agent.runJailbreak(identity?.mode, jailbreakUdid)}
                    >
                      {agent.running ? "Working…" : "Jailbreak"}
                    </button>
                  </div>
                )}
              </ConsentGate>
              <StepLog lines={[...log, ...usb.log, ...agent.log]} />
              <button type="button" onClick={() => setStep(5)}>
                Continue
              </button>
            </div>
          )}

          {step === 5 && (
            <div className="wizard-panel">
              <h2>All set</h2>
              <p>Browse and install packages from the store.</p>
              <div className="toolbar">
                <button
                  type="button"
                  disabled={!agentOk || postJbBusy}
                  onClick={async () => {
                    setPostJbBusy(true);
                    try {
                      if (selectedUdid) {
                        append("Syncing store to device…");
                        await storeSync(selectedUdid);
                        append("Store synced to device.");
                      } else {
                        append("No device selected — loading host catalog only.");
                      }
                      append("Loading packages…");
                      await loadFromAgent();
                      append("Packages loaded.");
                    } catch (e) {
                      append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
                    } finally {
                      setPostJbBusy(false);
                    }
                  }}
                >
                  {postJbBusy ? "Working…" : selectedUdid ? "Sync & load packages" : "Load packages"}
                </button>
                <Link to="/store" className="btn-primary btn-link">
                  Open store
                </Link>
              </div>
              <StepLog lines={log} />
            </div>
          )}
        </div>
      </div>

      <aside className="wizard-sidebar">
        <DeviceCard
          identity={
            identity ??
            (selectedUdid
              ? identityFromAgent(agentDevices.find((d) => d.udid === selectedUdid)!)
              : null)
          }
        />
      </aside>
    </div>
  );
}
