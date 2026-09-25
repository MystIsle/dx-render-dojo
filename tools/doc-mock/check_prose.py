#!/usr/bin/env python3
"""교재 원고(template-NN.html)의 기계 점검.

사람이 읽어야 아는 것은 다루지 않는다. 셀 수 있는 것만 센다 : 마커, 앵커, 태그 짝, 어미, 금지 표현, 리듬 수치.
규칙의 출처는 docs/DOC_STYLE.md 다.

사용법 :
    python tools/doc-mock/check_prose.py template-02.html --tag tut02  # 마커가 가리키는 경로·심볼을 git 태그에서 확인
    python tools/doc-mock/check_prose.py tutorial-02.html --built  # 생성 결과에 남은 마커가 없는지

오류가 하나라도 있으면 종료 코드 1. 경고는 종료 코드에 영향을 주지 않는다.
"""

import argparse
import html
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

MARKER = re.compile(r"<!--(INCLUDE|FILE|SYMBOL|DIFF|SLICE):(.*?)-->", re.S)


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


def git_lines(tag, path):
    result = subprocess.run(
        ["git", "-C", str(REPO), "show", f"{tag}:{path}"], capture_output=True, text=True, encoding="utf-8"
    )
    if result.returncode != 0:
        return None
    return result.stdout.split("\n")


def check_markers(source, tag, built, report):
    markers = MARKER.findall(source)
    kinds = {}
    for kind, _ in markers:
        kinds[kind] = kinds.get(kind, 0) + 1
    report.note("마커 : " + (", ".join(f"{k} {v}" for k, v in sorted(kinds.items())) or "없음"))

    for kind, body in markers:
        if kind == "SYMBOL" and ":" not in body:
            report.error(f"SYMBOL 마커에 심볼이 없음 : {body}")
        if kind == "SLICE" and not re.fullmatch(r".+?:.+?=>.+", body, re.S):
            report.error(f"SLICE 마커 형식이 `경로:시작=>끝` 이 아님 : {body}")

    if built and markers:
        report.error(f"생성 결과에 채워지지 않은 마커 {len(markers)}개")
    if tag is None or built:
        return

    ref = tag if tag.startswith("refs/") else f"refs/tags/{tag}"
    cache = {}

    def lines_of(path):
        if path not in cache:
            cache[path] = git_lines(ref, path)
        return cache[path]

    for kind, body in markers:
        if kind == "INCLUDE":
            if not (HERE / body).exists():
                report.error(f"INCLUDE 대상이 없음 : {body}")
            continue
        path = body.split(":", 1)[0]
        lines = lines_of(path)
        if lines is None:
            report.error(f"{kind} : {ref} 에 없는 경로 : {path}")
            continue
        if kind == "SYMBOL":
            symbol = body.split(":", 1)[1]
            if not any(symbol in line for line in lines):
                report.error(f"SYMBOL : {path} 에 없는 문자열 : {symbol}")
        if kind == "SLICE":
            begin, until = body.split(":", 1)[1].split("=>", 1)
            start = next((i for i, line in enumerate(lines) if begin in line), -1)
            if start < 0:
                report.error(f"SLICE : {path} 에 없는 시작 문자열 : {begin}")
            elif not any(until in line for line in lines[start + 1 :]):
                report.error(f"SLICE : {path} 에서 시작 줄 뒤에 끝 문자열이 없음 : {until}")


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
    for item in re.findall(r'<li(?![^>]*class="step")(?:\s[^>]*)?>(.*?)</li>', prose, flags=re.S):
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
