import { useState } from "react";
import type { AptSource } from "../context/StoreContext";

interface SourceChipsProps {
  sources: AptSource[];
  activeId: string | null;
  onSelect: (id: string) => void;
  onRemove: (id: string) => void;
  onAdd: (url: string) => void;
}

export function SourceChips({ sources, activeId, onSelect, onRemove, onAdd }: SourceChipsProps) {
  const [showAdd, setShowAdd] = useState(false);
  const [url, setUrl] = useState("");

  const submit = () => {
    if (!url.trim()) return;
    onAdd(url.trim());
    setUrl("");
    setShowAdd(false);
  };

  return (
    <div className="source-chips">
      {sources.map((s) => (
        <span key={s.id} className={`chip ${activeId === s.id ? "active" : ""}`}>
          <button type="button" className="chip-label" onClick={() => onSelect(s.id)}>
            {s.label}
          </button>
          {!s.builtin && (
            <button type="button" className="chip-remove" onClick={() => onRemove(s.id)} aria-label="Remove">
              ×
            </button>
          )}
        </span>
      ))}
      {showAdd ? (
        <span className="chip-add-form">
          <input
            className="text-input"
            value={url}
            onChange={(e) => setUrl(e.target.value)}
            placeholder="Packages URL"
            onKeyDown={(e) => e.key === "Enter" && submit()}
          />
          <button type="button" onClick={submit}>
            Add
          </button>
          <button type="button" className="btn-ghost" onClick={() => setShowAdd(false)}>
            Cancel
          </button>
        </span>
      ) : (
        <button type="button" className="chip chip-add" onClick={() => setShowAdd(true)}>
          + Add source
        </button>
      )}
    </div>
  );
}
