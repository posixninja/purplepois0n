import { Link } from "react-router-dom";

interface LogoProps {
  compact?: boolean;
  className?: string;
}

/** Brand mark — replace ui/web/public/logo.svg with your official asset. */
export function Logo({ compact = false, className = "" }: LogoProps) {
  return (
    <Link to="/store" className={`brand-lockup ${className}`.trim()} aria-label="purplepois0n home">
      <img src="/logo.svg" alt="" className="brand-mark" width={compact ? 28 : 32} height={compact ? 28 : 32} />
      {!compact && (
        <span className="brand-wordmark">
          purple<span className="brand-accent">pois0n</span>
        </span>
      )}
    </Link>
  );
}
