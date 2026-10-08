"""Recover bounded cleanup-only MSVC FuncInfo records from a pinned PE."""
import struct


def leaf_extent(pe, begin):
    # Leaf tail-call funclets have no .pdata record. Accept only the verified
    # no-stack LEA argument setup followed by a direct tail jump.
    from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    instructions = list(decoder.disasm(pe.get_data(begin, 32), pe.OPTIONAL_HEADER.ImageBase + begin, count=2))
    if (len(instructions) != 2 or instructions[0].mnemonic != 'lea'
            or instructions[0].reg_name(instructions[0].operands[0].reg) != 'rcx'
            or instructions[1].mnemonic != 'jmp'
            or instructions[1].operands[0].type != CS_OP_IMM):
        raise RuntimeError('Cleanup leaf extent is outside the validated LEA/tail-jump form')
    return begin + sum(i.size for i in instructions)


def records(pe, ranges):
    image = pe.OPTIONAL_HEADER.ImageBase
    directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    runtime = {a: (b, u) for a, b, u in struct.iter_unpack("<III",
               pe.get_data(directory.VirtualAddress, directory.Size))}
    result = {}
    for begin, _ in ranges:
        if begin not in runtime:
            continue
        end, unwind = runtime[begin]
        header = pe.get_data(unwind, 4)
        flags = header[0] >> 3
        if not flags & 3:
            continue
        if flags & 4:
            raise RuntimeError("Chained cleanup scopes need separate validation")
        tail = unwind + 4 + ((header[2] + 1) & ~1) * 2
        handler, info = struct.unpack("<II", pe.get_data(tail, 8))
        thunk = pe.get_data(handler, 6)
        if thunk[:2] != b'\xff\x25':
            raise RuntimeError('Unrecognized MSVC handler thunk')
        slot = handler + 6 + struct.unpack('<i', thunk[2:])[0]
        pe.parse_data_directories(directories=[1])
        handlers = [(e.dll.lower(), s.name) for e in pe.DIRECTORY_ENTRY_IMPORT
                    for s in e.imports if s.address-image == slot]
        if handlers != [(b'vcruntime140.dll', b'__CxxFrameHandler3')]:
            raise RuntimeError('Cleanup metadata is not handled by verified __CxxFrameHandler3')
        magic, count, actions, catches, _, ips, ip_map, _, _, _ = struct.unpack("<10I", pe.get_data(info, 40))
        if magic != 0x19930522 or catches or not 0 < count < 128 or not 0 < ips < 1024:
            raise RuntimeError("Only bounded cleanup-only MSVC FuncInfo 19930522 is supported")
        action_map = list(struct.iter_unpack("<iI", pe.get_data(actions, 8 * count)))
        states = list(struct.iter_unpack("<Ii", pe.get_data(ip_map, 8 * ips)))
        if states != sorted(states) or any(s < -1 or s >= count for _, s in states):
            raise RuntimeError("Invalid cleanup state map")
        for state, (next_state, _) in enumerate(action_map):
            if next_state < -1 or next_state >= state:
                raise RuntimeError("Cleanup state transitions must decrease")
        funclets = []
        for _, action in action_map:
            if not action:
                continue
            funclets.append((action, runtime[action][0] if action in runtime else leaf_extent(pe, action)))
        result[begin] = dict(begin=begin, end=end, unwind=unwind, image=image,
                             handler=handler, info=info, actions=action_map, states=states, funclets=funclets)
    return result


def declarations(metadata):
    out = ['#include "automatic_cleanup.h"']
    for begin, record in metadata.items():
        stem = f"cleanup_{begin:x}"
        out.append(f"static const AutomaticAction {stem}_actions[] = {{" +
                   ",".join(f"{{{s},0x{record['image']+a:x}ULL}}" if a else f"{{{s},0}}"
                            for s, a in record["actions"]) + "};")
        out.append(f"static const AutomaticState {stem}_states[] = {{" +
                   ",".join(f"{{0x{pc:x},{state}}}" for pc, state in record["states"]) + "};")
        out.append(f"static const AutomaticCleanup {stem} = {{{{0x{begin:x},0x{record['end']:x},"
                   f"0x{record['unwind']:x}}},{stem}_actions,{len(record['actions'])},"
                   f"{stem}_states,{len(record['states'])}}};")
    return "\n".join(out) + "\n"
