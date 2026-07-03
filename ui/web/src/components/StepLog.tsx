import { useEffect, useRef } from "react";

export function StepLog({ lines }: { lines: string[] }) {
  const end = useRef<HTMLDivElement>(null);
  useEffect(() => {
    end.current?.scrollIntoView({ behavior: "smooth" });
  }, [lines.length]);

  return (
    <pre className="log-panel">
      {lines.length === 0 ? <span className="muted">No output yet.</span> : lines.join("\n")}
      <div ref={end} />
    </pre>
  );
}
