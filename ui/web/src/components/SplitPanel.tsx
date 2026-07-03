import { type ReactNode } from "react";

export function SplitPanel({ controls, log }: { controls: ReactNode; log: ReactNode }) {
  return (
    <div className="split-panel">
      <div className="split-controls">{controls}</div>
      <div className="split-log">{log}</div>
    </div>
  );
}
