"""F04-002 only: extract private native packs and validate the real file oracle."""
import csv
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import time

import pefile
from research_config import ROOT, EXE, EXPECTED, COMPILER

FOLDER = ROOT / 'local/automatic/file-lifetime'
FIXTURES = FOLDER / 'fixtures'
IMPORTS = {0x17c83d8: 'CreateFileA', 0x17c8478: 'ReadFile',
           0x17c83a8: 'SetFilePointerEx', 0x17c83c8: 'GetFileSizeEx',
           0x17c8290: 'GetLastError', 0x17c8270: 'CloseHandle'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def checked_data(pe, rva, size, digest):
    data = pe.get_data(rva, size)
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise RuntimeError(f'Pinned native bytes changed: {rva:x}')
    return data


def environment():
    env = os.environ.copy()
    env.update(TEMP=str(ROOT / 'local/tmp'), TMP=str(ROOT / 'local/tmp'),
               PYTHONDONTWRITEBYTECODE='1', PYTHONPATH=str(ROOT / 'local/tools/translation-python'))
    return env


def invoke(command):
    started = time.perf_counter_ns()
    result = subprocess.run([str(a) for a in command], cwd=FOLDER, env=environment(),
                            capture_output=True, text=True)
    return dict(command=[str(a) for a in command], exit_code=result.returncode,
                stdout=result.stdout, stderr=result.stderr,
                elapsed_seconds=(time.perf_counter_ns()-started)/1e9)


def prepare():
    if ROOT.drive.upper() != 'E:' or sha(EXE) != EXPECTED:
        raise RuntimeError('Workspace/input identity mismatch')
    FOLDER.mkdir(parents=True, exist_ok=True)
    FIXTURES.mkdir(exist_ok=True)
    closure_path = ROOT / 'validation/file_lifetime_manifest.json'
    closure = json.loads(closure_path.read_text())
    if closure['input_sha256'] != EXPECTED or closure['code_bytes'] != 864:
        raise RuntimeError('Unexpected selected closure')
    pe = pefile.PE(str(EXE), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
    imports = {imp.address-pe.OPTIONAL_HEADER.ImageBase: (entry.dll.decode(), imp.name.decode())
               for entry in pe.DIRECTORY_ENTRY_IMPORT for imp in entry.imports if imp.name}
    if any(imports.get(rva) != ('KERNEL32.dll', name) for rva, name in IMPORTS.items()):
        raise RuntimeError('Native API identity mismatch')
    directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    real_runtime = set(struct.iter_unpack('<III', pe.get_data(directory.VirtualAddress, directory.Size)))
    pieces, runtime = [], []
    for function in closure['functions']:
        a, b = int(function['entry_rva'], 16), int(function['end_rva'], 16)
        pieces.append((a, checked_data(pe, a, b-a, function['code_sha256']), 1))
        if function['unwind']:
            unwind = function['unwind']; u = int(unwind['rva'], 16)
            if (a, b, u) not in real_runtime or unwind['flags'] != 0:
                raise RuntimeError('Native unwind identity mismatch')
            runtime.append((a, b, u))
            pieces.append((u, checked_data(pe, u, unwind['size'], unwind['sha256']), 2))
        elif any(a <= start < b for start, _, _ in real_runtime):
            raise RuntimeError('Unexpected leaf unwind')
    for table in closure['tables']:
        rva = int(table['rva'], 16)
        pieces.append((rva, checked_data(pe, rva, table['bytes'], table['sha256']), 2))
    pieces.extend((rva, bytes(8), 3) for rva in IMPORTS)
    runtime.sort()
    if len(runtime) != 7 or len(closure['functions']) != 11:
        raise RuntimeError('Native closure size mismatch')
    root = next(f for f in runtime if f[0] == 0xec0410)
    for name, ordered in [('native.pack', pieces), ('native-reversed.pack', pieces[::-1])]:
        pack = bytearray(struct.pack('<8s6I', b'SKYSLICE', 2, len(ordered), *root,
                                     pe.OPTIONAL_HEADER.SizeOfImage))
        pack.extend(struct.pack('<I', len(runtime)))
        for entry in runtime:
            pack.extend(struct.pack('<III', *entry))
        for rva, data, kind in ordered:
            pack.extend(struct.pack('<III', rva, len(data), kind)); pack.extend(data)
        (FOLDER/name).write_bytes(pack)

    # Independently interpret the six checked return blocks, not the mapper code.
    targets = struct.unpack('<6I', pe.get_data(0xec4e28, 24))
    selectors = pe.get_data(0xec4e40, 184)
    def scalar_return(rva):
        block = pe.get_data(rva, 6)
        if block[:3] == b'\x33\xc0\xc3':
            return 0
        if block[0] == 0xb8 and block[5] == 0xc3:
            return struct.unpack('<I', block[1:5])[0]
        raise RuntimeError('Unexpected mapper target contract')
    if any(s >= 6 for s in selectors) or any(not 0xec4de0 <= t < 0xec4e26 for t in targets):
        raise RuntimeError('Mapper target outside selected closure')
    values = [scalar_return(targets[s]) for s in selectors]
    fallback = scalar_return(0xec4e20)
    (FOLDER/'mapper-expectations.bin').write_bytes(struct.pack('<188I', *(values+[fallback]*4)))

    fixture_hashes = {}
    for size in (0, 1, 31, 4097, 65536):
        for pattern in range(3):
            data = bytes(0 if pattern == 0 else (0x55 if i & 1 else 0xaa) if pattern == 1
                         else ((i*73+19) ^ (i >> 3) ^ ((i*i) >> 7)) & 255 for i in range(size))
            path = FIXTURES/f'{size}_{pattern}.dat'
            if path.exists() and path.read_bytes() != data:
                raise RuntimeError(f'Existing fixture identity mismatch: {path.name}')
            if not path.exists():
                path.write_bytes(data)
            fixture_hashes[path.name] = sha(path)
    if (FIXTURES/'missing.dat').exists() or (FIXTURES/'missing-parent').exists():
        raise RuntimeError('Failure fixture paths must be absent')
    return dict(input_sha256=EXPECTED, closure_sha256=sha(closure_path), fixture_sha256=fixture_hashes,
                native_imports={hex(rva): name for rva, name in IMPORTS.items()},
                runtime_functions=[[hex(v) for v in f] for f in runtime])


def check_trace(path, summary):
    """Reconcile real API effects independently of the harness's assertions."""
    rows = list(csv.DictReader(path.open(newline='')))
    live, created, cases, counts = {}, set(), set(), {}
    previous = None
    for row in rows:
        case = (int(row['variant']), row['case'])
        if previous != case:
            if live:
                raise RuntimeError('Trace ownership escaped its workflow')
            if case in cases:
                raise RuntimeError('Trace workflow identity repeated')
            cases.add(case); previous = case
        api = row['api']; counts[api] = counts.get(api, 0)+1
        handle, result = int(row['handle_id']), int(row['result'])
        if api == 'CreateFileA':
            if (int(row['b']), int(row['c']), int(row['d']), int(row['e'])) != (0x80000000, 1, 3, 0x08000000):
                raise RuntimeError('Trace open contract mismatch')
            if result:
                if not handle or handle in created:
                    raise RuntimeError('Trace handle identity reused')
                live[handle] = case; created.add(handle)
            elif handle:
                raise RuntimeError('Failed open acquired an identity')
        elif api == 'CloseHandle':
            if result != 1 or handle not in live:
                raise RuntimeError('Trace double close/invalid close')
            del live[handle]
        elif api in ('ReadFile', 'SetFilePointerEx', 'GetFileSizeEx'):
            if result and handle not in live:
                raise RuntimeError('Trace use after close')
            if not result and int(row['last_error']) == 6 and handle != 0:
                raise RuntimeError('Closed operation used a non-sentinel handle')
        elif api == 'GetLastError':
            if result != int(row['last_error']):
                raise RuntimeError('Last error trace clobbered')
        else:
            raise RuntimeError('Unknown trace API')
    if live or len(cases) != 204 or len(rows)+1 != summary['api_calls']:
        raise RuntimeError('Trace/corpus reconciliation failed')
    return dict(rows=len(rows), workflows=len(cases), created_and_closed_handles=len(created),
                api_counts=counts, ownership_reconciled=True, sha256=sha(path))


def main():
    evidence = dict(task_id='F04-002', status='FAIL', builds=[], loader_runs=[], native_runs=[])
    try:
        evidence.update(prepare())
        flags = ['-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-strict-aliasing', '-static']
        for sources, output in [(['file_lifetime_native.cpp', 'file_call_probe.S'], 'native.exe'),
                                (['native_slice_test.cpp'], 'native_slice_test.exe')]:
            build = invoke([COMPILER, *flags, *(ROOT/'validation'/s for s in sources), '-o', FOLDER/output])
            evidence['builds'].append(build)
            if build['exit_code']:
                raise RuntimeError(f'Build failed: {output}: {build["stderr"]}')
        for mode in ('mixed', 'wx', 'truncated'):
            result = invoke([FOLDER/'native_slice_test.exe', FOLDER, mode])
            evidence['loader_runs'].append(result)
            if result['exit_code']:
                raise RuntimeError(f'Loader regression failed: {mode}')
        for negative in (True, False):
            trace = FOLDER/('negative-trace.csv' if negative else 'native-trace.csv')
            args = [FOLDER/'native.exe', FOLDER/'native.pack', FOLDER/'native-reversed.pack',
                    FIXTURES, FOLDER/'mapper-expectations.bin', trace]
            if negative:
                args.append('--corrupt-read-control')
            run = invoke(args); run['negative_control'] = negative
            evidence['native_runs'].append(run)
            if negative:
                if run['exit_code'] != 1 or run['stderr'].strip() != 'Native oracle failure: read bytes':
                    raise RuntimeError('Incorrect-read oracle control ineffective')
            else:
                if run['exit_code'] or run['stderr']:
                    raise RuntimeError('Native oracle failed: '+run['stderr'])
                summary = json.loads(run['stdout']); evidence['summary'] = summary
                if (summary['normal_workflows'], summary['failure_workflows'],
                    summary['partial_cleanup_retry_workflows'], summary['mapper_checks']) != (120, 28, 56, 376):
                    raise RuntimeError('Native corpus missing workflows')
                evidence['trace_validation'] = check_trace(trace, summary)
        sources = [Path(__file__), ROOT/'scripts/research_config.py',
                   *(ROOT/'validation'/s for s in ['file_lifetime_native.cpp', 'file_call_probe.S',
                                                 'native_slice.h', 'native_slice_test.cpp'])]
        evidence['source_sha256'] = {str(p.relative_to(ROOT)): sha(p) for p in sources}
        artifacts = ['native.pack', 'native-reversed.pack', 'mapper-expectations.bin',
                     'native.exe', 'native_slice_test.exe', 'native-trace.csv', 'negative-trace.csv']
        evidence['artifact_sha256'] = {name: sha(FOLDER/name) for name in artifacts}
        if sha(EXE) != EXPECTED:
            raise RuntimeError('Original input changed during validation')
        evidence['status'] = 'PASS'
    except Exception as error:
        evidence['error'] = str(error)
        raise
    finally:
        FOLDER.mkdir(parents=True, exist_ok=True)
        (FOLDER/'native-results.json').write_text(json.dumps(evidence, indent=2)+'\n')
        print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
