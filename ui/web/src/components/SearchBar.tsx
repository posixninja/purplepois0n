interface SearchBarProps {
  value: string;
  onChange: (v: string) => void;
  onSubmit?: () => void;
  placeholder?: string;
}

export function SearchBar({ value, onChange, onSubmit, placeholder = "Search packages…" }: SearchBarProps) {
  return (
    <input
      type="search"
      className="search-bar"
      value={value}
      onChange={(e) => onChange(e.target.value)}
      onKeyDown={(e) => e.key === "Enter" && onSubmit?.()}
      placeholder={placeholder}
      aria-label="Search packages"
    />
  );
}
