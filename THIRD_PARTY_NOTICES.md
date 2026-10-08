# Provenance and boundaries

Authored harnesses, drivers, repair integration and documentation are MIT licensed
under LICENSE. That license does not grant rights to Skyrim or other third-party
game code, data, names or assets. No game instruction bodies, lifted/decompiled
bodies, original constant tables, packs, TLS templates or game executables are
distributed. Supply your own compatible local executable; extraction stays in
ignored `local/` directories. This is an unofficial research project.

- [pcrecomp](https://github.com/sp00nznet/pcrecomp), commit
  `0a138b6ba423af7dd1d6955fb763dc2bfd2b058e`, MIT, copyright 2026 sp00nz.
  Setup clones its unmodified sources and license locally. Generated code uses
  its CPU runtime. Repairs subclass its lifter and preserve upstream attribution.
- [Capstone](https://github.com/capstone-engine/capstone), 5.0.7, BSD license;
  installed locally with its license notices, not vendored.
- [pefile](https://github.com/erocarrera/pefile), 2024.8.26, MIT license;
  installed locally with its license notices, not vendored.
- [BethesdaGhidraScripts](https://github.com/Nukem9/BethesdaGhidraScripts) and
  CommonLibSSE labels informed initial hypotheses. Current input bytes, imports,
  control flow and unwind records establish the tested boundaries; foreign-build
  labels are not treated as ABI proof. No SDK implementation is vendored.
- Ghidra was used for initial analysis; it is not required by the reproduction
  command and no Ghidra database is distributed.
- GCC/MinGW and MSVC/Windows runtimes remain separately installed local tools.

Pinned input: Steam Skyrim Special Edition 1.7.104.0, SHA256
`846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f`.
Other versions fail the input gate. Local generated artifacts may contain game
material and must remain excluded from Git and distribution.
