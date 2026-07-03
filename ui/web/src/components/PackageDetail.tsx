import { useState } from "react";
import { Link } from "react-router-dom";
import { useDevice } from "../context/DeviceContext";
import { storeInstall, storeSync } from "../lib/bridge";
import type { AptPackage } from "../lib/packages";

interface PackageDetailProps {
  pkg: AptPackage;
  onClose: () => void;
}

export function PackageDetail({ pkg, onClose }: PackageDetailProps) {
  const { agentOk, selectedUdid, selectedDevice } = useDevice();
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState("");

  const canInstall =
    agentOk && !!selectedUdid && (selectedDevice?.state === "normal" || !selectedDevice?.state);

  const handleInstall = async () => {
    if (!selectedUdid) return;
    setBusy(true);
    setStatus("");
    try {
      setStatus("Syncing store to device…");
      await storeSync(selectedUdid);
      setStatus(`Installing ${pkg.package}…`);
      const detail = await storeInstall(selectedUdid, pkg.package);
      setStatus(detail);
    } catch (e) {
      setStatus(e instanceof Error ? e.message : String(e));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="package-detail-overlay" onClick={onClose} role="presentation">
      <div className="package-detail" onClick={(e) => e.stopPropagation()} role="dialog" aria-modal>
        <button type="button" className="detail-close" onClick={onClose} aria-label="Close">
          ×
        </button>
        <div className="package-detail-header">
          <div className="package-icon large">{pkg.package.charAt(0).toUpperCase()}</div>
          <div>
            <h2>{pkg.package}</h2>
            <p className="package-meta">
              {pkg.version}
              {pkg.architecture ? ` · ${pkg.architecture}` : ""}
            </p>
          </div>
        </div>
        <p className="package-detail-desc">{pkg.description}</p>
        <dl className="detail-fields">
          {pkg.section && (
            <>
              <dt>Section</dt>
              <dd>{pkg.section}</dd>
            </>
          )}
          {pkg.maintainer && (
            <>
              <dt>Maintainer</dt>
              <dd>{pkg.maintainer}</dd>
            </>
          )}
          {pkg.depends && (
            <>
              <dt>Depends</dt>
              <dd>{pkg.depends}</dd>
            </>
          )}
          {pkg.filename && (
            <>
              <dt>Filename</dt>
              <dd className="mono">{pkg.filename}</dd>
            </>
          )}
          {pkg.sha256 && (
            <>
              <dt>SHA256</dt>
              <dd className="mono">{pkg.sha256}</dd>
            </>
          )}
          {pkg.size > 0 && (
            <>
              <dt>Size</dt>
              <dd>{pkg.size} bytes</dd>
            </>
          )}
        </dl>
        <div className="detail-actions">
          {canInstall ? (
            <button type="button" className="btn-primary" disabled={busy} onClick={handleInstall}>
              {busy ? "Installing…" : "Install"}
            </button>
          ) : !selectedUdid ? (
            <Link to="/device" className="btn-primary btn-link">
              Select device
            </Link>
          ) : selectedDevice?.state && selectedDevice.state !== "normal" ? (
            <Link to="/jailbreak" className="btn-primary btn-link">
              Jailbreak required
            </Link>
          ) : (
            <button type="button" className="btn-primary" disabled title="Start host service">
              Install
            </button>
          )}
          {status && <p className="muted note">{status}</p>}
          {!status && (
            <p className="muted note">
              Installs over SSH to your jailbroken device. Unlock the device and trust this computer.
            </p>
          )}
        </div>
      </div>
    </div>
  );
}
