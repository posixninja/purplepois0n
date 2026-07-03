export function CopyButton({ text, label = "Copy" }: { text: string; label?: string }) {
  const copy = async () => {
    try {
      await navigator.clipboard.writeText(text);
    } catch {
      /* ignore */
    }
  };
  return (
    <button type="button" className="btn-ghost" onClick={copy}>
      {label}
    </button>
  );
}
