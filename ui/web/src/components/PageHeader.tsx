import type { ReactNode } from "react";

interface PageHeaderProps {
  title: string;
  description?: string;
  actions?: ReactNode;
  variant?: "default" | "hero";
}

export function PageHeader({ title, description, actions, variant = "default" }: PageHeaderProps) {
  return (
    <header className={`page-header ${variant === "hero" ? "page-header-hero" : ""}`}>
      <div className="page-header-text">
        <h1>{title}</h1>
        {description && <p className="lead">{description}</p>}
      </div>
      {actions && <div className="page-header-actions">{actions}</div>}
    </header>
  );
}
