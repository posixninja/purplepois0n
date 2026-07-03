import { Link } from "react-router-dom";

export function Breadcrumb({ section }: { section: string }) {
  return (
    <nav className="breadcrumb" aria-label="Breadcrumb">
      <Link to="/store">Home</Link>
      <span className="sep">›</span>
      <span>Tools</span>
      <span className="sep">›</span>
      <span>{section}</span>
    </nav>
  );
}
