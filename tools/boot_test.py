import argparse
import pathlib
import re
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
SHARE_CANDIDATES = [
    pathlib.Path(r"C:\Program Files\qemu\share"),
    pathlib.Path("/usr/share/OVMF"),
    pathlib.Path("/usr/share/qemu"),
]


def find_qemu(explicit):
    if explicit:
        return explicit
    found = shutil.which("qemu-system-x86_64")
    if found:
        return found
    for p in (pathlib.Path(r"C:\Program Files\qemu\qemu-system-x86_64.exe"),):
        if p.exists():
            return str(p)
    sys.exit("qemu-system-x86_64 not found; pass --qemu")


def find_blob(explicit, names):
    if explicit:
        return pathlib.Path(explicit)
    for base in SHARE_CANDIDATES:
        for n in names:
            p = base / n
            if p.exists():
                return p
    sys.exit("firmware blob not found (%s); pass --code / --vars" % ", ".join(names))


def clean(raw):
    txt = re.sub(r"\x1b\[[0-9;=?]*[A-Za-z]", "", raw.decode("utf-8", "replace"))
    txt = txt.replace("\r", "")
    lines = [l.rstrip() for l in txt.splitlines()]
    for i, l in enumerate(lines):
        if 'starting Boot0000 "UiApp"' in l:
            lines = lines[:i + 1]
            break
    while lines and not lines[-1]:
        lines.pop()
    return "\n".join(lines) + "\n"


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser()
    ap.add_argument("--qemu")
    ap.add_argument("--code")
    ap.add_argument("--vars")
    ap.add_argument("--seconds", type=float, default=10)
    ap.add_argument("--out", default="example/ovmf.txt")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()

    qemu = find_qemu(args.qemu)
    code = find_blob(args.code, ["edk2-x86_64-code.fd", "OVMF_CODE.fd", "OVMF.fd"])
    varstpl = find_blob(args.vars, ["edk2-i386-vars.fd", "OVMF_VARS.fd"])
    image = ROOT / "build" / "efiprobe.efi"
    if not image.exists():
        sys.exit("build\\efiprobe.efi missing; run build.bat first")

    test = ROOT / "test"
    esp = test / "esp"
    (esp / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy(image, esp / "efiprobe.efi")
    shutil.copy(image, esp / "EFI" / "BOOT" / "BOOTX64.EFI")
    (esp / "startup.nsh").write_bytes(b"fs0:\r\nefiprobe.efi > out.txt\r\n")
    vars_fd = test / "OVMF_VARS.fd"
    shutil.copy(varstpl, vars_fd)
    serial = test / "serial.log"
    if serial.exists():
        serial.unlink()

    fwd = lambda p: str(p).replace("\\", "/")
    cmd = [
        qemu,
        "-drive", "if=pflash,format=raw,unit=0,readonly=on,file=" + fwd(code),
        "-drive", "if=pflash,format=raw,unit=1,file=" + fwd(vars_fd),
        "-drive", "format=raw,file=fat:rw:test/esp",
        "-m", "256", "-smp", "1", "-net", "none",
        "-display", "none", "-monitor", "none",
        "-serial", "file:test/serial.log",
        "-no-reboot",
    ]
    print("qemu   :", qemu)
    print("code   :", code)
    print("vars   :", varstpl)
    print("running: %.0fs" % args.seconds)

    p = subprocess.Popen(cmd, cwd=str(ROOT), stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    try:
        time.sleep(args.seconds)
    finally:
        if p.poll() is None:
            p.kill()
        p.wait(timeout=10)

    if not serial.exists() or not serial.stat().st_size:
        print(p.stdout.read().decode("utf-8", "replace")[:2000])
        sys.exit("no serial output; firmware did not reach the console")
    text = clean(serial.read_bytes())
    out = ROOT / args.out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8")

    lines = text.splitlines()
    banner_at = next((i for i, l in enumerate(lines) if "efiprobe 0.2" in l), None)
    returned = banner_at is not None and any(
        "BdsDxe: loading" in l for l in lines[banner_at + 1:]
    )
    checks = [
        ("app banner", banner_at is not None),
        ("config tables", "config tables" in text and "ACPI 2.0 RSDP" in text),
        ("GOP", "gop" in text and "fb 0x" in text),
        ("memory map", re.search(
            r"memmap\s+([1-9][0-9]*)\s+descriptors\s+desc\s+([0-9]+)\s+bytes",
            text) is not None),
        ("returned to firmware", returned),
    ]
    print("")
    for name, ok in checks:
        print("  %s  %s" % ("ok  " if ok else "BAD ", name))
    print("")
    print(text)
    print("wrote", out.relative_to(ROOT))
    if not args.keep:
        shutil.rmtree(esp, ignore_errors=True)
        vars_fd.unlink(missing_ok=True)
        serial.unlink(missing_ok=True)
    return 0 if all(ok for _, ok in checks) else 1


if __name__ == "__main__":
    sys.exit(main())
