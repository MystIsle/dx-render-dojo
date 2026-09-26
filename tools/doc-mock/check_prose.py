#!/usr/bin/env python3
"""교재 원고(template-NN.html)의 기계 점검.

사람이 읽어야 아는 것은 다루지 않는다. 셀 수 있는 것만 센다 : 마커, 따라 하기 조각의 빠짐없음, 앵커, 태그 짝, 어미, 금지 표현, 리듬 수치.
규칙의 출처는 docs/DOC_STYLE.md 다.

사용법 :
    python tools/doc-mock/check_prose.py template-02.html --tag tut02  # 마커가 가리키는 경로·심볼을 git 태그에서 확인
    python tools/doc-mock/check_prose.py tutorial-02.html --built  # 생성 결과에 남은 마커가 없는지

오류가 하나라도 있으면 종료 코드 1. 경고는 종료 코드에 영향을 주지 않는다.
"""

import argparse
import html
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent

# 값은 "한 편에서 이 횟수를 넘으면 경고". 0 이면 나오기만 해도 경고.
PHRASE_LIMITS = {
    "그래서": 2,
    "여기서는": 2,
    "자리": 2,
    "아니라": 1,  # 부정 대구 `A가 아니라 B` 는 문서당 1회
}

# 나오면 경고하는 표현. (이름, 정규식)
PHRASE_PATTERNS = [
    ("메타 담화", r"살펴보겠|살펴봅시다|알아보겠|알아봅시다|요약하면|결론적으로|정리하면"),
    ("강조 예고", r"핵심은|핵심이에요|중요한 것은|중요한 건|중요해요"),
    ("실제 동작을 가린 말", r"(방식|구조)을 (씁니다|써요)"),
    ("이중 피동", r"되어지|보여지|쓰여지"),
    ("빼도 되는 한자어 술어", r"수행(합|해)|진행(합|해)|실시(합|해)"),
    ("술어 비유", r"깔아 |닿[아을는게]|쥐[고는어]|얹[어는고]|건드리|(?<!손을 )떼[고면]"),
    ("주석을 의인화", r"주석이 말하"),
    ("원문 대비를 본문에", r"원문은 [^.]*(지만|는데),? 여기서는"),
]

PAIRED_TAGS = ["details", "summary", "figure", "figcaption", "section", "ul", "ol", "li", "div", "aside", "p", "pre"]

MARKER = re.compile(r"<!--(INCLUDE|FILE|SYMBOL|DIFF|SLICE|TOC)(?:@(\d+))?:(.*?)-->", re.S)
TOC_NAMES = ("eyebrow", "base", "pager", "progress", "count", "stages", "rail")

# 빠짐없음을 세는 파일. 프로젝트 파일은 솔루션 탐색기로 바뀌는 것이라 본문이 언급하는지만 본다.
CODE_PATH = re.compile(r"^(Source/|CMakeLists\.txt$|[^/]+\.(manifest|rc)$)")
PROJECT_FILE = re.compile(r"\.vcxproj(\.filters)?$")
ACTION_BADGES = ("신규", "수정", "삭제")


class Report:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.info = []

    def error(self, text):
        self.errors.append(text)

    def warn(self, text):
        self.warnings.append(text)

    def note(self, text):
        self.info.append(text)


def strip_tags(fragment):
    return re.sub(r"\s+", " ", html.unescape(re.sub(r"<[^>]+>", "", fragment))).strip()


def split_sentences(text):
    return [part.strip() for part in re.split(r"(?<=[.?!])\s+", text) if part.strip()]


def git_run(*args):
    result = subprocess.run(["git", "-C", str(REPO), *args], capture_output=True, text=True, encoding="utf-8")
    return result.stdout if result.returncode == 0 else None


def git_lines(tag, path):
    output = git_run("show", f"{tag}:{path}")
    return None if output is None else output.split("\n")


def symbol_bounds(lines, symbol):
    start = next((i for i, line in enumerate(lines) if symbol in line), -1)
    if start < 0:
        return None
    first = start
    while first > 0 and lines[first - 1].lstrip().startswith("//"):
        first -= 1
    depth, opened = 0, False
    for i in range(start, len(lines)):
        depth += lines[i].count("{") - lines[i].count("}")
        opened = opened or "{" in lines[i]
        if opened and depth <= 0:
            return first, i
    return None


def slice_bounds(lines, begin, until):
    start = next((i for i, line in enumerate(lines) if begin in line), -1)
    if start < 0:
        return None
    stop = next((i for i in range(start + 1, len(lines)) if until in lines[i]), -1)
    if stop < 0:
        return None
    first = start
    while first > 0 and lines[first - 1].lstrip().startswith("//"):
        first -= 1
    last = stop - 1
    while last > start and (lines[last].strip() == "" or lines[last].lstrip().startswith("//")):
        last -= 1
    return first, last


def code_blocks(lines):
    """중괄호 블록의 (머리 줄, 끝 줄, 네임스페이스인지). 문자열과 // 주석 안의 중괄호는 세지 않는다."""
    blocks, stack = [], []
    for i, line in enumerate(lines):
        code = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', "", line)
        code = code.split("//", 1)[0]
        for char in code:
            if char == "{":
                head = i
                if code.strip() == "{":
                    head = i - 1
                    while head > 0 and lines[head].strip() == "":
                        head -= 1
                stack.append((head, lines[head].strip().startswith("namespace")))
            elif char == "}" and stack:
                head, is_namespace = stack.pop()
                blocks.append((head, i, is_namespace))
    return blocks


def tag_ref(tag, step=""):
    ref = tag if tag.startswith("refs/") else f"refs/tags/{tag}"
    return f"{ref}-s{step}" if step else ref


def check_markers(source, tag, built, report):
    markers = MARKER.findall(source)
    kinds = {}
    for kind, _, _ in markers:
        kinds[kind] = kinds.get(kind, 0) + 1
    report.note("마커 : " + (", ".join(f"{k} {v}" for k, v in sorted(kinds.items())) or "없음"))

    for kind, _, body in markers:
        if kind == "SYMBOL" and ":" not in body:
            report.error(f"SYMBOL 마커에 심볼이 없음 : {body}")
        if kind == "SLICE" and not re.fullmatch(r".+?:.+?=>.+", body, re.S):
            report.error(f"SLICE 마커 형식이 `경로:시작=>끝` 이 아님 : {body}")
        if kind == "DIFF" and ":" in body and not re.fullmatch(r"[^:>\n]+?:.+?=>.+", body, re.S):
            report.error(f"범위 DIFF 마커 형식이 `경로:시작=>끝` 이 아님 : {body}")
        if kind == "TOC" and body not in TOC_NAMES:
            report.error(f"TOC 마커 이름을 모름 : {body}")

    if built and markers:
        report.error(f"생성 결과에 채워지지 않은 마커 {len(markers)}개")
    if tag is None or built:
        return

    cache = {}

    def lines_of(ref, path):
        if (ref, path) not in cache:
            cache[(ref, path)] = git_lines(ref, path)
        return cache[(ref, path)]

    for kind, step, body in markers:
        if kind == "TOC":
            continue
        if kind == "INCLUDE":
            if not (HERE / body).exists():
                report.error(f"INCLUDE 대상이 없음 : {body}")
            continue
        ref = tag_ref(tag, step)
        path = body.split(":", 1)[0]
        lines = lines_of(ref, path)
        if lines is None:
            report.error(f"{kind} : {ref} 에 없는 경로 : {path}")
            continue
        if kind == "SYMBOL":
            symbol = body.split(":", 1)[1]
            if not any(symbol in line for line in lines):
                report.error(f"SYMBOL : {path} 에 없는 문자열 : {symbol}")
        if kind == "SLICE" or (kind == "DIFF" and ":" in body):
            begin, until = body.split(":", 1)[1].split("=>", 1)
            start = next((i for i, line in enumerate(lines) if begin in line), -1)
            if start < 0:
                report.error(f"{kind} : {path} 에 없는 시작 문자열 : {begin}")
            elif not any(until in line for line in lines[start + 1 :]):
                report.error(f"{kind} : {path} 에서 시작 줄 뒤에 끝 문자열이 없음 : {until}")


def step_pieces(steps, tag):
    """따라 하기 절의 코드 조각을 문서 순서대로 돌려준다. (종류, 경로, 첫 줄, 끝 줄, 배지)"""
    pieces = []
    for figure in re.finditer(r"<figure\b.*?</figure>", steps, re.S):
        badges = re.findall(r'<span class="badge[^"]*">\s*([^<]*?)\s*</span>', figure.group(0))
        action = next((b for b in badges if b in ACTION_BADGES), None)
        for kind, _, body in MARKER.findall(figure.group(0)):
            if kind == "INCLUDE":
                continue
            path = body.split(":", 1)[0]
            lines = git_lines(tag, path)
            if lines is None:
                continue
            if kind == "FILE" or (kind == "DIFF" and ":" not in body):
                bounds = (0, len(lines) - 1)
            elif kind == "SYMBOL":
                bounds = symbol_bounds(lines, body.split(":", 1)[1])
            else:
                begin, until = body.split(":", 1)[1].split("=>", 1)
                bounds = slice_bounds(lines, begin, until)
            if bounds is not None:
                label = "DIFF범위" if kind == "DIFF" and ":" in body else kind
                pieces.append((label, path, *bounds, action))
    return pieces


def renamed_paths(base, tag):
    """옮긴 파일의 새 경로에서 옛 경로로 가는 표."""
    renames = {}
    for line in (git_run("diff", "-M", "--name-status", base, tag) or "").split("\n"):
        parts = line.split("\t")
        if len(parts) == 3 and parts[0].startswith("R"):
            renames[parts[2]] = parts[1]
    return renames


def changed_lines(base, tag, path, old_path=None):
    """이전 태그에서 이 태그로 오며 추가된 줄 번호와 지운 줄 묶음. 공백만 바뀐 줄은 뺀다.
    옮긴 파일은 옛 경로를 함께 넘겨야 git 이 짝을 지어 바뀐 줄만 돌려준다."""
    added, removed = set(), []
    paths = [old_path, path] if old_path else [path]
    output = git_run("diff", "-M", "-w", "--ignore-blank-lines", "-U0", base, tag, "--", *paths) or ""
    for line in output.split("\n"):
        match = re.match(r"@@ -\d+(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", line)
        if not match:
            continue
        old_count = int(match.group(1)) if match.group(1) is not None else 1
        new_start = int(match.group(2))
        new_count = int(match.group(3)) if match.group(3) is not None else 1
        added.update(range(new_start - 1, new_start - 1 + new_count))
        if old_count:
            around = range(new_start - 1, new_start - 1 + new_count) if new_count else range(new_start - 1, new_start + 1)
            removed.append((set(around), old_count))
    return added, removed


def spans_text(numbers):
    spans, run = [], []
    for number in sorted(numbers):
        if run and number != run[-1] + 1:
            spans.append(run)
            run = []
        run.append(number)
    if run:
        spans.append(run)
    return ", ".join(str(s[0] + 1) if len(s) == 1 else f"{s[0] + 1}-{s[-1] + 1}" for s in spans)


def step_blocks(steps):
    """따라 하기 절을 단계(<li class="step">)마다 자른다."""
    starts = [match.start() for match in re.finditer(r'<li class="step"', steps)]
    return [steps[start:end] for start, end in zip(starts, starts[1:] + [len(steps)])]


def check_follow_along(source, tag, report):
    ref = tag_ref(tag)
    # 앞 편 태그는 목차에서 읽는다. 원문에 없는 편은 번호로 앞 편을 알 수 없다.
    toc = json.loads((HERE / "toc.json").read_text(encoding="utf-8"))
    bases = {lesson["tag"]: lesson["base"] for stage in toc["stages"] for lesson in stage["lessons"]}
    base_tag = bases.get(ref.rsplit("/", 1)[-1])
    base = f"refs/tags/{base_tag}" if base_tag else None
    if base is None or git_run("rev-parse", "--verify", "--quiet", base) is None:
        report.note("빠짐없음 : 이전 태그가 없어 건너뜀")
        return

    steps_match = re.search(r'<section id="steps".*?</section>', source, re.S)
    if not steps_match:
        return
    steps = steps_match.group(0)

    if not any(step for _, step, _ in MARKER.findall(steps)):
        check_range(steps, base, ref, "", report)
        return

    # 단계 커밋 사슬. N단계 조각을 모으면 N-1단계에서 N단계로 온 diff 와 같아야 한다.
    blocks = step_blocks(steps)
    last = tag_ref(tag, str(len(blocks)))
    if git_run("rev-parse", "--verify", "--quiet", last) is None:
        report.error(f"단계 커밋 : 마지막 단계 태그가 없음 : {last}")
        return
    code_names = [
        name
        for name in (git_run("diff", "--name-only", base, ref) or "").split("\n")
        if name and (CODE_PATH.search(name) or PROJECT_FILE.search(name))
    ]
    leftover = [name for name in (git_run("diff", "--name-only", last, ref, "--", *code_names) or "").split("\n") if name]
    if leftover:
        report.error(f"단계 커밋 : 마지막 단계와 {ref.rsplit('/', 1)[-1]} 의 코드가 다름 : " + ", ".join(leftover))

    for number, block in enumerate(blocks, 1):
        strays = sorted({f"{kind}@{step or '없음'}" for kind, step, _ in MARKER.findall(block) if kind != "INCLUDE" and step != str(number)})
        if strays:
            report.error(f"{number}단계 : 다른 단계를 가리키거나 단계가 없는 마커 : " + ", ".join(strays))
        step_base = tag_ref(tag, str(number - 1)) if number > 1 else base
        check_range(block, step_base, tag_ref(tag, str(number)), f"{number}단계 ", report)


def check_range(steps, base, ref, label, report):
    """base 에서 ref 로 온 코드 변경이 steps 의 조각에 빠짐없이 나오는지, 나눈 조각의 순서가 소스와 같은지."""
    pieces = step_pieces(steps, ref)

    unbadged = sum(1 for piece in pieces if piece[4] is None)
    if unbadged:
        report.warn(f"{label}배지(신규·수정·삭제)가 없는 코드 조각 {unbadged}개")

    names = [name for name in (git_run("diff", "-M", "--name-only", base, ref) or "").split("\n") if name]
    renames = renamed_paths(base, ref)
    steps_text = strip_tags(steps)
    if any(PROJECT_FILE.search(name) for name in names) and "vcxproj" not in steps_text:
        report.error(f"{label}빠짐없음 : 프로젝트 파일이 바뀌었는데 따라 하기 본문이 언급하지 않음")
    unmentioned = sorted(Path(new).name for new in renames if CODE_PATH.search(new) and Path(new).name not in steps_text)
    if unmentioned:
        report.error(f"{label}빠짐없음 : 옮긴 파일을 따라 하기 본문이 언급하지 않음 : " + ", ".join(unmentioned))

    total_added = total_missing = total_removed = total_removed_missing = 0
    for path in names:
        if not CODE_PATH.search(path) or PROJECT_FILE.search(path):
            continue
        lines = git_lines(ref, path) or []
        covered, removal_covered = set(), set()
        for kind, piece_path, first, last, action in pieces:
            if piece_path != path:
                continue
            covered.update(range(first, last + 1))
            # 지운 줄은 diff 조각이나 수정 배지를 단 통째 조각만 덮는다. 통째로 보이기만 해서는 무엇을 지웠는지 드러나지 않는다.
            if kind in ("DIFF", "DIFF범위") or action == "수정":
                removal_covered.update(range(first, last + 2))
        added, removed = changed_lines(base, ref, path, renames.get(path))
        added = {i for i in added if i < len(lines) and lines[i].strip() != ""}
        missing = added - covered
        removed_missing = [(around, count) for around, count in removed if not (around & removal_covered)]
        total_added += len(added)
        total_missing += len(missing)
        total_removed += sum(count for _, count in removed)
        total_removed_missing += sum(count for _, count in removed_missing)
        if missing:
            report.error(f"{label}빠짐없음 : {path} 추가 줄 {len(missing)}개가 조각에 없음 [{spans_text(missing)}]")
        if removed_missing:
            where = spans_text({min(around) for around, _ in removed_missing})
            report.error(f"{label}빠짐없음 : {path} 지운 줄 {sum(c for _, c in removed_missing)}개가 조각에 없음 [새 파일 {where}줄 근처]")
    report.note(f"{label}빠짐없음 : 추가 {total_added}줄 중 빠짐 {total_missing}, 지운 {total_removed}줄 중 빠짐 {total_removed_missing}")

    check_piece_order(pieces, ref, label, report)
    check_badges(steps, base, ref, label, report)
    check_range_start(steps, base, ref, label, report)


def check_piece_order(pieces, ref, label, report):
    """한 덩어리(함수 본문, 클래스 선언, 파일 머리)를 나눈 조각은 문서 순서와 소스 순서가 같아야 한다. 네임스페이스는 덩어리로 치지 않는다."""
    groups = {}
    for index, (kind, path, first, last, _) in enumerate(pieces):
        if kind not in ("SLICE", "DIFF범위"):
            continue
        lines = git_lines(ref, path) or []
        enclosing = [
            (head, end)
            for head, end, is_namespace in code_blocks(lines)
            if not is_namespace and head <= first and last <= end and not (first <= head and end <= last)
        ]
        key = (path, max(enclosing) if enclosing else None)
        groups.setdefault(key, []).append((index, first))
    for (path, block), members in groups.items():
        starts = [first for _, first in members]
        if starts != sorted(starts):
            lines = git_lines(ref, path) or []
            where = lines[block[0]].strip() if block else "파일 머리"
            order = ", ".join(str(s + 1) for s in starts)
            report.error(f"{label}덩어리 순서 : {path} `{where}` 의 조각 순서가 소스와 다름 (문서 순서의 시작 줄 {order})")


def check_badges(steps, base, ref, label, report):
    """조각의 배지가 조각 종류와 맞는지. diff 조각은 늘 `수정`. 파일 전체 `신규` 는 앞 단계에 없던 파일, `수정` 은 있던 파일. 함수 `신규` 는 앞 단계에 없던 함수."""
    renames = renamed_paths(base, ref)
    for figure in re.finditer(r"<figure\b.*?</figure>", steps, re.S):
        badges = re.findall(r'<span class="badge[^"]*">\s*([^<]*?)\s*</span>', figure.group(0))
        action = next((b for b in badges if b in ACTION_BADGES), None)
        for kind, _, body in MARKER.findall(figure.group(0)):
            path = body.split(":", 1)[0]
            if kind == "DIFF" and action not in (None, "수정"):
                report.error(f"{label}배지 : {path} diff 조각의 배지가 `{action}`")
            if kind not in ("FILE", "SYMBOL") or action not in ("신규", "수정"):
                continue
            old = git_lines(base, renames.get(path, path))
            if kind == "FILE" and action == "신규" and old is not None:
                report.error(f"{label}배지 : {path} 파일 전체 `신규` 인데 앞 단계에 있던 파일")
            elif kind == "FILE" and action == "수정" and old is None:
                report.error(f"{label}배지 : {path} 파일 전체 `수정` 인데 앞 단계에 없던 파일")
            elif kind == "SYMBOL" and action == "신규" and old is not None:
                symbol = body.split(":", 1)[1]
                if any(symbol in line for line in old):
                    report.error(f"{label}배지 : {path} `{symbol}` : `신규` 인데 앞 단계에 있던 함수")


def check_range_start(steps, base, ref, label, report):
    """범위 diff 가 추가 줄로 시작하지 않는지. 파일 맨 위에 넣는 줄처럼 앞에 문맥 줄이 없으면 넘어간다."""
    renames = renamed_paths(base, ref)
    for kind, _, body in MARKER.findall(steps):
        if kind != "DIFF" or ":" not in body:
            continue
        path, rest = body.split(":", 1)
        begin, until = rest.split("=>", 1)
        lines = git_lines(ref, path) or []
        bounds = slice_bounds(lines, begin, until)
        if bounds is None:
            continue
        start = bounds[0]
        added, _ = changed_lines(base, ref, path, renames.get(path))
        if start in added and not all(i in added for i in range(start)):
            report.error(f"{label}범위 diff : {path} 조각이 추가 줄 `{lines[start].strip()[:50]}` 로 시작")


def check_predict(source, tag, report):
    """먼저 예측 1번이 앞 편의 "다음 편 미리 생각하기" 를 받는지 라벨로 본다. 질문 문장은 편에 맞게 고쳐 쓰므로 비교하지 않는다."""
    toc = json.loads((HERE / "toc.json").read_text(encoding="utf-8"))
    bases = {lesson["tag"]: lesson["base"] for stage in toc["stages"] for lesson in stage["lessons"]}
    base_tag = bases.get(tag_ref(tag).rsplit("/", 1)[-1])
    if not base_tag:
        return
    base_id = base_tag[3:]
    base_file = HERE / f"template-{base_id}.html"
    if not base_file.exists() or 'id="preview"' not in base_file.read_text(encoding="utf-8"):
        return
    name = "α" + base_id[5:] if base_id.startswith("alpha") else base_id
    expected = f"1 · {name}편 끝에서 낸 질문"
    predict = re.search(r'<section id="predict".*?</section>', source, re.S)
    first = re.search(r'<p class="box-label">(.*?)</p>', predict.group(0), re.S) if predict else None
    if first is None or strip_tags(first.group(1)) != expected:
        report.error(f"먼저 예측 1번 라벨이 `{expected}` 이 아님")


def check_walk(body, report):
    r"""줄 단위 풀이 항목의 첫 코드가 그 풀이 앞의 조각에 있는지. 앞 풀이 뒤로 나온 조각만 본다. `...` 로 줄인 곳은 나눠 찾고, 공백과 줄 잇기 `\` 는 무시한다."""
    steps = re.search(r'<section id="steps".*?</section>', body, re.S)
    if not steps:
        return
    pattern = r'<figure class="code">.*?</figure>|<details class="fold">\s*<summary>줄 단위 풀이</summary>.*?</details>'
    for number, block in enumerate(step_blocks(steps.group(0)), 1):
        code = []
        for item in re.finditer(pattern, block, re.S):
            text = item.group(0)
            if text.startswith("<figure"):
                for attrs, pre in re.findall(r"<pre\b([^>]*)>(.*?)</pre>", text, re.S):
                    lines = html.unescape(re.sub(r"<[^>]+>", "", pre)).split("\n")
                    # diff 조각은 줄머리 기호를 떼야 여러 줄에 걸친 호출이 이어진다.
                    if 'class="diff"' in attrs:
                        lines = [line[1:] for line in lines if not line.startswith("@@")]
                    code.extend(lines)
                continue
            flat = re.sub(r"[\s\\]+", "", "\n".join(code))
            for entry in re.findall(r'<li class="entry">\s*<code>(.*?)</code>', text, re.S):
                snippet = html.unescape(entry)
                parts = [re.sub(r"[\s\\]+", "", part) for part in re.split(r"\.\.\.|…", snippet)]
                if not all(part in flat for part in parts if part):
                    report.warn(f"{number}단계 풀이 : 바로 위 조각에 없는 코드 `{snippet[:50]}`")
            code = []


def check_structure(body, report):
    for tag in PAIRED_TAGS:
        opened = len(re.findall(rf"<{tag}[\s>]", body))
        closed = body.count(f"</{tag}>")
        if opened != closed:
            report.error(f"태그 짝이 안 맞음 : <{tag}> 열림 {opened}, 닫힘 {closed}")

    ids = re.findall(r'\bid="([^"]+)"', body)
    duplicated = sorted({i for i in ids if ids.count(i) > 1})
    if duplicated:
        report.error("중복 id : " + ", ".join(duplicated))
    dangling = sorted(set(re.findall(r'href="#([^"]+)"', body)) - set(ids))
    if dangling:
        report.error("가리키는 곳이 없는 앵커 : " + ", ".join("#" + d for d in dangling))


def check_prose(body, report):
    # 자동 블록(바뀐 파일)과 푸터는 산문 점검에서 뺀다. 푸터의 라이선스 문구는 합쇼체가 맞다.
    prose = re.sub(r"<!-- code:begin -->.*?<!-- code:end -->", "", body, flags=re.S)
    prose = re.sub(r"<footer\b.*?</footer>", "", prose, flags=re.S)
    prose = re.sub(r"<pre\b.*?</pre>", "", prose, flags=re.S)

    whole = strip_tags(prose)
    if "—" in whole or "–" in whole:
        report.error(f"줄표 {whole.count('—') + whole.count('–')}개. 콜론이나 마침표로")

    for phrase, limit in PHRASE_LIMITS.items():
        count = whole.count(phrase)
        if count > limit:
            report.warn(f"`{phrase}` {count}회 (기준 {limit}회)")
    for name, pattern in PHRASE_PATTERNS:
        found = re.findall(pattern, whole)
        if found:
            hits = [m.group(0) for m in re.finditer(pattern, whole)]
            report.warn(f"{name} {len(found)}곳 : " + ", ".join(sorted(set(hits))))

    paragraphs = [strip_tags(p) for p in re.findall(r"<p(?:\s[^>]*)?>(.*?)</p>", prose, flags=re.S)]
    paragraphs = [p for p in paragraphs if re.search(r"[.?!]$", p)]  # 레이블로 쓰인 <p> 는 뺀다
    sentences = [s for p in paragraphs for s in split_sentences(p)]
    if not sentences:
        report.warn("산문 문단을 찾지 못함")
        return

    formal = [s for s in sentences if re.search(r"(습니다|입니다|합니다|됩니다|십시오)[.?!]$", s)]
    for sentence in formal:
        report.error(f"본문에 합쇼체 : {sentence[:60]}")

    lengths = [len(s) for s in sentences]
    words = [len(s.split()) for s in sentences]
    long_count = sum(1 for n in lengths if n >= 100)
    with_comma = sum(1 for s in sentences if "," in s)
    after_ending = [s for s in sentences if re.search(r"(고|며|지만|는데|은데|인데|서|니|면),", s)]
    report.note(
        f"산문 : 문단 {len(paragraphs)}, 문장 {len(sentences)}, 평균 {sum(words) / len(words):.1f}어절 / {sum(lengths) / len(lengths):.0f}자"
    )
    report.note(
        f"100자 이상 문장 {long_count}개 (1000문장당 {1000 * long_count / len(sentences):.0f}. 참고 : 사람 91, AI 8)"
    )
    report.note(
        f"쉼표가 든 문장 {100 * with_comma / len(sentences):.0f}% (참고 : 사람 26%, AI 61%), 연결어미 뒤 쉼표 {len(after_ending)}곳"
    )
    if with_comma / len(sentences) > 0.35:
        report.warn("쉼표가 든 문장이 35% 를 넘음. 연결어미 뒤 쉼표부터 뺀다")

    # 문단을 여닫는 버릇. 하나씩은 멀쩡하고 여러 번 나오면 틀로 보인다.
    count_openers = []
    forward_tails = []
    short_tails = []
    for paragraph in paragraphs:
        parts = split_sentences(paragraph)
        if re.search(r"[은는] (둘|셋|넷|다섯|여섯|일곱|\d+개|[두세네] (가지|개|곳))(이에요|예요)\.$", parts[0]):
            count_openers.append(parts[0])
        if len(parts) > 1:
            if re.search(r"(단계|편)에서 (다시 )?(봐요|다뤄요|써요|나와요|따라가요)\.$", parts[-1]):
                forward_tails.append(parts[-1])
            if len(parts[-1].split()) <= 5 and not parts[-1].endswith("?"):
                short_tails.append(parts[-1])
    if count_openers:
        report.warn(f"개수 선언으로 여는 문단 {len(count_openers)}곳 : " + " / ".join(count_openers))
    if len(forward_tails) > 2:
        report.warn(f"`N단계에서 봐요` 로 닫는 문단 {len(forward_tails)}곳 : " + " / ".join(forward_tails))
    if short_tails:
        report.note("문단 끝의 짧은 문장 (요약 꼬리인지 사람이 확인) :")
        for sentence in short_tails:
            report.note("    " + sentence)

    # 제목과 목록의 어미
    for title in steps_titles(prose):
        text = strip_tags(title)
        if not text.endswith("다"):
            report.warn(f"단계 제목이 `~한다` 동사구가 아님 : {text}")
    for item in re.findall(r'<li(?![^>]*class="(?:step|entry)[" ])(?:\s[^>]*)?>(.*?)</li>', prose, flags=re.S):
        text = strip_tags(re.sub(r"<(ul|ol)\b.*?</\1>", "", item, flags=re.S))
        if re.search(r"(요|니다)[.?!]?$", text) and not text.endswith("?"):
            report.warn(f"목록 항목이 명사 종결이 아님 : {text[-40:]}")


def steps_titles(prose):
    block = re.search(r'<ol class="steps">(.*?)</ol>\s*</section>', prose, flags=re.S)
    return re.findall(r"<h3>(.*?)</h3>", block.group(1), flags=re.S) if block else []


def main():
    parser = argparse.ArgumentParser(description="교재 원고의 기계 점검")
    parser.add_argument("file")
    parser.add_argument("--tag", help="마커의 경로·심볼을 확인할 git 태그 (예 : tut02)")
    parser.add_argument("--built", action="store_true", help="생성 결과를 점검. 마커가 남아 있으면 오류")
    args = parser.parse_args()

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    source = Path(args.file).read_text(encoding="utf-8")
    start = source.find("<body")
    end = source.rfind("<script>")
    body = source[start if start >= 0 else 0 : end if end > start else len(source)]

    report = Report()
    check_markers(source, args.tag, args.built, report)
    if args.tag and not args.built:
        check_follow_along(source, args.tag, report)
        check_predict(source, args.tag, report)
    if args.built:
        check_walk(body, report)
    check_structure(body, report)
    check_prose(body, report)

    print(f"점검 : {args.file}")
    for line in report.info:
        print("  " + line)
    for line in report.warnings:
        print("  [경고] " + line)
    for line in report.errors:
        print("  [오류] " + line)
    print(f"오류 {len(report.errors)}, 경고 {len(report.warnings)}")
    return 1 if report.errors else 0


if __name__ == "__main__":
    sys.exit(main())
