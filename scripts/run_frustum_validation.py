"""Pinned stateful slice with genuine loader TLS, OS synchronization and MSVC EH."""
import hashlib
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

import pefile
from run_offline_validation import ROOT, EXE, EXPECTED


def prepare(output):
    raw = EXE.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise RuntimeError("Original executable changed")
    pe = pefile.PE(data=raw, fast_load=True)
    pe.parse_data_directories(directories=[1, 9])
    directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    ranges = {a: (b, u) for a, b, u in struct.iter_unpack(
        "<III", pe.get_data(directory.VirtualAddress, directory.Size))}
    code = {0x224660: 0x2247D3, 0xEF7430: 0xEF7465, 0xEF74A0: 0xEF74E8,
            0xEF76F0: 0xEF7700, 0xEF7710: 0xEF7CED, 0x15A7C7C: 0x15A7CAC,
            0x15A7CAC: 0x15A7D0C, 0x15A7D0C: 0x15A7D73, 0x15A7D84: 0x15A7DD4,
            0x15A7DE4: 0x15A7E5B, 0x163811F: 0x163812B, 0x1627B90: 0x1627B92,
            0x15A97D4: 0x15A97DA}
    pieces = []
    functions = []
    for begin, end in code.items():
        pieces.append((begin, pe.get_data(begin, end - begin), 1, "original instructions"))
        if begin in ranges:
            actual_end, unwind = ranges[begin]
            if actual_end != end:
                raise RuntimeError(f"Unwind/code boundary mismatch {begin:x}")
            header = pe.get_data(unwind, 4)
            flags = header[0] >> 3
            if flags & 4:
                raise RuntimeError("Unexpected chained unwind; enumerate before execution")
            size = 4 + ((header[2] + 1) & ~1) * 2 + (8 if flags & 3 else 0)
            pieces.append((unwind, pe.get_data(unwind, size), 2, "original unwind info"))
            functions.append((begin, end, unwind))
    for rva, size, label in [(0x1803770, 40, "MSVC FuncInfo"), (0x1CC42F4, 8, "unwind map"),
                             (0x1CC42FC, 16, "IP state map"), (0x1B5BE28, 4, "one"),
                             (0x1B5C730, 16, "sign mask")]:
        pieces.append((rva, pe.get_data(rva, size), 2, label))
    for rva, size, label in [(0x21A41E0, 0x80, "frustum cache/guard/camera"),
                             (0x2079EA0, 4, "cached frame"), (0x3275610, 4, "fixture frame"),
                             (0x331C458, 12, "zero-filled default vector"),
                             (0x369A230, 0x50, "owned CRT synchronization and TLS index")]:
        pieces.append((rva, bytes(size), 3, label))
    for rva, size, label in [(0x20DA464, 4, "initial global epoch"), (0x20DA488, 8, "cookie")]:
        pieces.append((rva, pe.get_data(rva, size), 3, label))
    imports = []
    wanted = {0x17C8250, 0x17C8258, 0x17C8128, 0x17C8130, 0x17C8138, 0x17C8A58}
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        for symbol in entry.imports:
            rva = symbol.address - pe.OPTIONAL_HEADER.ImageBase
            if rva in wanted:
                imports.append(dict(rva=rva, dll=entry.dll.decode(), name=symbol.name.decode()))
                pieces.append((rva, bytes(8), 3, "host OS/runtime import fixup"))
    if {entry["rva"] for entry in imports} != wanted:
        raise RuntimeError("Missing import identity")
    pieces.append((0x17C93F8, bytes(8), 3, "relocated original CFG dispatch pointer"))
    functions.sort()
    primary_end, primary_unwind = ranges[0x224660]
    pack = bytearray(struct.pack("<8s6I", b"SKYSLICE", 2, len(pieces), 0x224660,
                                 primary_end, primary_unwind, pe.OPTIONAL_HEADER.SizeOfImage))
    pack.extend(struct.pack("<I", len(functions)))
    for function in functions:
        pack.extend(struct.pack("<III", *function))
    manifest = dict(input=str(EXE), sha256=EXPECTED, imports=imports,
                    runtime_functions=[[hex(x) for x in f] for f in functions], pieces=[])
    for rva, data, kind, label in pieces:
        if not data:
            raise RuntimeError(f"Missing bytes for {rva:x}")
        pack.extend(struct.pack("<III", rva, len(data), kind))
        pack.extend(data)
        manifest["pieces"].append(dict(rva=hex(rva), size=len(data), kind=kind, label=label,
                                       sha256=hashlib.sha256(data).hexdigest()))
    (output / "frustum.pack").write_bytes(pack)
    tls = pe.DIRECTORY_ENTRY_TLS.struct
    if pe.get_data(tls.AddressOfCallBacks - pe.OPTIONAL_HEADER.ImageBase, 8) != bytes(8):
        raise RuntimeError("Original loader TLS callbacks require further enumeration")
    template = pe.get_data(tls.StartAddressOfRawData - pe.OPTIONAL_HEADER.ImageBase,
                           tls.EndAddressOfRawData - tls.StartAddressOfRawData)
    if template[:16] != bytes(16):
        raise RuntimeError("Original TLS prefix cannot be represented by linker padding")
    if template[0x2A08:0x2A0C] != bytes.fromhex("00000080"):
        raise RuntimeError("TLS epoch initial value changed")
    (output / "tls-template.inc").write_text(
        "\n".join(",".join(f"0x{b:02x}" for b in template[i:i + 24]) + ","
                  for i in range(16, len(template), 24)) + "\n", encoding="ascii")
    manifest["tls"] = dict(size=len(template), sha256=hashlib.sha256(template).hexdigest(),
                            epoch_offset="0x2a08", original_callbacks=[])
    (output / "frustum-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest
