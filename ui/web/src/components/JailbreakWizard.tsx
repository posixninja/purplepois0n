import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { identityFromAgent, useDevice } from "../context/DeviceContext";
import { useStore } from "../context/StoreContext";
import { fetchDevicePlan, storeInstall, storeSync, type DevicePlan } from "../lib/bridge";
import { useAgentStream } from "../hooks/useAgentStream";
import { useWebUsbDevice } from "../hooks/useWebUsbDevice";
import { ConsentGate } from "./ConsentGate";
import { DeviceCard } from "./DeviceCard";
import { DfuGuide } from "./DfuGuide";
import { StepLog } from "./StepLog";
import { CopyButton } from "./CopyButton";
import { WIZARD_STEPS } from "./wizardSteps";

function PlanSummary({ plan, loading, error }: { plan: DevicePlan | null; loading: boolean; error: string | null }) {
  if (loading) {
    return <p className="muted">Scanning device and selecting strategy…</p>;
  }
  if (error) {
    return <p className="warn-text">{error}</p>;
  }
  if (!plan) {
    return <p className="muted">Connect a device to see the recommended jailbreak path.</p>;
  }

  const blockers = plan.plan.blockers ?? [];

  return (
    <div className="plan-card">
      <h3>Recommended path</h3>
      <p className="plan-strategy">
        <code>{plan.plan.strategy}</code>
      </p>
      <p>{plan.plan.summary}</p>
      <ul className="plan-meta">
        {plan.device.soc && <li>SoC: {plan.device.soc}</li>}
        {plan.device.iosVersion && <li>iOS {plan.device.iosVersion}</li>}
        {plan.device.state && <li>Mode: {plan.device.state}</li>}
        {plan.plan.bootLane && plan.plan.bootLane !== "auto" && <li>Boot lane: {plan.plan.bootLane}</li>}
        {plan.plan.ramdiskSource && <li>Ramdisk: {plan.plan.ramdiskSource}</li>}
        {plan.plan.ipsw && (
          <li>
            IPSW: <code className="mono">{plan.plan.ipsw}</code>
          </li>
        )}
      </ul>
      {blockers.length > 0 && (
        <ul className="plan-blockers warn-text">
          {blockers.map((b) => (
            <li key={b}>{b}</li>
          ))}
        </ul>
      )}
      {plan.plan.canExecute ? (
        <p className="ok-text">Ready to execute on this device.</p>
      ) : (
        <p className="warn-text">Fix blockers above before jailbreaking.</p>
      )}
    </div>
  );
}

export function JailbreakWizard() {
  const {
    agentOk,
    pluginsEnabled,
    kpfBuilt,
    webUsbAvailable,
    identity,
    agentDevices,
    selectedUdid,
    setSelectedUdid,
    refreshAgent,
  } = useDevice();
  const { loadFromAgent, packages } = useStore();
  const usb = useWebUsbDevice();
  const agent = useAgentStream();
  const [step, setStep] = useState(0);
  const [dfuStep, setDfuStep] = useState(0);
  const [log, setLog] = useState<string[]>([]);
  const [postJbBusy, setPostJbBusy] = useState(false);
  const [devicePlan, setDevicePlan] = useState<DevicePlan | null>(null);
  const [planLoading, setPlanLoading] = useState(false);
  const [planError, setPlanError] = useState<string | null>(null);

  const append = (line: string) => setLog((p) => [...p, line]);

  const jailbreakUdid = selectedUdid ?? identity?.udid;
  const alreadyJailbroken = devicePlan?.plan.alreadyJailbroken === true;
  const canExecutePlan = devicePlan?.plan.canExecute === true;
  const needsRecoveryGuide = identity && (identity.mode === "dfu" || identity.mode === "unknown");

  useEffect(() => {
    usb.loadGranted();
    refreshAgent();
  }, [usb.loadGranted, refreshAgent]);

  useEffect(() => {
    if (identity && step < 3) setStep(3);
  }, [identity, step]);

  useEffect(() => {
    if (step === 5 && agentOk) {
      loadFromAgent().catch(() => {});
    }
  }, [step, agentOk, loadFromAgent]);

  useEffect(() => {
    if (step !== 4 || !agentOk) {
      return;
    }
    let cancelled = false;
    setPlanLoading(true);
    setPlanError(null);
    fetchDevicePlan(jailbreakUdid ?? undefined)
      .then((plan) => {
        if (!cancelled) setDevicePlan(plan);
      })
      .catch((e) => {
        if (!cancelled) {
          setDevicePlan(null);
          setPlanError(e instanceof Error ? e.message : String(e));
        }
      })
      .finally(() => {
        if (!cancelled) setPlanLoading(false);
      });
    return () => {
      cancelled = true;
    };
  }, [step, agentOk, jailbreakUdid]);

  const storeAppPkg =
    packages.find((p) => p.package === "purplepois0n-zebra")?.package ??
    packages.find((p) => p.package.includes("zebra"))?.package;

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
              <p className="muted">
                The host scans your device, picks a strategy, and runs the matching path automatically.
              </p>
              <PlanSummary plan={devicePlan} loading={planLoading} error={planError} />
              {!pluginsEnabled && agentOk && !alreadyJailbroken && (
                <p className="warn-text">
                  Run <code>make plugins</code> and restart the host service before jailbreaking on hardware.
                </p>
              )}
              {!kpfBuilt && agentOk && !alreadyJailbroken && devicePlan?.plan.useBootDelivery && (
                <p className="warn-text">
                  Boot module not built — run <code>make kpf</code> for DFU usb-loader paths.
                </p>
              )}
              <ConsentGate
                label={
                  alreadyJailbroken
                    ? "I have already jailbroken this device (Dopamine/palera1n)"
                    : "I understand this will modify my device and I have a backup"
                }
              >
                {(enabled) => (
                  <div className="toolbar">
                    <button
                      type="button"
                      className="btn-jailbreak"
                      disabled={
                        !enabled ||
                        !agentOk ||
                        agent.running ||
                        planLoading ||
                        (!alreadyJailbroken && (!pluginsEnabled || !canExecutePlan))
                      }
                      onClick={() =>
                        alreadyJailbroken
                          ? agent.runAlreadyJailbrokenProbe(jailbreakUdid)
                          : agent.runAutoJailbreak(jailbreakUdid)
                      }
                    >
                      {agent.running
                        ? "Working…"
                        : alreadyJailbroken
                          ? "Verify jailbreak"
                          : "Jailbreak"}
                    </button>
                    <button
                      type="button"
                      disabled={!agentOk || planLoading}
                      onClick={() => {
                        setPlanLoading(true);
                        setPlanError(null);
                        fetchDevicePlan(jailbreakUdid ?? undefined)
                          .then(setDevicePlan)
                          .catch((e) =>
                            setPlanError(e instanceof Error ? e.message : String(e)),
                          )
                          .finally(() => setPlanLoading(false));
                      }}
                    >
                      Rescan
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
              {!storeAppPkg && (
                <p className="muted note">
                  No store app in catalog — run <code>legacy/scripts/seed-store.sh</code> on the host.
                </p>
              )}
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
                        if (storeAppPkg) {
                          append(`Installing ${storeAppPkg}…`);
                          const detail = await storeInstall(selectedUdid, storeAppPkg);
                          append(detail);
                        }
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
                  {postJbBusy
                    ? "Working…"
                    : selectedUdid
                      ? storeAppPkg
                        ? "Sync & install store"
                        : "Sync & load packages"
                      : "Load packages"}
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
