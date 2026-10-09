# Measured inventory and partial group sizes

These are sizes of the currently attributed records, not full subsystem sizes or effort estimates. Most domain owners are unknown; named ownership is predominantly heuristic. Zero means no records attributed by this analysis, not an empty subsystem.

| Coverage group | Attributed function records | Body union bytes | Planned import entries | Full group size |
|---|---:|---:|---:|---|
| Foundations | 36 | 10,303 | 301 | Not estimated |
| Content | 20 | 8,803 | 0 | Not estimated |
| World | 66 | 30,888 | 0 | Not estimated |
| Simulation | 300 | 117,811 | 0 | Not estimated |
| Presentation | 112 | 28,935 | 37 | Not estimated |
| Persistence | 8 | 3,217 | 0 | Not estimated |
| Platform | 0 | 0 | 268 | Not estimated |
| Compatibility | 0 | 0 | 0 | Not estimated |
| Tooling | 0 | 0 | 0 | Not estimated |
| Unknown | 175,459 | 22,673,618 | 0 | Not estimated |

**Total:** 176,001 discovered function records; 22,873,575 unique Ghidra body bytes (21.81 MiB). Only 542 records (0.308%) have a proposed domain other than U00. This is attribution coverage, not translation completion.

Unique body-byte partition: 199,957 only in proposed domains; 22,673,618 only in U00; 0 shared between those categories. These sum to 22,873,575.

Body union is calculated separately within each group. Group and domain unions can overlap and must not be added as if disjoint. CSV record-body-byte and instruction sums count each function record, so shared tails can be counted more than once.

Another 2,289,993 executable-section bytes lie outside analyzer bodies and require classification; they are not all necessarily code. The executable file is 37,910,440 bytes and its declared loaded image is 59,936,768 bytes, including data, metadata, resources and zero-fill.

The table does not size resource archives, scripts, shaders, saves, external DLL implementations, mod collections or tooling. Their bytes and integration workloads require separate inventories. Platform/GPU/API import counts describe planned bridge scope, not translated engine function ownership.

The attributed counts are not confirmed lower bounds: a heuristic owner can be wrong. In particular, no size ratio or zero count justifies ranking the true sizes of Papyrus, physics, rendering or mod compatibility.

[Domain CSV](engine-domain-sizes.csv) includes all 38 domains, instruction/body-record totals and selected-harness entry counts. [Group CSV](engine-group-sizes.csv) provides the table data. See [effort and calibration](engine-effort.md) for planning judgments.

This table records the 2026-10-08 private mapping snapshot. Its detailed inputs
and reconciliation are retained with the private evidence backup.
