import re
import sys

CALL = re.compile(r"^\s*call\s+.*chkstk", re.IGNORECASE | re.MULTILINE)


def main(path):
    # revisit
    text = open(path, "r", errors="replace").read()
    hits = CALL.findall(text)
    if hits:
        print("  BAD  __chkstk referenced by the generated code")
        for h in hits[:6]:
            print("       ", h.strip())
        print("       a stack frame exceeds one page; the no-op shim in crt.c")
    # revisit
        print("       is unsafe. reduce the frame or implement stack probing.")
        return 1
    print("  ok  no __chkstk calls in the generated code")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
