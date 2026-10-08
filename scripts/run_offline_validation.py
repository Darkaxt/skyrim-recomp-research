"""Build/run only explicitly pinned offline slices. All writes stay under E:."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import time

import pefile

from research_config import ROOT, EXE, EXPECTED, COMPILER


def prepare(output, stage):
    raw = EXE.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED:
        raise RuntimeError(f"Input identity changed: {digest}")
    pe = pefile.PE(data=raw, fast_load=True)
    directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    entries = dict((a, (b, u)) for a, b, u in struct.iter_unpack(
        "<III", pe.get_data(directory.VirtualAddress, directory.Size)))
    begin = 0xEF1180 if stage == "projection" else 0x447180
    end, unwind = entries[begin]
    header = pe.get_data(unwind, 4)
    if header[0] >> 3:
        raise RuntimeError("Slice unexpectedly has an exception handler")
    unwind_size = 4 + ((header[2] + 1) & ~1) * 2
    pieces = [(begin, end - begin, 1), (unwind, unwind_size, 2), (0x1B5BE28, 4, 2)]
    if stage == "projection":
        pieces += [(0x17ED6C0, 16, 2), (0x1B5BE14, 4, 2)]
    else:
        pieces += [(0x1B5BE80, 4, 2), (0x1B5BF14, 4, 2), (0x219DEC8, 8, 3)]
    pack = bytearray(struct.pack("<8s6I", b"SKYSLICE", 1, len(pieces), begin,
                                 end, unwind, pe.OPTIONAL_HEADER.SizeOfImage))
    manifest = dict(input=str(EXE), sha256=digest, begin=hex(begin), end=hex(end),
                    unwind=hex(unwind), pieces=[])
    for rva, size, kind in pieces:
        data = bytes(size) if kind == 3 else pe.get_data(rva, size)
        if len(data) != size:
            raise RuntimeError("Incomplete slice")
        pack.extend(struct.pack("<III", rva, size, kind))
        pack.extend(data)
        manifest["pieces"].append(dict(rva=hex(rva), size=size, kind=kind,
            sha256=hashlib.sha256(data).hexdigest(), bytes=data.hex()))
    (output / f"{stage}.pack").write_bytes(pack)
    (output / f"{stage}-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return digest
