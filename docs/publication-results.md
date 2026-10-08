# Final reconciliation and publication

All N1-N4 acceptance criteria pass. No required blocker or tracked deferral
remains in this requested scope.

| Requirement | Verified evidence |
|---|---|
| N1: cleanup | Unchanged serial/TLS/concurrency checks; all six native and generated throw positions/retries; disabling cleanup reproduces guard -1 failure |
| N2: lookup | All positions, misses, mixed case, empty/duplicate names; exact real CRT trace and memory preservation; incorrect candidate rejected |
| N3: CRC | Four real automatically lifted routines; complete chained native unwind records; exact guarded buffer/integer outputs and input preservation; incorrect result rejected |
| N4: reproduction | Fresh separate checkout installed pinned public dependencies and passed `python research.py --setup --run`; final packaged sources passed `python research.py --run`; retained verification also passes |
| N4: original inputs | All 13 previously sampled executable/DLL SHA256 identities rechecked unchanged; no game launch or mod execution |
| N4: cleanup | 56 explicitly reviewed expendable compiler intermediates/canceled-scan output deleted by exclusive handle after identity/hash verification; 2730722 bytes removed; required evidence retained |
| N4: public delivery | Public main branch created and its initial commit/tree verified against the exact reviewed local source set; no private Git history imported |

Repository: [Darkaxt/skyrim-recomp-research](https://github.com/Darkaxt/skyrim-recomp-research).
Initial verified public commit: `9b868ede204256eaf5f2b323576bab7d67fa68fc`.
The subsequent documentation commit records stage closure. Public visibility,
remote HEAD and exact tracked tree are checked again after that push.

Public history begins with an explicit authored allowlist. Game binaries,
constant tables, TLS templates, original/manual/decompiled/lifted instruction
bodies, packs, mod inventories and private research history are excluded.
Two authored extraction modules omit private manual-candidate CLI entry points;
their preparation logic is retained and tested in the public package. No game
bytes are embedded by this export transformation.

Private and public raw evidence stays under each checkout's ignored `local/`.
The public `docs/verified-gates.json` contains measured summaries only. Timings
are host-specific and include adapter overhead; they are not engine performance
estimates. The original stock/strict failures remain historical failures; the
new cleanup variant supersedes their capability limit without rewriting them.

Further authored research can evolve in the public checkout using the same
native-oracle and negative-control gate rules. Keep each new game-derived output
under ignored `local/` and review staged files before every public commit.
The initial export script intentionally refuses to overwrite an existing export.

No whole-engine, general exception, gameplay, save or native-mod compatibility
claim is made. Broader untested capabilities are research limits, not deferred
requirements of the now-completed requested gates.
