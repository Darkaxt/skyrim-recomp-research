"""Pinned, unmodified pcrecomp lifting; all generated/game-derived files on E:."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from run_offline_validation import ROOT, EXE, EXPECTED

UPSTREAM = ROOT / "local/sources/pcrecomp"
COMMIT = "0a138b6ba423af7dd1d6955fb763dc2bfd2b058e"
DEPS = ROOT / "local/tools/translation-python"
OUTPUT = ROOT / "local/automatic"
RANGES = {
    "projection": [(0xEF1180, 0xEF12FC)],
    "dispatch": [(0x447180, 0x4471F7)],
    "frustum": [(0x224660, 0x2247D3), (0xEF7430, 0xEF7465),
                (0xEF74A0, 0xEF74E8), (0xEF76F0, 0xEF7700),
                (0xEF7710, 0xEF7CED)],
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def environment():
    env = os.environ.copy()
    env.update(TEMP=str(ROOT / "local/tmp"), TMP=str(ROOT / "local/tmp"),
               PYTHONDONTWRITEBYTECODE="1", PYTHONPATH=str(DEPS))
    return env


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("stage", choices=RANGES)
    args = parser.parse_args()
    if ROOT.drive.upper() != "E:" or sha(EXE) != EXPECTED:
        raise RuntimeError("Workspace or input identity mismatch")
    actual = subprocess.check_output(["git", "-C", str(UPSTREAM), "rev-parse", "HEAD"],
                                     text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(UPSTREAM), "status", "--porcelain"], text=True)
    if actual != COMMIT or dirty:
        raise RuntimeError("Lifter checkout identity changed")
    OUTPUT.mkdir(exist_ok=True)
    catalog = OUTPUT / f"{args.stage}-catalog.txt"
    catalog.write_text("".join(f"0x{0x140000000 + a:x} 0x{b-a:x} slice_{a:x}\n"
                               for a, b in RANGES[args.stage]), encoding="ascii")
    generated = OUTPUT / f"{args.stage}-stock.c"
    command = [sys.executable, str(UPSTREAM / "tools/lift/lift64_cpu.py"),
               str(EXE), str(catalog), str(generated)]
    started = time.perf_counter()
    result = subprocess.run(command, cwd=OUTPUT, env=environment(), capture_output=True, text=True)
    emitted = generated.read_text() if generated.exists() else ""
    source_files = [UPSTREAM / p for p in ("tools/lift/lift64_cpu.py", "tools/lift/lift32_cpu.py",
                                          "runtime/recomp64_cpu/cpu64.h", "LICENSE")]
    evidence = dict(tool="pcrecomp", commit=COMMIT, capstone="5.0.7", input_sha256=EXPECTED,
                    command=command, exit_code=result.returncode, stdout=result.stdout,
                    stderr=result.stderr, generation_seconds=time.perf_counter() - started,
                    generated_sha256=sha(generated) if generated.exists() else None,
                    todo_count=emitted.count("RECOMP_TODO("),
                    source_sha256={str(p.relative_to(ROOT)): sha(p) for p in source_files},
                    driver_sha256=sha(__file__), catalog_sha256=sha(catalog))
    (OUTPUT / f"{args.stage}-lift.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(evidence, indent=2))
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
