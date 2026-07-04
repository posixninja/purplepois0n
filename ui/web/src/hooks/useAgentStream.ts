import { useCallback, useState } from "react";
import { runDoctor, runJailbreak, type AgentEvent } from "../lib/bridge";

export function useAgentStream() {
  const [log, setLog] = useState<string[]>([]);
  const [running, setRunning] = useState(false);

  const append = useCallback((line: string) => setLog((p) => [...p, line]), []);

  const onProgressEvent = useCallback(
    (ev: AgentEvent) => {
      if (ev.type === "step") {
        const arrow = ev.phase === "request" ? "→" : "←";
        const okMark =
          ev.phase === "response" && ev.success !== undefined ? (ev.success ? " OK" : " FAIL") : "";
        append(`  ${arrow} ${ev.id ?? "?"}${okMark}: ${ev.detail ?? ""}`);
      } else if (ev.type === "syringe") {
        append(`    ${ev.command ?? ev.detail ?? ""}`);
      } else if (ev.type === "complete") {
        append(ev.success ? "Done." : `Failed — ${ev.detail ?? "see log"}`);
      } else if (ev.type === "log" && ev.detail) {
        append(ev.detail);
      }
    },
    [append],
  );

  const run = useCallback(
    async (
      startMessage: string,
      doneMessage: string,
      fn: (onEvent: (e: AgentEvent) => void) => Promise<boolean>,
    ) => {
      setRunning(true);
      setLog([]);
      append(startMessage);
      try {
        const ok = await fn(onProgressEvent);
        if (ok) append(doneMessage);
        else append("Something went wrong. Check the log and try again.");
        return ok;
      } catch (e) {
        append(`ERROR: ${e instanceof Error ? e.message : String(e)}`);
        return false;
      } finally {
        setRunning(false);
      }
    },
    [append, onProgressEvent],
  );

  const runAutoJailbreak = useCallback(
    (udid?: string) =>
      run("Scanning device and running jailbreak plan…", "Jailbreak flow complete.", (cb) =>
        runJailbreak({ auto: true, execute: true, udid }, cb),
      ),
    [run],
  );

  const runJailbreakFlow = useCallback(
    (_mode?: string, udid?: string) => runAutoJailbreak(udid),
    [runAutoJailbreak],
  );

  const runAlreadyJailbrokenProbe = useCallback(
    (udid?: string) =>
      run("Verifying jailbreak…", "Device is jailbroken.", (cb) =>
        runJailbreak({ auto: true, execute: true, udid }, cb),
      ),
    [run],
  );

  const runTestConnection = useCallback(
    (udid?: string) =>
      run("Testing connection…", "Connection test complete.", (cb) => runDoctor(false, cb, udid)),
    [run],
  );

  return {
    log,
    running,
    append,
    clearLog: () => setLog([]),
    runAutoJailbreak,
    runJailbreak: runJailbreakFlow,
    runAlreadyJailbrokenProbe,
    runTestConnection,
  };
}
