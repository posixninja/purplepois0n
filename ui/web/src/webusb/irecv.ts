/**
 * irecv protocol subset for WebUSB (libirecovery-compatible control transfers).
 * Reference: libimobiledevice/libirecovery src/libirecovery.c
 */

function encAscii(text: string): Uint8Array {
  return new TextEncoder().encode(text);
}

async function controlOut(
  device: USBDevice,
  request: number,
  value: number,
  index: number,
  data: BufferSource,
): Promise<void> {
  const result = await device.controlTransferOut(
    { requestType: "vendor", recipient: "device", request, value, index },
    data,
  );
  if (result.status !== "ok") {
    throw new Error(`controlTransferOut failed: ${result.status}`);
  }
}

async function controlIn(
  device: USBDevice,
  request: number,
  value: number,
  index: number,
  length: number,
): Promise<Uint8Array> {
  const result = await device.controlTransferIn(
    { requestType: "vendor", recipient: "device", request, value, index },
    length,
  );
  if (result.status !== "ok" || !result.data) {
    throw new Error(`controlTransferIn failed: ${result.status}`);
  }
  return new Uint8Array(result.data.buffer);
}

/** irecv_send_command_raw — bmRequestType 0x40, bRequest 0 */
export async function sendCommand(device: USBDevice, command: string): Promise<void> {
  const payload = encAscii(command + "\0");
  if (payload.length >= 0x100) {
    throw new Error("command too long");
  }
  await controlOut(device, 0, 0, 0, payload);
}

/** Read response buffer after command (getenv / getret path). */
export async function readResponse(device: USBDevice, size = 255): Promise<string> {
  const data = await controlIn(device, 0, 0, 0, size);
  const nul = data.indexOf(0);
  const slice = nul >= 0 ? data.subarray(0, nul) : data;
  return new TextDecoder().decode(slice).trim();
}

export async function getenv(device: USBDevice, variable: string): Promise<string> {
  await sendCommand(device, `getenv ${variable}`);
  return readResponse(device);
}

export async function getret(device: USBDevice): Promise<number> {
  await sendCommand(device, "");
  const raw = await readResponse(device, 1);
  return raw.charCodeAt(0) || 0;
}

export function usbMemoryAddressFields(address: number): { value: number; index: number } {
  return {
    value: address & 0xffff,
    index: (address >> 16) & 0xffff,
  };
}

/** irecv_usb_control_transfer 0xC0 memory read (DFU bootrom). */
export async function usbMemoryRead(
  device: USBDevice,
  address: number,
  length: number,
): Promise<Uint8Array> {
  if (length > 0xffff) throw new Error("read too large");
  const { value, index } = usbMemoryAddressFields(address);
  return controlIn(device, 0, value, index, length);
}

/** irecv_usb_control_transfer 0x40 memory write. */
export async function usbMemoryWrite(
  device: USBDevice,
  address: number,
  data: Uint8Array,
): Promise<void> {
  if (data.length > 0xffff) throw new Error("write too large");
  const { value, index } = usbMemoryAddressFields(address);
  await controlOut(device, 0, value, index, data);
}

export interface IRecvIdentity {
  cpid: number;
  ecid: bigint | null;
  serial: string;
  cpidFromEnv?: string;
  ecidFromEnv?: string;
}

export async function identifyDevice(
  device: USBDevice,
  serialFallback: string,
): Promise<IRecvIdentity> {
  let cpid = 0;
  let ecid: bigint | null = null;
  let cpidFromEnv: string | undefined;
  let ecidFromEnv: string | undefined;

  try {
    cpidFromEnv = await getenv(device, "cpid");
    const parsed = parseInt(cpidFromEnv.replace(/^0x/i, ""), 16);
    if (!Number.isNaN(parsed)) cpid = parsed;
  } catch {
    // DFU may not answer getenv until pwned — fall back to serial.
  }

  try {
    ecidFromEnv = await getenv(device, "ecid");
    ecid = BigInt(ecidFromEnv);
  } catch {
    // optional
  }

  const serial = serialFallback || device.serialNumber || "";
  if (!cpid && serial) {
    const m = serial.match(/CPID:([0-9a-fA-F]+)/);
    if (m) cpid = parseInt(m[1], 16);
  }
  if (ecid === null && serial) {
    const m = serial.match(/ECID:([0-9a-fA-F]+)/);
    if (m) ecid = BigInt("0x" + m[1]);
  }

  return { cpid, ecid, serial, cpidFromEnv, ecidFromEnv };
}

/** Small DFU sanity probe — read 4 bytes from a low address (may fail on some SoCs). */
export async function probeDfuRead(device: USBDevice): Promise<boolean> {
  try {
    await usbMemoryRead(device, 0x100, 4);
    return true;
  } catch {
    return false;
  }
}

export const DFU_STEPS = [
  "Get ready for DFU",
  "Press and hold sleep (2 sec)",
  "Hold sleep + press and hold home (10 sec)",
  "Release sleep; keep holding home (15 sec)",
  "Device screen should stay black — connect USB",
];
