"""Reconcile F04-002 evidence with current sources, inputs and existing gates."""
import json
from pathlib import Path
import subprocess
import sys

from run_file_lifetime_native import ROOT, EXE, EXPECTED, FOLDER, FIXTURES, sha, environment, check_trace


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    evidence_path = FOLDER/'native-results.json'
    native = json.loads(evidence_path.read_text())
    check(native['status'] == 'PASS' and native['input_sha256'] == sha(EXE) == EXPECTED,
          'Native input/result identity mismatch')
    check(native['closure_sha256'] == sha(ROOT/'validation/file_lifetime_manifest.json'),
          'Selected native closure changed')
    for name, digest in native['source_sha256'].items():
        check(sha(ROOT/name) == digest, f'Stale native source: {name}')
    for name, digest in native['artifact_sha256'].items():
        check(sha(FOLDER/name) == digest, f'Changed native artifact: {name}')
    for name, digest in native['fixture_sha256'].items():
        check(sha(FIXTURES/name) == digest, f'Changed fixture: {name}')
    check(len(native['fixture_sha256']) == 15 and len(native['runtime_functions']) == 7,
          'Native fixture/registration corpus incomplete')
    check(all(b['exit_code'] == 0 and not b['stderr'] for b in native['builds']), 'Native build failed')
    check(len(native['loader_runs']) == 3 and all(r['exit_code'] == 0 for r in native['loader_runs']),
          'Loader regression failed')
    red = json.loads((ROOT/'docs/file-lifetime-historical-controls.json').read_text())
    check([r['exit_code'] for r in red['runs']] == [1, 1, 1] and
          [r['stderr'].strip() for r in red['runs']] ==
          ['Mixed code/data page lost RX permission', 'Required invalid-pack rejection missing',
           'Rejected construction leaked reserved image'], 'Loader historical failures lost')
    negative, positive = native['native_runs']
    check(negative['negative_control'] and negative['exit_code'] == 1 and
          negative['stderr'] == 'Native oracle failure: read bytes\n', 'Read control ineffective')
    check(not positive['negative_control'] and positive['exit_code'] == 0 and not positive['stderr'],
          'Native run failed')
    summary = json.loads(positive['stdout'])
    check(summary == native['summary'] and summary['status'] == 'pass' and
          (summary['normal_workflows'], summary['failure_workflows'],
           summary['partial_cleanup_retry_workflows'], summary['mapper_checks']) == (120, 28, 56, 376),
          'Native workflow corpus changed')
    check(all(summary[key] for key in ('real_native_apis', 'guard_pages',
                                      'two_relocated_mappings', 'reversed_pack_order')) and
          not summary['game_launched'], 'Native isolation/relocation evidence missing')
    trace = check_trace(FOLDER/'native-trace.csv', summary)
    check(trace == native['trace_validation'], 'Native trace evidence changed')

    # This verifies the full suite's existing current results; it does not rerun it.
    existing = subprocess.run([sys.executable, str(ROOT/'research.py')], cwd=ROOT,
                              env=environment(), capture_output=True, text=True)
    check(existing.returncode == 0 and not existing.stderr, 'Existing regression evidence stale/failed')
    regressions = json.loads(existing.stdout)
    check(regressions['status'] == 'PASS', 'Existing regression gates failed')
    result = dict(task_id='F04-002', status='PASS',
                  criteria=dict(pinned_native_closure=True, mixed_page_order_and_failure_cleanup=True,
                                real_api_ownership_and_failure_trace=True, exact_memory_and_abi_checks=True,
                                relocated_tables_and_unwind=True, effective_read_and_abi_controls=True,
                                existing_shared_loader_regressions=True),
                  native_summary=summary, trace_validation=trace,
                  existing_gates=list(regressions['gates']),
                  native_evidence_sha256=sha(evidence_path),
                  regression_evidence_sha256=sha(ROOT/'local/automatic/next-slices-verification.json'),
                  verifier_sha256=sha(Path(__file__)), game_launched=False,
                  generated_candidate_started=False)
    (FOLDER/'F04-002-verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
