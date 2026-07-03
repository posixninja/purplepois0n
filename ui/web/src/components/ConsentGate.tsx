import { type ReactNode, useState } from "react";

export function ConsentGate({
  label,
  children,
}: {
  label: string;
  children: (enabled: boolean) => ReactNode;
}) {
  const [consent, setConsent] = useState(false);

  return (
    <div className="consent-gate">
      <div className="consent-banner">
        <label>
          <input type="checkbox" checked={consent} onChange={(e) => setConsent(e.target.checked)} />
          {label}
        </label>
      </div>
      {children(consent)}
    </div>
  );
}
