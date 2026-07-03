import { useEffect, useState } from "react";
import { DFU_STEPS } from "../webusb/irecv";

const STEP_DURATIONS_SEC = [0, 2, 10, 15, 0];

export function DfuGuide({ step, autoTimer = false }: { step: number; autoTimer?: boolean }) {
  const [remaining, setRemaining] = useState(0);

  useEffect(() => {
    if (!autoTimer || step >= DFU_STEPS.length - 1) {
      setRemaining(0);
      return;
    }
    const duration = STEP_DURATIONS_SEC[step] ?? 0;
    if (duration <= 0) return;
    setRemaining(duration);
    const id = window.setInterval(() => {
      setRemaining((r) => {
        if (r <= 1) {
          window.clearInterval(id);
          return 0;
        }
        return r - 1;
      });
    }, 1000);
    return () => window.clearInterval(id);
  }, [step, autoTimer]);

  return (
    <ol className="dfu-steps">
      {DFU_STEPS.map((text, i) => (
        <li key={text} className={i === step ? "active" : i < step ? "done" : ""}>
          {text}
          {autoTimer && i === step && remaining > 0 && (
            <span className="dfu-timer"> — {remaining}s</span>
          )}
        </li>
      ))}
    </ol>
  );
}
