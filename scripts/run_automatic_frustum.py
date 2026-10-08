"""MSVC stateful oracle with generated pcrecomp bodies and explicit native CRT boundary."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

import pefile
from lift_automatic import ROOT, OUTPUT, UPSTREAM, EXE, RANGES, sha, environment
from repair_automatic import repaired_lift
from run_offline_validation import COMPILER
from run_frustum_validation import prepare
from research_config import vcvars as find_vcvars


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--variant", choices=["stock", "compare", "strict", "cleanup"], default="stock")
    parser.add_argument("--disable-cleanup", action="store_true")
    args = parser.parse_args()
    if ROOT.drive.upper() != "E:":
        raise RuntimeError("Research outputs must stay on E:")
    if args.disable_cleanup and args.variant != "cleanup":
        parser.error("--disable-cleanup requires --variant cleanup")
    label = args.variant + ("-disabled" if args.disable_cleanup else "")
    folder = OUTPUT / f"frustum-{label}"
    folder.mkdir(exist_ok=True)
    manifest = prepare(folder)
    (folder / "cpu64.h").write_bytes((UPSTREAM / "runtime/recomp64_cpu/cpu64.h").read_bytes())
    pe = pefile.PE(str(EXE), fast_load=True)
    generated = ((OUTPUT / "frustum-stock.c").read_text() if args.variant == "stock" else
                 repaired_lift("frustum", RANGES["frustum"], args.variant, pe))
    (folder / "candidate-generated.cpp").write_text(generated, encoding="utf-8")
    harness_path = ROOT / "validation/frustum.cpp"
    harness = harness_path.read_text()
    hook = "frustum_context = {slice.base, get_epoch, nullptr};"
    checkpoint_serial = "        initialize_camera(cameras[0], false); bound[0] = bound[1] = 0;"
    checkpoint_threads = "        c.constructor_override = nullptr;\n        for (unsigned candidate = 0;"
    checkpoint_exception = "            require(caught && c.at<int>(0x21A4250) == 0"
    checkpoint_state = "                if (result_a != result_b || mxcsr_a != mxcsr_b"
    for marker in (hook, checkpoint_serial, checkpoint_threads, checkpoint_exception, checkpoint_state):
        if harness.count(marker) != 1:
            raise RuntimeError("Pinned stateful harness injection point changed")
    harness = 'extern "C" void automatic_set_base(unsigned char*);\n' + harness.replace(
        hook, hook + " automatic_set_base(slice.base);")
    harness = harness.replace(checkpoint_serial,
        '        std::cout << "{\\"checkpoint\\":\\"serial_pass\\",\\"trials\\":" << trials << "}\\n" << std::flush;\n' + checkpoint_serial)
    harness = harness.replace(checkpoint_threads,
        '        std::cout << "{\\"checkpoint\\":\\"concurrency_pass\\",\\"threads\\":6,\\"waiters\\":5}\\n" << std::flush;\n' + checkpoint_threads)
    harness = harness.replace(checkpoint_exception,
        '            std::cout << "{\\"checkpoint\\":\\"exception_state\\",\\"candidate\\":" << candidate'
        ' << ",\\"position\\":" << failure << ",\\"caught\\":" << caught'
        ' << ",\\"guard\\":" << c.at<int>(0x21A4250) << ",\\"global_epoch\\":" << c.at<int>(0x20DA464)'
        ' << ",\\"tls_epoch\\":" << *c.get_epoch() << ",\\"lock_recursion\\":" << lock->RecursionCount'
        ' << "}\\n" << std::flush;\n' + checkpoint_exception)
    harness = harness.replace(checkpoint_state,
        '                if (!(after_a == after_b)) for (unsigned byte = 0; byte < 128; ++byte) {\n'
        '                    if (after_a.cache[byte] != after_b.cache[byte]) {\n'
        '                        std::cerr << "Cache mismatch byte=" << byte << " original=" << unsigned(after_a.cache[byte])'
        ' << " generated=" << unsigned(after_b.cache[byte]) << "\\n"; break;\n'
        '                    }\n                }\n' + checkpoint_state)
    (folder / "harness.cpp").write_text(harness, encoding="utf-8")
    env = environment()
    env["VSCMD_SKIP_SENDTELEMETRY"] = "1"
    vcvars = find_vcvars()
    env_script = folder / "msvc-env.cmd"
    env_script.write_text(f'@echo off\ncall "{vcvars}" >nul\nif errorlevel 1 exit /b 1\nset\n')
    settings = subprocess.check_output(["cmd.exe", "/d", "/c", str(env_script)], cwd=folder, env=env, text=True)
    for line in settings.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            env[key] = value
    cl = shutil.which("cl.exe", path=env.get("Path", env.get("PATH")))
    if not cl:
        raise RuntimeError("MSVC missing")
    includes = ["/I" + str(folder), "/I" + str(ROOT / "validation"),
                "/I" + str(UPSTREAM / "runtime/recomp64_cpu")]
    dll_cmd = [cl, "/nologo", "/LD", "/MD", "/O2", "/W4", "/WX", "/I" + str(folder),
               "/Fo" + str(folder / "tls_owner.obj"), str(ROOT / "validation/tls_owner.cpp"),
               "/link", "/OUT:" + str(folder / "tls_owner.dll"), "/IMPLIB:" + str(folder / "tls_owner.lib")]
    sse_cmd = [str(COMPILER), "-O2", "-fno-fast-math", "-ffp-contract=off", "-mno-avx", "-mno-fma",
               "-I", str(folder), "-I", str(UPSTREAM / "runtime/recomp64_cpu"),
               "-c", str(ROOT / "validation/automatic_sse.cpp"), "-o", str(folder / "automatic_sse.obj")]
    exe_cmd = [cl, "/nologo", "/std:c++20", "/MD", "/EHs", "/O2", "/fp:strict", "/W3",
               "/DAUTOMATIC_FRUSTUM", *(["/DAUTOMATIC_CLEANUP"] if args.variant == "cleanup" else []),
               *(["/DAUTOMATIC_DISABLE_CLEANUP"] if args.disable_cleanup else []),
               *includes, "/Fo" + str(folder) + "\\",
               "/Fd" + str(folder / "candidate.pdb"), str(folder / "harness.cpp"),
               str(ROOT / "validation/automatic_adapter.cpp"), str(folder / "automatic_sse.obj"),
               str(folder / "tls_owner.lib"), "/link", "/OUT:" + str(folder / "candidate.exe")]
    evidence = dict(stage="frustum", variant=args.variant, cleanup_disabled=args.disable_cleanup, edits=[] if args.variant == "stock" else
                    ["generic scalar-SSE comparison repair"] +
                    (["generic scalar-SSE arithmetic/min/max repair"] if args.variant in ("strict", "cleanup") else []) +
                    (["metadata-driven cleanup-only unwinding"] if args.variant == "cleanup" else []),
                    harness_changes=["base-address hook", "serial/concurrency/exception/state diagnostics; assertions unchanged"],
                    input_sha256=manifest["sha256"],
                    data_sha256={name: sha(folder / name) for name in ["frustum.pack", "frustum-manifest.json", "tls-template.inc"]},
                    builds=[], runs=[])
    for command in [dll_cmd, sse_cmd, exe_cmd]:
        result = subprocess.run(command, cwd=folder, env=env, capture_output=True, text=True)
        evidence["builds"].append(dict(command=command, exit_code=result.returncode,
                                       stdout=result.stdout, stderr=result.stderr))
        if result.returncode:
            break
    else:
        owner = pefile.PE(str(folder / "tls_owner.dll"), fast_load=True)
        owner.parse_data_directories(directories=[9])
        tls = owner.DIRECTORY_ENTRY_TLS.struct
        template = owner.get_data(tls.StartAddressOfRawData - owner.OPTIONAL_HEADER.ImageBase,
                                  manifest["tls"]["size"])
        if hashlib.sha256(template).hexdigest() != manifest["tls"]["sha256"]:
            raise RuntimeError("Generated TLS prefix differs from original")
        evidence["loader_tls_template_verified"] = True
        evidence["hashes"] = {p.name: sha(p) for p in [folder / "candidate.exe", folder / "tls_owner.dll"]}
        for negative in [True, False]:
            command = [str(folder / "candidate.exe"), str(folder / "frustum.pack"), str(folder / "tls_owner.dll")]
            if negative:
                command.append("--negative-control")
            start = time.perf_counter()
            result = subprocess.run(command, cwd=folder, env=env, capture_output=True, text=True)
            evidence["runs"].append(dict(command=command, negative_control=negative,
                exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr,
                seconds=time.perf_counter() - start))
    files = [Path(__file__), ROOT / "scripts/research_config.py", ROOT / "scripts/lift_automatic.py",
             ROOT / "scripts/repair_automatic.py", ROOT / "scripts/msvc_cleanup.py",
             ROOT / "validation/automatic_cleanup.h", ROOT / "scripts/run_frustum_validation.py",
             ROOT / "scripts/run_offline_validation.py", ROOT / "validation/automatic_adapter.cpp",
             ROOT / "validation/automatic_sse.cpp", ROOT / "validation/frustum.cpp", ROOT / "validation/frustum.h",
             ROOT / "validation/tls_owner.cpp", ROOT / "validation/native_slice.h",
             folder / "harness.cpp", folder / "candidate-generated.cpp", folder / "cpu64.h",
             UPSTREAM / "runtime/recomp64_cpu/eh64.h"]
    evidence["source_sha256"] = {str(p.relative_to(ROOT)): sha(p) for p in files}
    (OUTPUT / f"frustum-{label}-results.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(evidence, indent=2))


if __name__ == "__main__":
    main()
