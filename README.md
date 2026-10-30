# efiprobe

A working [UEFI][uefi] application that dumps the firmware memory map, the
configuration tables, the graphics output modes and the loaded image — and a
**reference / template** for anyone writing their own freestanding `.efi` code
with MSVC instead of EDK2.

`efiprobe` is deliberately structured like a real project, not a single demo
file, so it works as something you can copy and start deleting from. See
[Repository layout](#repository-layout) below. There are **no external
dependencies**: no EDK2, no libc, no imported DLLs. `link.exe` has known about
`/SUBSYSTEM:EFI_APPLICATION` for years, which is the whole trick.

**What it is:** a valid PE32+ image (machine `0x8664`, subsystem 10, zero
imports, `ImageBase` 0) that the firmware loads, relocates and calls at
`efi_main`. It is boot-tested under QEMU 11.1.0 with OVMF
(`edk2-x86_64-code.fd`, UEFI 2.70); the captured output is in
[`example/ovmf.txt`](example/ovmf.txt) and the test is reproducible with
`tools/boot_test.py`. Nine structure sizes and 29 selected member offsets are
checked at build time, and the produced PE is validated byte-for-byte; see
[Verification](#verification).

[uefi]: https://uefi.org/specifications

---

## Table of contents

1. [How this works](#how-this-works)
2. [Repository layout](#repository-layout)
3. [What it prints](#what-it-prints)
4. [Build from source](#build-from-source)
5. [Run it](#run-it)
6. [Write your own `.efi` application](#write-your-own-efi-application)
   - [Minimum viable application](#minimum-viable-application)
   - [The boot services table](#the-boot-services-table)
   - [Protocols and GUIDs](#protocols-and-guids)
   - [The memory map](#the-memory-map)
   - [Freestanding CRT shims](#freestanding-crt-shims)
   - [Struct layout is the whole game](#struct-layout-is-the-whole-game)
7. [Verification](#verification)
8. [Release process](#release-process)
9. [License](#license)

---

## How this works

UEFI x64 uses the Microsoft x64 calling convention (RCX, RDX, R8, R9), which
is exactly what MSVC emits. So `EFIAPI` is empty here and an ordinary C
function pointer can call into firmware. That is the only reason this works
without EDK2.

Startup sequence:

```
firmware
  ↳ locates efiprobe.efi on the ESP
  ↳ loads PE32+ (subsystem 10), applies .reloc against ImageBase 0
  ↳ calls efi_main(ImageHandle, SystemTable)
      ↳ gST / gOut stashed from SystemTable
      ↳ ConOut->ClearScreen / SetAttribute
      ↳ banner()            gST->Hdr.Revision, FirmwareVendor
      ↳ conout_info()       ConOut->QueryMode on the current mode
      ↳ loaded_image()      BS->HandleProtocol(ImageHandle, LOADED_IMAGE)
      ↳ config_tables()     gST->ConfigurationTable[] with GUID names
      ↳ gop_modes()         BS->LocateProtocol(GOP), QueryMode per mode
      ↳ memory_map()        BS->GetMemoryMap (sizing call, then real call)
      ↳ return EFI_SUCCESS  → firmware shows the shell prompt again
```

Nothing calls `ExitBootServices`. The app owns no memory after it returns —
the pool buffer and every `QueryMode` info block are freed inside their own
section — and the firmware stays in boot services the whole time.

---

## Repository layout

```
efiprobe/
├── src/
│   ├── efi.h                 hand-written UEFI types, protocol layouts,
│   │                         GUIDs, memory type/attribute constants
│   ├── crt.c                 memcpy / memset / __chkstk shims
│   └── efiprobe.c            the application: print helpers + 6 dump sections
├── tools/
│   ├── layout.c              checks 9 struct sizes and 29 member offsets
│   ├── pecheck.py            validates the linked PE (stdlib only)
│   ├── chkstk.py             fails the build if __chkstk is ever referenced
│   └── boot_test.py          boots the image under QEMU/OVMF, captures ConOut
├── example/
│   └── ovmf.txt              captured output from a real OVMF boot
├── .github/workflows/        build + release
├── build.bat                 layout → compile → link → stack probe → pe check
├── notes.txt                 what OVMF taught me
├── LICENSE                   Unlicense
└── README.md
```

**How the pieces fit.** `efi.h` is the only header. `efiprobe.c` includes it
and nothing else — not even `<stdio.h>`. All output goes through
`gOut->OutputString`, so there is no CRT dependency to satisfy at link time.
`crt.c` exists solely because MSVC sometimes emits calls to `memset`, `memcpy`
or `__chkstk` even in `/GS-` code; defining them locally keeps
`/NODEFAULTLIB` honest.

`CHAR16` is `wchar_t`, which is 16 bits on MSVC but not universally. This is
an MSVC x64-only project; do not read `efi.h` as a portable UEFI header.

**Layer discipline.** `tools/layout.c` compiles as a normal console program
(with the CRT) and includes `efi.h` to check its offsets. That means the header
has to be valid in both a freestanding C translation unit and a hosted one —
which is why it pulls in `<stddef.h>` for `wchar_t`/`NULL` and defines nothing
that needs a runtime. `tools/pecheck.py` never sees C at all; it reads the
final bytes. `tools/boot_test.py` is the third layer: it runs the finished
image under emulated firmware and captures the console over the serial port.
`tools/chkstk.py` checks the generated disassembly for `__chkstk` calls —
the no-op shim is only safe while no frame exceeds one page, and the build
now enforces that instead of assuming it.

**Coverage of the layout checker.** It asserts `sizeof` for 9 structures and
`offsetof` for 29 selected members: `EFI_GUID`, `EFI_TABLE_HEADER`,
`EFI_MEMORY_DESCRIPTOR`, `EFI_CONFIGURATION_TABLE`, `EFI_SYSTEM_TABLE`,
`EFI_BOOT_SERVICES`, `EFI_GRAPHICS_OUTPUT_MODE_INFORMATION`,
`EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE` and `EFI_LOADED_IMAGE_PROTOCOL`. It does
not check every function-pointer slot in `EFI_BOOT_SERVICES` or every member
of every protocol. The offsets it does check are the ones the app calls.

**Sizing.** 604 lines of C across 3 files, 322 lines of verification tooling.
`efi.h` is the bulk of it at 266 lines, almost all struct definitions — that
is the point. `efiprobe.c` is 324 lines and each dump is a single
self-contained function. The split isn't about hitting a line target; it's
that when you add a seventh protocol or a new config-table GUID there's an
obvious place for it, and the offset checker catches you immediately if you
get a pad wrong.

---

## What it prints

| Section | Source | What it does |
| --- | --- | --- |
| `banner` | `EFI_SYSTEM_TABLE` | firmware vendor, revision, UEFI spec revision |
| `conout` | `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL` | current mode, columns/rows, protocol address |
| `loaded image` | `EFI_LOADED_IMAGE_PROTOCOL` | image base/size, code type, load options, device handle |
| `config tables` | `EFI_CONFIGURATION_TABLE[]` | GUID, friendly name, table address (ACPI, SMBIOS, ESRT, …) |
| `gop` | `EFI_GRAPHICS_OUTPUT_PROTOCOL` | framebuffer base/size, every mode's resolution, stride, pixel format |
| `memmap` | `EFI_MEMORY_DESCRIPTOR[]` | runtime descriptors, per-type totals, attribute flags |

The GUID name table is deliberately short (10 entries) and falls back to `-`
for unknown entries — it prints the raw GUID either way, so a table you don't
recognise is still usable.

---

## Build from source

Requires **MSVC 2022 x64** (`cl`, `link`) and **Python 3** for the PE check.
No CMake, no EDK2, no WDK, no vcpkg. Run from any prompt — `build.bat` finds
`vcvars64.bat` itself if `cl` is not already on `PATH`.

```bat
git clone https://github.com/vmgexit/efiprobe.git
cd efiprobe
build.bat
```

Output: `build\efiprobe.efi`.

The build is five stages and any failure stops it:

```
[*] layout check     cl /TC tools\layout.c  →  build\layout.exe → run it
[*] compiling        cl /c /TC /GS- /Zl /O1 /Fo:build\ src\*.c
[*] linking          link /SUBSYSTEM:EFI_APPLICATION /ENTRY:efi_main
                       /NODEFAULTLIB /MACHINE:X64 /NXCOMPAT:NO /BASE:0x0
                       /IGNORE:4281
[*] stack probe check  dumpbin /disasm → py tools\chkstk.py
[*] pe check           py tools\pecheck.py build\efiprobe.efi
```

Flags that matter:

| Flag | Why |
| --- | --- |
| `/SUBSYSTEM:EFI_APPLICATION` | sets the PE subsystem to 10; this is what makes it an `.efi` |
| `/ENTRY:efi_main` | x64 has no leading underscore, so the symbol is exactly `efi_main` |
| `/NODEFAULTLIB` | a firmware image cannot import the CRT |
| `/GS-` | no stack cookie, which would need `__security_check_cookie` |
| `/Zl` | don't bake a `/DEFAULTLIB` directive into the objects |
| `/BASE:0x0` | EDK2-style zero image base; firmware applies `.reloc` |
| `/NXCOMPAT:NO` | some firmware rejects images with DEP flags set |
| `/IGNORE:4281` | suppress the harmless ASLR warning that `/BASE:0x0` produces |

`/O1` rather than `/O2` keeps stack frames small. A frame over one page makes
MSVC emit a `__chkstk` call — `crt.c` provides a no-op for exactly that case,
but the app is written to avoid needing it.

---

## Run it

The one-command path is `py tools\boot_test.py`. It builds an ESP, boots the
image under QEMU/OVMF, captures ConOut over the serial console, checks five
conditions and writes `example/ovmf.txt`. Requires QEMU 11.1.0 (or newer) with
its bundled EDK2 blobs; nothing else.

```
py tools\boot_test.py --seconds 10
```

Under the hood it does the following.

1. **Build or grab `efiprobe.efi`.** Tagged releases attach it as an asset.
2. **Put it on an ESP.** A FAT32 directory is enough for QEMU. Putting the
   image at the removable-media path makes OVMF boot it directly, so no shell
   interaction is needed:

   ```
   esp/
   ├── efiprobe.efi
   ├── startup.nsh                 (fallback if the shell starts instead)
   └── EFI/BOOT/BOOTX64.EFI       (copy of efiprobe.efi)
   ```

3. **Boot OVMF.** `edk2-x86_64-code.fd` is a pflash CODE image, not a `-bios`
   ROM — `-bios` fails with `could not load PC BIOS`. It needs a writable VARS
   volume alongside it:

   ```sh
   cp edk2-i386-vars.fd OVMF_VARS.fd
   qemu-system-x86_64 \
     -drive if=pflash,format=raw,unit=0,readonly=on,file=edk2-x86_64-code.fd \
     -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
     -drive format=raw,file=fat:rw:test/esp \
     -m 256 -smp 1 -net none -display none -monitor none \
     -serial file:test/serial.log -no-reboot
   ```

   OVMF's console is on the serial port, which is why `-serial file:` is the
   whole capture mechanism. `-display none` means no window; the app still has
   a real `ConOut`. Terminate QEMU after a few seconds — `efiprobe` returns to
   firmware immediately, so there is nothing to wait for.

4. **Verify.** The first line should be `efiprobe 0.2  |  EDK II  rev 1.0`,
   `memmap` should report a non-zero descriptor count, `config tables` should
   show the ACPI 2.0 RSDP, and `gop` should report a real framebuffer address.
   `boot_test.py` asserts all five, including a `BdsDxe` boot attempt that
   appears after the app banner (a clean return rather than a hang).

Verified result (QEMU 11.1.0, `edk2-stable202408`, UEFI 2.70, 256 MiB guest):

```
efiprobe 0.2  |  EDK II  rev 1.0  |  uefi spec 2.70
------------------------------------------------------------
conout      mode 2  cols 100 rows 31  conout @ 0x000000000E725FD0
loaded image  base 0x000000000DF22000  size 0x00007000  code 01  opts 0  device 0x000000000E5C7D98

config tables  10
  EB9D2D30-2D88-11D3-9A16-0090273FC14D  ACPI 1.0 RSDP  @ 0x000000000F77E000
  8868E871-E4F1-11D3-BC22-0080C73C8881  ACPI 2.0 RSDP  @ 0x000000000F77E014

gop         mode 0 / 30  fb 0x0000000080000000  size 0x0000003E8000  cur 1280x800  bgra8
memmap      105 descriptors  desc 48 bytes  ver 00000001  key 0x0000000000000A23
  address space 12547 MiB (incl. reserved/MMIO)
  conventional     207 MiB
```

Note `desc 48 bytes`: OVMF returns 48-byte descriptors, not the 40 bytes of
`sizeof(EFI_MEMORY_DESCRIPTOR)`. Walking by the returned `DescriptorSize` is
the only correct way to iterate the map — see
[The memory map](#the-memory-map). The `address space` line sums every
descriptor including the 12 GiB reserved MMIO hole above 4 GiB; `conventional`
is the actual RAM figure for this 256 MiB guest. Full output in
[`example/ovmf.txt`](example/ovmf.txt).

---

## Write your own `.efi` application

Copy `src/efi.h` into your own project. That is the only header you need —
nothing from EDK2 is exposed or required.

### Minimum viable application

Smallest thing that compiles, links and prints one line:

```c
#include "efi.h"

static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* out;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    (void)ImageHandle;
    out = SystemTable->ConOut;
    out->ClearScreen(out);
    out->OutputString(out, L"hello from firmware\r\n");
    return EFI_SUCCESS;
}
```

Build it with the same flags as `build.bat` and link with
`/ENTRY:efi_main /SUBSYSTEM:EFI_APPLICATION /NODEFAULTLIB`. `OutputString`
takes `CHAR16*` and does **not** interpret `\n` — use `\r\n`.

### The boot services table

`EFI_BOOT_SERVICES` is a 24-byte header followed by 44 function pointers at
fixed offsets. `efi.h` declares every one as `void*` and casts at the call
site, because only a handful are used and a wrong prototype on an unused slot
is worse than an untyped one. The offsets that matter here:

| Offset | Field | Used by |
| --- | --- | --- |
| `0x18` | `RaiseTPL` … `FreePages` | — |
| `0x38` | `GetMemoryMap` | `memory_map()` |
| `0x40` | `AllocatePool` | `memory_map()` |
| `0x48` | `FreePool` | `memory_map()`, `gop_modes()` |
| `0x98` | `HandleProtocol` | `loaded_image()` |
| `0x140` | `LocateProtocol` | `gop_modes()` |
| `0x178` | end of table | — |

Typed aliases live next to the struct for the ones actually called, e.g.

```c
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
    UINTN*, EFI_MEMORY_DESCRIPTOR*, UINTN*, UINTN*, UINT32*);
```

and a call is

```c
((EFI_GET_MEMORY_MAP)bs->GetMemoryMap)(&size, NULL, &key, &dsize, &dver);
```

That cast is deliberate. `tools/layout.c` asserts all of these offsets on
every build, so if you reorder a field the build fails before you ever boot.

### Protocols and GUIDs

`HandleProtocol` takes an `EFI_HANDLE` and a GUID; `LocateProtocol` takes a
GUID and returns the first matching interface. `efi.h` ships two GUID
constants and a small name table:

```c
static const EFI_GUID gEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2,
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
};
```

GUIDs are a 4/2/2/8 mixed-endian struct on the wire — the first three fields
are little-endian integers, the last eight bytes are in order. `putguid()`
formats them accordingly. When you add a protocol, add its GUID here and, if
it shows up in the configuration table, add a row to `gGuids[]`.

### The memory map

`GetMemoryMap` needs **two calls**:

```c
UINTN size = 0, key = 0, dsize = 0;
UINT32 dver = 0;
EFI_STATUS s = get_map(&size, NULL, &key, &dsize, &dver);
if (s != EFI_BUFFER_TOO_SMALL) { /* bail */ }
size += dsize * 8;            /* AllocatePool itself grows the map */
void* mem = NULL;
allocate_pool(EfiLoaderData, size, &mem);
get_map(&size, mem, &key, &dsize, &dver);
```

Three rules that are easy to get wrong:

- **Walk by the returned `DescriptorSize`, not `sizeof(EFI_MEMORY_DESCRIPTOR)`.**
  Firmware is allowed to pad descriptors. OVMF does: it returns 48 where the
  struct is 40. Hardcoding the struct size reads garbage from the second
  descriptor onward.
- **Add slack after the sizing call.** Allocating the exact size fails because
  `AllocatePool` adds a descriptor of its own.
- **`EFI_MEMORY_RUNTIME`** marks memory that survives `ExitBootServices`.
  Filtering on it is a cheap view of what the OS will inherit.

`QueryMode` allocates its info buffer from boot services too — free it with
`FreePool` or the next memory map is different.

### Freestanding CRT shims

`/NODEFAULTLIB` means `memcpy`, `memset` and `__chkstk` must exist if the
compiler emits them. `src/crt.c` is 12 lines and does exactly that. The
`__chkstk` implementation is intentionally a no-op: no function in this app
has a frame large enough to need page probing. If you add one, implement it
properly or reduce the frame — a silent no-op on a real probe is a crash you
will not be able to debug from a firmware console.

### Struct layout is the whole game

There is no debugger on a firmware console. A wrong pad in
`EFI_SYSTEM_TABLE` gives you a plausible-looking pointer that reads garbage,
and you will spend an afternoon on it. `tools/layout.c` is the defence:

```c
CK(EFI_SYSTEM_TABLE, ConOut, 0x40);
CK(EFI_BOOT_SERVICES, LocateProtocol, 0x140);
CK(EFI_LOADED_IMAGE_PROTOCOL, Unload, 0x58);
```

It compiles as a normal console program, prints `ok`/`BAD` for each field,
and returns non-zero on any mismatch, which fails `build.bat`. Add a `CK` line
for every new struct you introduce. `pecheck.py` covers the other half: it
reads the linked bytes and asserts machine `0x8664`, magic `0x20B`, subsystem
10, a non-zero entry RVA inside a section, and an empty import directory.

---

## Verification

What `build.bat` proves on every run:

| Check | Tool | Asserts |
| --- | --- | --- |
| struct layout | `tools/layout.c` | 29 selected `offsetof` values against UEFI 2.x x64, plus `sizeof` for 9 structs |
| PE validity | `tools/pecheck.py` | `MZ`, `PE\0\0`, AMD64, PE32+, subsystem 10, entry in a section, zero imports |
| freestanding | `link /NODEFAULTLIB` | no unresolved CRT symbols |
| stack probes | `tools/chkstk.py` | no `__chkstk` call in the disassembly, so the no-op shim is safe |
| firmware execution | `tools/boot_test.py` | app banner, ACPI 2.0 RSDP, GOP framebuffer, non-zero descriptor count and returned descriptor size, clean return to firmware under QEMU/OVMF |

The first four are static and run on every `build.bat`. The fifth is the real
boot test; run it once after any change to a protocol call or struct.

Sample build output:

```
[*] layout check
  efi struct layout (x64, msvc)
  ok   EFI_MEMORY_DESCRIPTOR Type                   0x0
  ok   EFI_SYSTEM_TABLE   ConOut                 0x40
  ok   EFI_BOOT_SERVICES  LocateProtocol         0x140
  ... 29 offset checks
  all offsets match uefi 2.x x64
[*] compiling
  efiprobe.c
  crt.c
[*] linking
[*] stack probe check
  ok  no __chkstk calls in the generated code
[*] pe check
  pe      0xC0  sig PE\0\0
  machine 0x8664
  magic   0x20B
  subsys  10 EFI_APPLICATION
  ok  valid freestanding x64 efi application
[+] build\efiprobe.efi
```

Boot test output:

```
  ok    app banner
  ok    config tables
  ok    GOP
  ok    memory map
  ok    returned to firmware
wrote example\ovmf.txt
```

The full 65-line capture is in [`example/ovmf.txt`](example/ovmf.txt).

---

## Release process

CI at `.github/workflows/build.yml` builds on `windows-2022` on every push to
`main` and uploads the image plus a `SHA256SUMS` as an artifact. `build.bat`
locates `vcvars64.bat` itself, so the runner needs no extra setup.
`workflow_dispatch` is enabled too.

A release is cut by hand from a local build:

```sh
build.bat
gh release create v0.2.0 build\efiprobe.efi LICENSE SHA256SUMS
```

`SHA256SUMS` is the checksum line for the built image. `gh release create`
tags the current `main` and attaches the three assets; bump the banner version
in `src/efiprobe.c` with the tag.

---

## License

Unlicense — public domain. Take `src/efi.h`, drop it into your own tree and
delete everything you don't need.
