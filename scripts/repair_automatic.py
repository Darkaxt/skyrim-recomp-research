"""Bounded, explicit lifter repairs; no routine-specific semantic reconstruction."""
import sys

from lift_automatic import DEPS, UPSTREAM
from msvc_cleanup import records, declarations as cleanup_declarations


def repaired_lift(stage, ranges, variant, pe):
    sys.path[:0] = [str(DEPS), str(UPSTREAM / "tools/lift")]
    import lift64_cpu
    metadata = records(pe, ranges) if variant == "cleanup" else {}

    class RepairedLifter(lift64_cpu.Lifter):
        def translate(self, insn, labels):
            lines = super().translate(insn, labels)
            if insn.mnemonic == "call" and self.func_start - pe.OPTIONAL_HEADER.ImageBase in metadata:
                begin = self.func_start - pe.OPTIONAL_HEADER.ImageBase
                pc = insn.address - pe.OPTIONAL_HEADER.ImageBase
                return ["{ CPU saved = *c; try { " + " ".join(lines) +
                        f" }} catch (...) {{ automatic_cleanup_unwind(saved, cleanup_{begin:x}, 0x{pc:x}); throw; }} }}"]
            return lines

        def sse(self, insn):
            mnemonic = insn.mnemonic
            operands = insn.operands
            if mnemonic in ("comiss", "ucomiss"):
                return ["strict_%s(c, %s, %s);" % (mnemonic,
                    self._ss(insn, operands[0]), self._ss(insn, operands[1]))]
            if variant in ("strict", "cleanup") and mnemonic in ("addss", "subss", "mulss", "divss", "minss", "maxss"):
                dest = self._ss(insn, operands[0])
                return ["%s = strict_%s(%s, %s);" % (dest, mnemonic,
                    dest, self._ss(insn, operands[1]))]
            return super().sse(insn)

    lifter = RepairedLifter(image_size=pe.OPTIONAL_HEADER.SizeOfImage,
        read_va=lambda va, size: pe.get_data(va - 0x140000000, size), image_base=0x140000000)
    declarations = '\n'.join(f'extern "C" float strict_{op}(float, float);'
        for op in ("addss", "subss", "mulss", "divss", "minss", "maxss"))
    declarations += '\nextern "C" void strict_comiss(CPU*, float, float);'
    declarations += '\nextern "C" void strict_ucomiss(CPU*, float, float);'
    extra = sorted(set(pair for record in metadata.values() for pair in record["funclets"]))
    all_ranges = ranges + extra
    dispatch_table = ""
    if metadata:
        dispatch_table = '\nstatic bool automatic_generated_dispatch(CPU* c, uint64_t target) { switch(target) {\n'
        dispatch_table += '\n'.join(f'case 0x{pe.OPTIONAL_HEADER.ImageBase + a:x}ULL: L_{pe.OPTIONAL_HEADER.ImageBase + a:012X}(c); return true;'
                                   for a, _ in all_ranges)
        dispatch_table += '\ndefault: return false; }}\n'
    metadata_text = cleanup_declarations(metadata) if metadata else ""
    return ('#include "cpu64.h"\n' + metadata_text + declarations + '\n' + '\n\n'.join(
        lifter.lift_function(pe.get_data(begin, end - begin), 0x140000000 + begin)
        for begin, end in all_ranges) + dispatch_table)
