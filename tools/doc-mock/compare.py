#!/usr/bin/env python3
import argparse
import html
import re
import subprocess
import sys
import tempfile
from difflib import SequenceMatcher
from pathlib import Path

import lxml.html

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
TOKEN = re.compile(r"<[^>]+>|[^\s<]+|\s+")


def inner(el):
    text = html.escape(el.text, quote=False) if el.text else ""
    for child in el:
        text += lxml.html.tostring(child, encoding="unicode")
    return text.strip()


def norm(text):
    return re.sub(r"\s+", " ", text).strip()


def text_of(fragment):
    return norm(html.unescape(re.sub(r"<[^>]+>", "", fragment)))


def sentences(fragment):
    parts = re.split(r"(?<=요\.)\s+|(?<=요\?)\s+|(?<=음\.)\s+|(?<=함\.)\s+|(?<=됨\.)\s+", norm(fragment))
    return [part for part in parts if part]


def wrap_tokens(tokens, cls):
    return "".join(t if t.startswith("<") or t.isspace() else f'<span class="{cls}">{t}</span>' for t in tokens)


def word_diff(a, b):
    ta, tb = TOKEN.findall(a), TOKEN.findall(b)
    left, right = [], []
    for op, i1, i2, j1, j2 in SequenceMatcher(None, ta, tb, autojunk=False).get_opcodes():
        if op == "equal":
            left.append("".join(ta[i1:i2]))
            right.append("".join(tb[j1:j2]))
        else:
            left.append(wrap_tokens(ta[i1:i2], "d"))
            right.append(wrap_tokens(tb[j1:j2], "i"))
    return "".join(left), "".join(right)


def pair_lists(a, b, similar):
    left, right = [], []
    matcher = SequenceMatcher(None, [text_of(x) for x in a], [text_of(x) for x in b], autojunk=False)
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == "equal":
            left += a[i1:i2]
            right += b[j1:j2]
            continue
        la, lb = a[i1:i2], b[j1:j2]
        k = 0
        while k < len(la) and k < len(lb) and SequenceMatcher(None, text_of(la[k]), text_of(lb[k])).ratio() > similar:
            l, r = word_diff(la[k], lb[k])
            left.append(l)
            right.append(r)
            k += 1
        left += [f'<span class="sd">{x}</span>' for x in la[k:]]
        right += [f'<span class="si">{x}</span>' for x in lb[k:]]
    return left, right


def code_label(figure):
    path = figure.find_class("path")
    badge = figure.find_class("badge")
    return (path[0].text_content() if path else "") + " · " + (badge[0].text_content() if badge else "")


def blocks(container):
    out = []
    for el in container:
        cls = el.get("class", "")
        if el.tag == "p":
            fragment = inner(el)
            out.append({"t": "p", "key": "p:" + text_of(fragment), "h": fragment})
        elif el.tag == "figure":
            label = code_label(el)
            out.append({"t": "fig", "key": "fig:" + label, "label": label})
        elif el.tag == "details":
            items = []
            for sub in el.iter("li", "p", "figure"):
                if sub.tag == "figure":
                    items.append("〔코드 · " + html.escape(code_label(sub)) + "〕")
                elif sub.getparent().tag != "li":
                    items.append(inner(sub))
            summary = inner(el.find("summary"))
            figs = ["fig:" + code_label(f) for f in el.iter("figure")]
            out.append({"t": "det", "key": "det:" + text_of(summary), "summ": summary,
                        "open": "open" in el.attrib, "items": items, "figs": figs})
        elif el.tag == "div" and ("learn" in cls or "pitfall" in cls.split()):
            fragment = "<br>".join(inner(p) for p in el.iter("p"))
            out.append({"t": "learn", "key": "learn:" + text_of(fragment), "h": fragment})
        elif el.tag == "li":
            label = el.find_class("label")
            what = el.find_class("what")
            spans = el.findall("span")
            fragment = inner(what[0]) if what else (inner(spans[-1]) if len(spans) > 1 else inner(el))
            name = label[0].text_content() if label else ""
            out.append({"t": "check", "key": "chk:" + name, "lab": name, "h": fragment})
    return out


def raw(block):
    if block["t"] == "det":
        return block["summ"] + " ".join(block["items"])
    if block["t"] == "fig":
        return block["label"]
    return block["h"]


def render(block, body=None, state="same", moved=False):
    cls = {"same": "", "del": " del", "ins": " ins", "chg": " chg"}[state]
    if block["t"] == "fig":
        note = " <em>접기 안으로 옮김</em>" if moved else ""
        return f'<div class="chip{"" if moved else cls}">코드 · {html.escape(block["label"])}{note}</div>'
    if block["t"] == "det":
        items = (block["items"] if block["open"] else []) if body is None else body
        mark, state_name = ("▾", "펼침") if block["open"] else ("▸", "접힘")
        listed = "".join(f"<li>{x}</li>" for x in items)
        return (f'<div class="det blk{cls}"><div class="sum">{mark} {block["summ"]} <span class="st">{state_name}</span></div>'
                + (f"<ul>{listed}</ul>" if listed else "") + "</div>")
    fragment = block["h"] if body is None else body
    if block["t"] == "learn":
        return f'<div class="learn blk{cls}">{fragment}</div>'
    if block["t"] == "check":
        return f'<div class="blk{cls}"><b class="lab">{html.escape(block["lab"])}</b> {fragment}</div>'
    return f'<p class="blk{cls}">{fragment}</p>'


def row(left, right):
    return f'<div class="row"><div class="cell">{left}</div><div class="cell">{right}</div></div>'


GAP = '<div class="gap"></div>'


def pair_row(a, b):
    if a["t"] == "det":
        left, right = pair_lists(a["items"], b["items"], 0.0)
        changed = left != a["items"] or right != b["items"]
        if not changed and a["open"] == b["open"] and a["summ"] == b["summ"]:
            return row(render(a), render(b))
        if not a["open"]:
            left = [x for x in left if 'class="d"' in x or 'class="sd"' in x]
        if not b["open"]:
            right = [x for x in right if 'class="i"' in x or 'class="si"' in x]
        return row(render(a, left, "chg"), render(b, right, "chg"))
    if a["t"] == "fig" or a["h"] == b["h"]:
        return row(render(a), render(b))
    left, right = pair_lists(sentences(a["h"]), sentences(b["h"]), 0.55)
    return row(render(a, " ".join(left), "chg"), render(b, " ".join(right), "chg"))


def diff_blocks(old, new):
    moved = {f for b in new if b["t"] == "det" for f in b["figs"]}
    rows = []
    matcher = SequenceMatcher(None, [x["key"] for x in old], [x["key"] for x in new], autojunk=False)
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == "equal":
            rows += [pair_row(old[i], new[j]) for i, j in zip(range(i1, i2), range(j1, j2))]
            continue
        olds, news = list(range(i1, i2)), list(range(j1, j2))
        candidates = sorted(((SequenceMatcher(None, text_of(raw(old[i])), text_of(raw(new[j]))).ratio(), i, j)
                             for i in olds for j in news if old[i]["t"] == new[j]["t"]), reverse=True)
        pairs, used = {}, set()
        for ratio, i, j in candidates:
            if ratio > 0.3 and i not in pairs and j not in used:
                pairs[i] = j
                used.add(j)
        pending = [j for j in news if j not in used]
        for i in olds:
            if i in pairs:
                while pending and pending[0] < pairs[i]:
                    rows.append(row(GAP, render(new[pending.pop(0)], state="ins")))
                rows.append(pair_row(old[i], new[pairs[i]]))
            else:
                was_moved = old[i]["t"] == "fig" and old[i]["key"] in moved
                rows.append(row(render(old[i], state="del", moved=was_moved), GAP))
        rows += [row(GAP, render(new[j], state="ins")) for j in pending]
    return rows


def changed(old, new):
    return [x["key"] for x in old] != [x["key"] for x in new] or any(
        raw(a) != raw(b) or a.get("open") != b.get("open") for a, b in zip(old, new))


def first(doc, xpath):
    found = doc.xpath(xpath)
    return found[0] if found else None


def sections(old_doc, new_doc):
    out = []

    def add(short, title, xpath_old, xpath_new=None):
        a, b = first(old_doc, xpath_old), first(new_doc, xpath_new or xpath_old)
        if a is None or b is None:
            return
        old, new = blocks(a), blocks(b)
        if changed(old, new):
            out.append((short, title, diff_blocks(old, new)))

    add("예측", "먼저 예측", '//section[@id="predict"]')
    for n, step in enumerate(old_doc.xpath('//li[contains(@class,"step")]'), 1):
        title = f"{n}단계 · " + step.xpath(".//h3")[0].text_content()
        add(f"{n}단계", title, f'//li[@id="{step.get("id")}"]/div[@class="step-body"]')
    add("확인", "다 됐는지 확인", '//section[@id="check"]//ul[@class="checks"]')
    add("흔한 실수", "흔한 실수", '//div[@class="pitfalls"]')
    add("바뀐 파일", "바뀐 파일 (안내문)", '//section[@id="files"]')
    add("문제", "확인 문제", '//section[@id="quiz"]')
    add("바꿔 보기", "바꿔 보기", '//section[@id="variants"]//ul')
    return out


def stats(doc):
    body = doc.xpath('//li[contains(@class,"step")]/div[@class="step-body"]')
    count = sum(len(sentences(inner(p))) for b in body for p in b.xpath("./p"))
    visible = 0
    for b in body:
        for el in b:
            if el.tag == "p" or (el.tag == "div" and "learn" in el.get("class", "")) or (el.tag == "details" and "open" in el.attrib):
                visible += len(re.sub(r"\s", "", el.text_content()))
    return count, visible


STYLE = """
:root { --bg:#fff; --fg:#1f2328; --mute:#656d76; --line:#d8dee4; --chip:#f3f4f6; --del:#ffebe9; --delfg:#b42318; --ins:#dafbe1; --insfg:#116329; --chg:#fff8c5; }
@media (prefers-color-scheme: dark) { :root { --bg:#16181d; --fg:#e6e8eb; --mute:#9aa4b0; --line:#30363d; --chip:#22262d; --del:#3d1f22; --delfg:#ff9b93; --ins:#1b3325; --insfg:#7ee2a0; --chg:#3a3320; } }
* { box-sizing:border-box; }
body { margin:0; background:var(--bg); color:var(--fg); font:15px/1.75 "Pretendard","Malgun Gothic",system-ui,sans-serif; }
main { max-width:1320px; margin:0 auto; padding:24px 16px 80px; }
h1 { font-size:22px; margin:0 0 6px; }
.sub { color:var(--mute); margin:0 0 16px; }
.summary { border:1px solid var(--line); border-radius:10px; padding:14px 18px; margin-bottom:18px; }
.nums { display:flex; gap:28px; flex-wrap:wrap; margin-bottom:8px; }
.nums b { font-size:20px; }
.legend span { margin-right:14px; white-space:nowrap; }
.toc { position:sticky; top:0; z-index:5; background:var(--bg); border-bottom:1px solid var(--line); padding:8px 0 6px; }
.toc .links { display:flex; gap:6px; flex-wrap:wrap; margin-bottom:6px; }
.toc a { display:inline-block; min-width:30px; text-align:center; padding:2px 8px; border:1px solid var(--line); border-radius:6px; color:inherit; text-decoration:none; }
.colhead { display:grid; grid-template-columns:1fr 1fr; gap:16px; font-weight:700; color:var(--mute); }
section { margin-top:28px; }
h2 { font-size:18px; border-bottom:2px solid var(--line); padding-bottom:4px; }
.row { display:grid; grid-template-columns:1fr 1fr; gap:16px; border-bottom:1px dashed var(--line); padding:8px 0; }
.cell { min-width:0; }
.blk { margin:0; padding:6px 10px; border-radius:6px; }
.blk.del { background:var(--del); color:var(--delfg); text-decoration:line-through; }
.blk.ins { background:var(--ins); }
.blk.chg { background:var(--chg); }
.d, .sd { background:var(--del); color:var(--delfg); text-decoration:line-through; border-radius:3px; }
.i, .si { background:var(--ins); color:var(--insfg); font-weight:600; border-radius:3px; }
.chip { display:inline-block; font:12.5px/1.6 ui-monospace,Consolas,monospace; background:var(--chip); color:var(--mute); border:1px solid var(--line); border-radius:6px; padding:3px 10px; }
.chip.del { text-decoration:line-through; color:var(--delfg); background:var(--del); }
.chip.ins { background:var(--ins); color:var(--insfg); }
.chip em { font-style:normal; color:var(--fg); font-family:"Pretendard","Malgun Gothic",sans-serif; }
.det { border:1px solid var(--line); }
.det .sum { font-weight:600; font-size:14px; }
.det .st { font-weight:400; font-size:12px; color:var(--mute); border:1px solid var(--line); border-radius:10px; padding:0 7px; margin-left:4px; }
.det ul { margin:6px 0 0; padding-left:18px; font-size:14px; }
.learn { border-left:3px solid #8b5cf6; }
.lab { display:inline-block; min-width:70px; }
code { font:0.88em ui-monospace,Consolas,monospace; background:var(--chip); padding:0 4px; border-radius:4px; }
.gap { height:100%; min-height:20px; background:repeating-linear-gradient(45deg,transparent 0 6px,var(--chip) 6px 12px); border-radius:6px; opacity:.6; }
@media (max-width:760px) { .row, .colhead { grid-template-columns:1fr; } .gap { display:none; } }
"""


def page(name, old_label, parts, old_stats, new_stats):
    toc = "".join(f'<a href="#s{i}">{html.escape(short)}</a>' for i, (short, _, _) in enumerate(parts))
    body = "".join(f'<section id="s{i}"><h2>{html.escape(title)}</h2>{"".join(rows)}</section>'
                   for i, (_, title, rows) in enumerate(parts))
    if not parts:
        body = '<p class="sub">바뀐 곳이 없습니다.</p>'
    return f"""<!doctype html>
<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>{html.escape(name)} 비교</title><style>{STYLE}</style></head><body><main>
<h1>{html.escape(name)} : {html.escape(old_label)} 와 지금 판</h1>
<p class="sub">바뀐 곳이 있는 절만 보입니다. 코드는 회색 칩으로 줄였고, 접힌 상자는 안에서 바뀐 줄만 보입니다.</p>
<div class="summary">
<div class="nums"><div>따라 하기 본문 문장<br><b>{old_stats[0]} → {new_stats[0]}</b></div><div>접지 않고 보이는 글자 (공백 제외)<br><b>{old_stats[1]:,} → {new_stats[1]:,}</b></div></div>
<div class="legend"><span><span class="sd">빨간 취소선</span> 뺀 것</span><span><span class="si">초록</span> 새로 쓴 것</span></div>
</div>
<div class="toc"><div class="links">{toc}</div><div class="colhead"><div>{html.escape(old_label)}</div><div>지금 판</div></div></div>
{body}
</main></body></html>"""


def main():
    parser = argparse.ArgumentParser(
        description="교재 원고 두 판의 산문을 절별로 나란히 비교하는 HTML 을 만든다. 규칙은 docs/DOC_STYLE.md.",
        epilog="예 : python tools/doc-mock/compare.py tools/doc-mock/template-02.html  (마지막 커밋과 작업 중인 파일을 비교)")
    parser.add_argument("file", help="지금 판 원고 (template-NN.html)")
    parser.add_argument("--rev", default="HEAD", help="이전 판을 읽을 git 리비전. 기본값 HEAD")
    parser.add_argument("--old", help="이전 판 파일. 주면 --rev 대신 이 파일과 비교")
    parser.add_argument("-o", "--out", help="결과 HTML 경로. 기본값은 임시 폴더")
    args = parser.parse_args()

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    new_path = Path(args.file).resolve()
    if args.old:
        old_source = Path(args.old).read_text(encoding="utf-8")
        old_label = Path(args.old).name
    else:
        relative = new_path.relative_to(REPO).as_posix()
        result = subprocess.run(["git", "-C", str(REPO), "show", f"{args.rev}:{relative}"],
                                capture_output=True, text=True, encoding="utf-8")
        if result.returncode != 0:
            print(result.stderr.strip())
            return 1
        old_source = result.stdout
        old_label = args.rev

    old_doc = lxml.html.fromstring(old_source)
    new_doc = lxml.html.fromstring(new_path.read_text(encoding="utf-8"))
    parts = sections(old_doc, new_doc)

    out = Path(args.out) if args.out else Path(tempfile.gettempdir()) / f"{new_path.stem}-compare.html"
    out.write_text(page(new_path.name, old_label, parts, stats(old_doc), stats(new_doc)), encoding="utf-8")
    print(f"비교 : {out}")
    print(f"바뀐 절 {len(parts)}개 : " + (", ".join(short for short, _, _ in parts) or "없음"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
