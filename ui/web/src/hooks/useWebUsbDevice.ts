import { useCallback, useState } from "react";
import { identityFromUsb, useDevice } from "../context/DeviceContext";
import {
  deviceLabel,
  guessMode,
  listGrantedAppleDevices,
  openAppleDevice,
  parseCpidFromSerial,
  parseEcidFromSerial,
  requestAppleDevice,
  webUsbSupported,
} from "../webusb/apple";
import { identifyDevice, probeDfuRead } from "../webusb/irecv";

export function useWebUsbDevice() {
  const { setIdentity } = useDevice();
  const [device, setDevice] = useState<USBDevice | null>(null);
  const [granted, setGranted] = useState<USBDevice[]>([]);
  const [busy, setBusy] = useState(false);
  const [log, setLog] = useState<string[]>([]);

  const append = useCallback((line: string) => setLog((p) => [...p, line]), []);

  const attachDevice = useCallback(
    async (dev: USBDevice) => {
      append(`Opening: ${deviceLabel(dev)}`);
      await openAppleDevice(dev);
      setDevice(dev);
      const mode = guessMode(dev);
      const serial = dev.serialNumber ?? "";
      const id = await identifyDevice(dev, serial);
      const cpid = id.cpid || parseCpidFromSerial(serial) || 0;
      const ecid = id.ecid ?? parseEcidFromSerial(serial);
      const ident = identityFromUsb(serial, cpid, ecid, mode, deviceLabel(dev));
      setIdentity(ident);
      append(`Compatibility: ${ident.support}`);
      if (mode === "dfu" || mode === "unknown") {
        const canRead = await probeDfuRead(dev);
        append(`DFU memory probe: ${canRead ? "ok" : "failed"}`);
      }
      return ident;
    },
    [append, setIdentity],
  );

  const connect = useCallback(async () => {
    if (!webUsbSupported()) {
      append("ERROR: WebUSB not available");
      return null;
    }
    setBusy(true);
    try {
      const dev = await requestAppleDevice();
      const ident = await attachDevice(dev);
      setGranted(await listGrantedAppleDevices());
      return ident;
    } catch (e) {
      append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
      return null;
    } finally {
      setBusy(false);
    }
  }, [append, attachDevice]);

  const reconnect = useCallback(
    async (dev: USBDevice) => {
      setBusy(true);
      try {
        return await attachDevice(dev);
      } catch (e) {
        append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
        return null;
      } finally {
        setBusy(false);
      }
    },
    [append, attachDevice],
  );

  const disconnect = useCallback(async () => {
    if (device?.opened) {
      try {
        await device.close();
      } catch {
        /* ignore */
      }
    }
    setDevice(null);
    setIdentity(null);
    append("Disconnected");
  }, [device, append, setIdentity]);

  const loadGranted = useCallback(async () => {
    if (webUsbSupported()) setGranted(await listGrantedAppleDevices());
  }, []);

  return {
    device,
    granted,
    busy,
    log,
    append,
    connect,
    reconnect,
    disconnect,
    loadGranted,
    clearLog: () => setLog([]),
  };
}
