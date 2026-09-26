import os
import pathlib
import re
import sys

if "ACC_WT" not in os.environ or "ACC_STEPS" not in os.environ:
    raise SystemExit("ACC_WT(worktree) 와 ACC_STEPS(dump_steps.py 결과) 환경 변수가 필요함. 사용법은 README.md")

WT = pathlib.Path(os.environ["ACC_WT"])
STEPS = pathlib.Path(os.environ["ACC_STEPS"]).read_text(encoding="utf-8").split("\n")
CRLF_SUFFIXES = (".h", ".cpp", ".rc", ".sln", ".hlsl", ".vs", ".ps")


def blocks():
    out = []
    caption = ""
    i = 0
    while i < len(STEPS):
        line = STEPS[i]
        if line.startswith("CAPTION: "):
            caption = line[len("CAPTION: "):]
        if line.startswith("```") and len(line) > 3:
            body = []
            i += 1
            while STEPS[i] != "```":
                body.append(STEPS[i])
                i += 1
            out.append((line[3:], caption, body))
            caption = ""
        i += 1
    return out


def read(path):
    p = WT / path
    return p.read_bytes().decode("utf-8").replace("\r\n", "\n").split("\n") if p.exists() else []


def write(path, lines):
    p = WT / path
    p.parent.mkdir(parents=True, exist_ok=True)
    text = "\n".join(lines)
    if p.suffix in CRLF_SUFFIXES or p.name.endswith((".vcxproj", ".filters")):
        text = text.replace("\n", "\r\n")
    p.write_bytes(text.encode("utf-8"))


def brace_end(lines, start):
    depth = 0
    opened = False
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                opened = True
            elif ch == "}":
                depth -= 1
        if opened and depth <= 0:
            return i
    raise SystemExit(f"unbalanced from line {start + 1}")


def apply_diff(lines, body):
    hunks = []
    cur = None
    for b in body:
        if b.startswith("@@"):
            cur = []
            hunks.append(cur)
        elif cur is not None:
            cur.append(b)
    for h in hunks:
        old = [x[1:] for x in h if x[:1] in (" ", "-") or x == ""]
        new = [x[1:] for x in h if x[:1] in (" ", "+") or x == ""]
        while old and old[-1] == "" and new and new[-1] == "":
            old.pop()
            new.pop()
        found = [i for i in range(len(lines) - len(old) + 1) if lines[i:i + len(old)] == old]
        if len(found) != 1:
            raise SystemExit(f"hunk context matched {len(found)} times:\n" + "\n".join(old[:6]))
        i = found[0]
        lines[i:i + len(old)] = new
    return lines


def find_one(lines, anchor):
    idx = [i for i, l in enumerate(lines) if anchor in l]
    if len(idx) != 1:
        raise SystemExit(f"anchor matched {len(idx)} times: {anchor}")
    return idx[0]


def main():
    cmd = sys.argv[1]
    if cmd == "move":
        src, dst = WT / sys.argv[2], WT / sys.argv[3]
        if not src.exists() or dst.exists():
            raise SystemExit(f"cannot move {sys.argv[2]} -> {sys.argv[3]}")
        dst.parent.mkdir(parents=True, exist_ok=True)
        src.rename(dst)
        print(f"move {sys.argv[2]} -> {sys.argv[3]}")
        return

    bl = blocks()
    if cmd == "list":
        for n, (k, c, b) in enumerate(bl):
            print(f"{n:2d} {k:5s} {c} | {b[0][:60] if b else ''}")
        return
    if cmd == "show":
        k, c, b = bl[int(sys.argv[2])]
        print(c)
        print("\n".join(b))
        return

    path = sys.argv[2]
    kind, caption, body = bl[int(sys.argv[3])]
    lines = read(path)
    if cmd == "write":
        lines = body + [""]
    elif cmd == "diff":
        lines = apply_diff(lines, body)
    elif cmd == "func":
        head = next(l for l in body if l.strip() and not l.lstrip().startswith("//"))
        idx = [i for i, l in enumerate(lines) if l == head]
        if len(idx) == 0:
            name = re.search(r"([A-Za-z_][\w:~]*)\(", head).group(1)
            idx = [i for i, l in enumerate(lines)
                   if re.match(r"^[\w:<>&\* ]*\b" + re.escape(name) + r"\(", l) and not l.rstrip().endswith(";")]
            print(f"  (head differs; matched by name {name})")
        if len(idx) != 1:
            raise SystemExit(f"function head matched {len(idx)} times: {head}")
        s = idx[0]
        while s > 0 and lines[s - 1].lstrip().startswith("//"):
            s -= 1
        e = brace_end(lines, idx[0])
        lines[s:e + 1] = body
    elif cmd == "append":
        while lines and lines[-1] == "":
            lines.pop()
        lines += [""] + body + [""]
    elif cmd == "after":
        e = brace_end(lines, find_one(lines, sys.argv[4]))
        lines[e + 1:e + 1] = [""] + body
    elif cmd == "afterline":
        i = find_one(lines, sys.argv[4])
        lines[i + 1:i + 1] = body
    elif cmd == "afterlineblank":
        i = find_one(lines, sys.argv[4])
        lines[i + 1:i + 1] = [""] + body
    elif cmd == "aftercase":
        i = find_one(lines, sys.argv[4])
        e = next(j for j in range(i, len(lines)) if lines[j].strip() == "return 0;")
        lines[e + 1:e + 1] = [""] + body
    else:
        raise SystemExit(f"unknown command: {cmd}")
    write(path, lines)
    print(f"{cmd} {path} <- block {sys.argv[3]} ({caption})")


main()
