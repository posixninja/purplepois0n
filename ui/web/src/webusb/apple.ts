/** Apple USB vendor ID and common recovery/DFU product IDs. */
export const APPLE_VENDOR_ID = 0x05ac;

/** PongoOS USB product ID (post-checkm8). */
export const PONGO_PRODUCT_ID = 0x4141;

/** DFU / Recovery filters for WebUSB requestDevice. */
export const APPLE_RECOVERY_FILTERS: USBDeviceFilter[] = [
  { vendorId: APPLE_VENDOR_ID },
];

export function webUsbSupported(): boolean {
  return typeof navigator !== "undefined" && "usb" in navigator;
}

export async function requestAppleDevice(): Promise<USBDevice> {
  if (!webUsbSupported()) {
    throw new Error("WebUSB is not available (use Chrome or Edge over HTTPS/localhost)");
  }
  const device = await navigator.usb.requestDevice({ filters: APPLE_RECOVERY_FILTERS });
  return device;
}

/** Devices the user already granted — no picker required. */
export async function listGrantedAppleDevices(): Promise<USBDevice[]> {
  if (!webUsbSupported()) return [];
  const all = await navigator.usb.getDevices();
  return all.filter((d) => d.vendorId === APPLE_VENDOR_ID);
}

export async function openAppleDevice(device: USBDevice): Promise<void> {
  if (!device.opened) {
    await device.open();
  }
  if (device.configuration === null) {
    await device.selectConfiguration(1);
  }
  const iface = device.configuration?.interfaces[0];
  if (!iface) {
    throw new Error("No USB interface found");
  }
  const alt = iface.alternates[0];
  if (!alt) {
    throw new Error("No USB alternate setting");
  }
  try {
    await device.claimInterface(iface.interfaceNumber);
  } catch {
    // May already be claimed after reconnect.
  }
}

export function deviceLabel(device: USBDevice): string {
  const pid = device.productId.toString(16).padStart(4, "0");
  return `${device.productName ?? "Apple device"} [${pid}]`;
}

export function parseCpidFromSerial(serial: string): number | null {
  const m = serial.match(/CPID:([0-9a-fA-F]+)/);
  if (!m) return null;
  return parseInt(m[1], 16);
}

export function parseEcidFromSerial(serial: string): bigint | null {
  const m = serial.match(/ECID:([0-9a-fA-F]+)/);
  if (!m) return null;
  return BigInt("0x" + m[1]);
}

export function serialIndicatesPwned(serial: string): boolean {
  return /PWND:\s*\[checkm8\]/i.test(serial) || serial.includes("PWND");
}

const SUPPORTED_CPIDS = new Set([
  0x8940, 0x8942, 0x8945, 0x8947,
  0x8950, 0x8955,
  0x8960,
  0x7000, 0x7001, 0x7002,
  0x8000, 0x8001, 0x8002, 0x8003, 0x8004,
  0x8010, 0x8011, 0x8012, 0x8015,
]);

const UNSUPPORTED_CPIDS = new Set([
  0x8020, 0x8030, 0x8101, 0x8110, 0x8120, 0x8140, 0x8142, 0x8143,
  0x8201, 0x8210, 0x8211, 0x8212, 0x8220,
]);

const CPID_NAMES: Record<number, string> = {
  0x8940: "A5",
  0x8942: "A5",
  0x8945: "A5",
  0x8947: "A5",
  0x8950: "A6",
  0x8955: "A6",
  0x8960: "A7",
  0x8010: "A10",
  0x8011: "A10",
  0x8012: "A10",
  0x8015: "A11",
};

export function cpidSupport(cpid: number): "supported" | "unsupported" | "unknown" {
  if (SUPPORTED_CPIDS.has(cpid)) return "supported";
  if (UNSUPPORTED_CPIDS.has(cpid)) return "unsupported";
  return "unknown";
}

export function cpidToSocName(cpid: number): string {
  return CPID_NAMES[cpid] ?? `CPID 0x${cpid.toString(16)}`;
}

export type DeviceMode = "dfu" | "recovery" | "pongo" | "unknown";

export function guessMode(device: USBDevice): DeviceMode {
  if (device.productId === PONGO_PRODUCT_ID) return "pongo";
  const serial = device.serialNumber ?? "";
  if (serial.includes("Recovery") || device.productId === 0x1281) return "recovery";
  if (serial.includes("DFU") || device.productId === 0x1227) return "dfu";
  return "unknown";
}
