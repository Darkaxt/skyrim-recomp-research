#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

// Let the Microsoft compiler/runtime construct the loader TLS directory. The
// linker marker occupies the first 16 bytes; the build verifies that layout.
extern "C" {
extern ULONG _tls_index;
__declspec(thread) unsigned char original_tls_template[] = {
#include "tls-template.inc"
};
__declspec(dllexport) unsigned get_index() { return _tls_index; }
__declspec(dllexport) unsigned get_template_offset() {
    auto slots = reinterpret_cast<unsigned char**>(__readgsqword(0x58));
    return static_cast<unsigned>(original_tls_template - slots[_tls_index]);
}
__declspec(dllexport) int* get_epoch() {
    return reinterpret_cast<int*>(original_tls_template + 0x29f8);
}
}
