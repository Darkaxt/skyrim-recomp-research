# First connected slice: native file-object lifetime

Authority: [daily research specification](task-plan.md), task F04-001.
This selects the smallest evidenced resource/ownership workflow for the following
task IDs. It does not broaden the offline boundary or claim engine allocator,
archive/VFS, smart-pointer, streaming or gameplay support.

## Observable workflow

Use the pinned engine's own instructions to initialize a small file object,
open a harness-owned read-only file, obtain its size, read/seek/read, transfer
the live handle into another object, and close both objects in the correct
order. The moved-from object must no longer own the handle. Repeat construction
and use; exercise real open/read/seek failures and cleanup on partially completed
workflows. The original instruction execution is the oracle; automatically
lifted instructions are the later candidate.

The selected object is described as a **16-byte native file-handle wrapper**.
Its observed field offsets match the CommonLib `BSResource::BSSystemFile`
declaration, but the original C++ class identity and enum names are not proven.
Do not import SDK error enums or name-based behavior into the oracle. Numerical
status behavior comes from this build's actual mapper and real Windows results.

## Pinned dependency closure

Input: Steam SkyrimSE.exe 1.7.104.0, SHA256
`846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f`.
RVAs below are metadata, not distributed instruction bodies.

| RVA | Observed role | Bytes |
|---|---|---:|
| EC0190 | Default initialization: status/flags field and invalid handle | 18 |
| EBFD70 | Parameterized initialization and open; retain returned status | 59 |
| EC0410 | Open through the actual file API; map error if necessary | 365 |
| EC0590 | Read; return actual transferred count or mapped failure | 102 |
| EC0670 | Seek; return resulting position on success | 91 |
| EC0710 | Query size; zero the size output on failure | 62 |
| EC01B0 | Construct destination by copying ownership and resetting source | 38 |
| EBFF00 | Reset source status/flags and invalidate its handle | 11 |
| EC0990 | Write the invalid handle sentinel | 9 |
| EC0960 | Close a valid handle once, then invalidate it; skip invalid handle | 39 |
| EC4DE0 | Numeric Windows-error mapper with a bounded local switch | 70 |

Total: **11 entries, 864 code bytes**, seven native unwind records with no
handler/chain flags, and four routines needing no unwind record. The mapper
also needs **208 embedded table bytes**: six target RVAs and 184 selector bytes.
They are data on an executable section page; they are not extra functions.

All immediate control transfers remain within the selected closure. The one
computed jump resolves to six checked instruction starts in the mapper. All
RIP-relative memory imports resolve to the six APIs below; the mapper's remaining
RIP-relative LEA establishes the image base for its RVA tables. No game TLS,
engine singleton, allocator, virtual callback or unresolved global is required
by this selected path. Windows APIs retain their real OS-thread state.

The real native bridges are `CreateFileA`, `ReadFile`, `SetFilePointerEx`,
`GetFileSizeEx`, `GetLastError` and `CloseHandle`. The recovered path retains
their ABI and actual effects. Instrumentation must preserve last-error state
across trace bookkeeping; `SetLastError` and handle-validity queries used by the
harness are declared instrumentation, not successful replacement APIs.

## Fixture validity and isolation

- Use guarded, aligned raw storage of at least 16 bytes, initialized through
  the actual default/parameterized constructor. Byte offset 0 is the observed
  32-bit status/flags field; offset 8 is the handle. Offset 4 padding must remain
  unchanged unless original instructions write it. Do not fabricate a live
  engine object or seed it with a guessed global/heap/vtable.
- Use real handles returned by the API. Destination storage for the transfer
  constructor has no pre-existing live handle. Invoke the original reset/close
  instructions for ownership transitions. Harness cleanup must also reclaim any
  test-owned handle on a failing assertion without concealing candidate leaks.
- Initial coverage is synchronous, read-only, existing-file access, using the
  decoded mode values 0/0 and a false asynchronous flag. Use controlled ASCII
  paths within `local/automatic/file-lifetime/fixtures`.
  Generate fixture content there; never pass game/MO2 paths to these open calls.
  Do not exercise creation/truncation/write modes or unbuffered/overlapped I/O.
- Native mapping copies only declared code/table/unwind/IAT pieces at original
  relative offsets in a reserved image. The six IAT slots resolve to real typed
  Windows bridges. Do not load the complete EXE or call its entry point. The
  image-base anchor and RVA switch tables must work at the relocated mapping.
- Register the seven applicable native unwind records. The four verified
  stack-neutral leaf/tail routines do not acquire invented unwind entries. This
  slice has numeric API-error cleanup; it does not prove C++ catch/destruction,
  general exception conformance or asynchronous cancellation.

Read-only open-existing behavior follows the actual decoded calls and
[CreateFileA's API contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea).
Short/end-of-file read behavior and seek effects must follow the real calls,
checked against [ReadFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile)
and [SetFilePointerEx](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfilepointerex).
Verify ownership release using actual
[CloseHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle),
not just a changed sentinel field.

## Required normal corpus and observations

Use file lengths 0, 1, 31, 4097 and 65536 with at least zero, alternating and
deterministically generated byte patterns. Cover zero-length reads, reads smaller
than/equal to/larger than the remaining content, repeated reads, EOF, and seek
from beginning/current/end. Verify data, transferred counts, positions, status,
file hashes, object padding and guard regions. Exercise both initialization
forms and transfer followed by closing the source before using/closing the
destination. Repeated close must not issue another OS close for the same owner.

Trace API identity, argument order, scalar arguments, return status, required
last-error observations, output writes and handle ownership. Native and generated
runs may receive different opaque OS handle values: map each created handle to
a trace identity, retaining one-to-one identity, use and close order. Compare
actual object handle state against that run's recorded handle. Compare the final
invalid sentinel exactly. Do not equate unrelated handles, skip lifecycle checks
or loosen numerical/memory comparisons because the runs use independent handles.
Do not require equality of unspecified volatile registers or a return value for
the observed void close routine; check the actual calling convention, nonvolatile
state and permitted writes.

## Required failure corpus and controls

Use missing files and missing parent directories under the owned fixture root,
and a deterministic sharing conflict created by a harness-owned exclusive handle.
After open failure, verify constructor status, invalid handle and safe close.
After successful open, provoke a real invalid seek and read/size after close;
verify each routine's distinct output-on-error behavior and required last-error
mapping. Cover transfer and cleanup at each completed lifecycle step, followed
by a new successful construction/read/close. Do not induce faults by closing
unrelated/recycled handles or changing global security/ACL policy.

Also compare the pure mapper for every input 0..183 plus values 184, 255, 65535
and UINT32_MAX. Verify table dependencies and output status without inventing
enum meanings. The original oracle must independently meet fixture expectations.
Reject a candidate with a corrupted read result and a candidate that omits source
invalidation or close; leak/ownership controls must detect the actual OS effect,
not merely fail an unrelated result assertion. Match failures against the original
contract; don't turn an unspecified output into a stronger invented guarantee.

## Task criteria and known integration work

- **F04-002:** establish the native fixture, real bridges/trace validity, corpus
  expectations, normal/failure cleanup and an effective oracle/control check.
  Fix the native pack loader's mixed-page protection handling before execution:
  code and mapper tables share a page. Derive permissions from all pieces on
  each page so read-only data plus code yields executable/read-only permission
  independent of piece order. Reject any unsupported writable/executable union.
  Verify reversed piece ordering, relocated tables, valid leaf/unwind registration,
  and applicable regressions if shared loader support changes. Retain source/input/
  artifact hashes. No generated candidate is required by this ID.
- **C01-001:** translate all selected entries automatically; integrate direct and
  computed dispatch, the two tables and real typed API bridges. Pass the complete
  normal corpus with exact defined observations and the incorrect-read control.
- **C01-002:** pass the specified real failure/partial-workflow cases and mapper
  corpus; reject broken ownership/close controls. Preserve historical failures
  and fix causes without success stubs or relaxed comparisons.
- **T02-001:** reconcile the whole slice, all criteria/blockers/deferrals and
  relevant regressions. Record actual effort and reusable solutions, recheck input
  hashes and locally commit verified authored work. State precisely that this is
  file-resource/handle lifetime proof; engine heap, archive/VFS and broad stream
  support remain outside this slice.

These are future implementation criteria. F04-001 is complete only after its
static selection evidence and this specification are independently checked;
native execution and translation remain NOT STARTED at selection closure.
