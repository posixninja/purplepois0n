/** PongoOS WebUSB client (05ac:4141) — minimal shell commands. */

import { APPLE_VENDOR_ID, PONGO_PRODUCT_ID } from "./apple";

export async function requestPongoDevice(): Promise<USBDevice> {
  if (!("usb" in navigator)) throw new Error("WebUSB unavailable");
  return navigator.usb.requestDevice({
    filters: [{ vendorId: APPLE_VENDOR_ID, productId: PONGO_PRODUCT_ID }],
  });
}

export async function openPongo(device: USBDevice): Promise<void> {
  if (!device.opened) await device.open();
  if (device.configuration === null) await device.selectConfiguration(1);
  const iface = device.configuration!.interfaces[0];
  await device.claimInterface(iface.interfaceNumber);
}

async function pongoControlOut(device: USBDevice, request: number, data: BufferSource): Promise<void> {
  const r = await device.controlTransferOut(
    { requestType: "class", recipient: "interface", request, value: 0, index: 0 },
    data,
  );
  if (r.status !== "ok") throw new Error(`Pongo control out: ${r.status}`);
}

export async function pongoExec(device: USBDevice, command: string): Promise<void> {
  const line = new TextEncoder().encode(command + "\n");
  await pongoControlOut(device, 3, line);
}

export async function pongoProbe(device: USBDevice): Promise<string> {
  await pongoExec(device, "echo pong");
  const r = await device.controlTransferIn(
    { requestType: "class", recipient: "interface", request: 2, value: 0, index: 0 },
    256,
  );
  if (!r.data) return "";
  const nul = r.data.getUint8(0);
  return new TextDecoder().decode(r.data.buffer.slice(0, Math.min(256, nul || 256)));
}

export function isPongoDevice(device: USBDevice): boolean {
  return device.vendorId === APPLE_VENDOR_ID && device.productId === PONGO_PRODUCT_ID;
}
