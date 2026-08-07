import struct
import sys

SUBSYSTEMS = {10: "EFI_APPLICATION", 11: "EFI_BOOT_SERVICE_DRIVER",
              12: "EFI_RUNTIME_DRIVER", 13: "EFI_ROM"}


def fail(msg):
    print("  BAD  " + msg)
    return 1


def main(path):
    d = open(path, "rb").read()
    bad = 0
    if d[:2] != b"MZ":
        return fail("no MZ")
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    if d[pe:pe + 4] != b"PE\0\0":
        return fail("no PE signature at 0x%X" % pe)

    coff = pe + 4
    machine, nsect = struct.unpack_from("<HH", d, coff)
    size_opt = struct.unpack_from("<H", d, coff + 16)[0]
    opt = coff + 20
    magic, = struct.unpack_from("<H", d, opt)
    entry, = struct.unpack_from("<I", d, opt + 16)
    image_base, = struct.unpack_from("<Q", d, opt + 24)
    subsystem, = struct.unpack_from("<H", d, opt + 68)
    numdirs, = struct.unpack_from("<I", d, opt + 108)
    imp_rva, imp_size = struct.unpack_from("<II", d, opt + 112 + 8)

    print("  pe      0x%X  sig PE\\0\\0" % pe)
    print("  machine 0x%04X" % machine)
    print("  magic   0x%03X" % magic)
    print("  entry   0x%08X  base 0x%X" % (entry, image_base))
    print("  subsys  %d %s" % (subsystem, SUBSYSTEMS.get(subsystem, "?")))
    print("  dirs    %d  import rva 0x%X size 0x%X" % (numdirs, imp_rva, imp_size))

    if machine != 0x8664:
        bad += fail("expected AMD64")
    if magic != 0x20B:
        bad += fail("expected PE32+")
    if subsystem != 10:
        bad += fail("expected EFI_APPLICATION subsystem")
    if entry == 0:
        bad += fail("no entry point")
    if imp_size != 0:
        bad += fail("image has imports, should be freestanding")

    sec = opt + size_opt
    sections = []
    for i in range(nsect):
        o = sec + i * 40
        name = d[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
        vsize, va, rawsize, raw = struct.unpack_from("<IIII", d, o + 8)
        sections.append((name, va, vsize, raw, rawsize))
        print("  sect    %-8s rva 0x%-8X vsize 0x%-8X raw 0x%-8X size 0x%X"
              % (name, va, vsize, raw, rawsize))

    if not any(va <= entry < va + max(vs, 1) for _, va, vs, _, _ in sections):
        bad += fail("entry rva outside every section")

    if bad:
        print("  %d problem(s)" % bad)
        return 1
    print("  ok  valid freestanding x64 efi application")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: pecheck.py <image.efi>")
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
