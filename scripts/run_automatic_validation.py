"""Run generated semantics through a baseline harness with one base-address hook."""
import argparse
import json
from pathlib import Path
import subprocess
import time

from lift_automatic import ROOT, OUTPUT, UPSTREAM, EXE, RANGES, sha, environment
from run_offline_validation import COMPILER, prepare
from repair_automatic import repaired_lift
import pefile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--variant", choices=["stock", "compare", "strict"], default="stock")
    parser.add_argument("--stage", choices=["projection", "dispatch"], default="projection")
    args = parser.parse_args()
    if ROOT.drive.upper() != "E:":
        raise RuntimeError("Research outputs must stay on E:")
    stage = args.stage
    folder = OUTPUT / f"{stage}-{args.variant}"
    folder.mkdir(exist_ok=True)
    prepare(folder, stage)
    header = (UPSTREAM / "runtime/recomp64_cpu/cpu64.h").read_text()
    edits = []
    (folder / "cpu64.h").write_text(header, encoding="utf-8")
    if args.variant == "stock":
        generated = (OUTPUT / f"{stage}-stock.c").read_text()
    else:
        pe = pefile.PE(str(EXE), fast_load=True)
        generated = repaired_lift(stage, RANGES[stage], args.variant, pe)
        edits.append(dict(kind="semantic repair", scope="Generic scalar SSE comparisons",
                          implementation="validation/automatic_sse.cpp"))
        if args.variant == "strict":
            edits.append(dict(kind="semantic repair", scope="Generic scalar SSE arithmetic/min/max",
                              implementation="validation/automatic_sse.cpp"))
    (folder / "candidate-generated.cpp").write_text(generated, encoding="utf-8")
    harness_path = ROOT / f"validation/{stage}.cpp"
    harness = harness_path.read_text()
    hook = ("Slice slice(argv[1]); slice.verify_unwind();" if stage == "projection"
            else "NativeSlice slice(argv[1]);")
    if harness.count(hook) != 1:
        raise RuntimeError("Harness injection point changed")
    harness = 'extern "C" void automatic_set_base(unsigned char*);\n' + harness.replace(
        hook, hook + " automatic_set_base(slice.base);")
    (folder / "harness.cpp").write_text(harness, encoding="utf-8")
    executable = folder / "candidate.exe"
    command = [str(COMPILER), "-std=c++20", "-O2", "-g", "-Wall", "-Wextra",
               "-fno-fast-math", "-ffp-contract=off", "-frounding-math", "-ftrapping-math",
               "-fno-strict-aliasing", "-mno-avx", "-mno-fma", "-msse2", "-static",
               "-Wl,--no-insert-timestamp", "-I", str(folder), "-I", str(OUTPUT),
               "-I", str(ROOT / "validation"), "-I", str(UPSTREAM / "runtime/recomp64_cpu"),
               "-DAUTOMATIC_" + stage.upper(),
               str(folder / "harness.cpp"), str(ROOT / "validation/automatic_adapter.cpp"),
               str(ROOT / "validation/automatic_sse.cpp")]
    if stage == "projection":
        command.append(str(ROOT / "validation/abi_probe.S"))
    command += ["-o", str(executable)]
    build = subprocess.run(command, cwd=folder, env=environment(), capture_output=True, text=True)
    evidence = dict(variant=args.variant, stage=stage, build_command=command,
                    build_exit_code=build.returncode, build_stdout=build.stdout, build_stderr=build.stderr,
                    edits=edits, harness_change="Only declaration and automatic_set_base(slice.base) hook",
                    input_sha256=sha(EXE),
                    data_sha256={name: sha(folder / name) for name in [f"{stage}.pack", f"{stage}-manifest.json"]},
                    source_sha256={str(p.relative_to(ROOT)): sha(p) for p in
                        [Path(__file__), ROOT / "scripts/research_config.py", ROOT / "scripts/lift_automatic.py",
                         ROOT / "scripts/msvc_cleanup.py", ROOT / "scripts/repair_automatic.py",
                         ROOT / "validation/automatic_adapter.cpp", ROOT / "validation/automatic_sse.cpp", harness_path,
                         ROOT / f"validation/{stage}.h", ROOT / "validation/abi_probe.S", ROOT / "validation/native_slice.h",
                         folder / "cpu64.h", folder / "harness.cpp", folder / "candidate-generated.cpp", OUTPUT / f"{stage}-stock.c",
                         UPSTREAM / "runtime/recomp64_cpu/eh64.h"]})
    if build.returncode == 0:
        evidence["executable_sha256"] = sha(executable)
        evidence["runs"] = []
        for negative in [True, False]:
            run = [str(executable), str(folder / f"{stage}.pack")]
            if negative:
                run.append("--negative-control")
            started = time.perf_counter()
            result = subprocess.run(run, cwd=folder, env=environment(), capture_output=True, text=True)
            evidence["runs"].append(dict(negative_control=negative, command=run,
                exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr,
                seconds=time.perf_counter() - started))
    path = OUTPUT / f"{stage}-{args.variant}-results.json"
    path.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(evidence, indent=2))


if __name__ == "__main__":
    main()
