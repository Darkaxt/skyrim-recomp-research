# Structural coverage snapshot

Historical mapping snapshot: 2026-10-08. Its selected-harness counts describe
that milestone; see [current task progress](progress.md) for newer results.

Pinned input SHA256: `846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f`.

Generated from current PE metadata and the read-only Ghidra export. These are structural accounting values, not translation or engine-completion percentages.

| Item | Count |
|---|---:|
| Coverage domains | 38 |
| Discovered Ghidra function records | 176,001 |
| Function body ranges | 177,137 |
| MSVC runtime-function records | 122,258 |
| Static import DLLs / entries | 32 / 606 |
| Community function label rows / unique addresses | 701 / 698 |
| Function entries with selected-harness proof | 13 |
| Function entries without a proposed subsystem owner | 175,459 |
| Full subsystems verified | 0 |

| Call-index classification | Rows |
|---|---:|
| computed_external_label | 16,019 |
| computed_numeric_target | 4,017 |
| computed_without_target | 81,733 |
| direct_exact_entry | 413,759 |

Call rows can include multiple targets per site. External labels and numeric computed targets still require ABI/target-set verification.

| PE section | RVA | Virtual bytes | Ghidra body union | Structural treatment |
|---|---|---:|---:|---|
| SEC00 .text | 0x1000 | 24,929,512 | 22,859,691 | code/startup investigation |
| SEC01 .rdata | 0x17c8000 | 9,097,812 | 0 | data/metadata/resource ownership and initialization audit |
| SEC02 .data | 0x2076000 | 23,220,360 | 0 | data/metadata/resource ownership and initialization audit |
| SEC03 .pdata | 0x369c000 | 1,467,096 | 0 | data/metadata/resource ownership and initialization audit |
| SEC04 .text | 0x3803000 | 6,408 | 0 | data/metadata/resource ownership and initialization audit |
| SEC05 _RDATA | 0x3805000 | 1,760 | 0 | data/metadata/resource ownership and initialization audit |
| SEC06 .rsrc | 0x3806000 | 600,848 | 0 | data/metadata/resource ownership and initialization audit |
| SEC07 .reloc | 0x3899000 | 350,508 | 0 | data/metadata/resource ownership and initialization audit |
| SEC08 .bind | 0x38ef000 | 234,056 | 13,884 | code/startup investigation |

Executable section bytes outside recorded function bodies: **2,289,993**, in 136,353 ranges. This includes unclassified gaps that may be padding, embedded data, missed code or protected startup; it is not an untranslated-code count.

Declared image alignment/gap regions: 27,384 bytes. File gaps/overlay outside headers/raw sections: 10,592 bytes. Both are explicitly ledgered; neither is automatically declared unnecessary.

`.data` includes 22,011,528 virtual zero-fill bytes. Their live values and initialization/lifetimes cannot be recovered by copying file bytes alone.

Detailed function/range/import/call/gap ledgers and hashes stay under ignored `local/analysis/coverage/`.
