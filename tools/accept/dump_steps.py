import re
import sys
from html.parser import HTMLParser

src = open(sys.argv[1], encoding="utf-8").read()
start = src.index("<h2>따라 하기</h2>")
end = src.index("<h2>", start + 10)
section = src[start:end]


class Dumper(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.out = []
        self.buf = []
        self.in_pre = False
        self.pre_kind = ""
        self.in_caption = False
        self.caption = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag == "pre":
            self.in_pre = True
            self.pre_kind = a.get("class", "")
            self.buf = []
        elif tag == "figcaption":
            self.in_caption = True
            self.caption = []
        elif tag == "span" and self.in_caption and "badge" in (a.get("class") or ""):
            self.caption.append(" [")
        elif tag in ("h3", "summary"):
            self.buf = []
        if tag == "br" and self.in_pre:
            self.buf.append("\n")

    def handle_endtag(self, tag):
        if tag == "pre":
            self.out.append(f"```{self.pre_kind}\n{''.join(self.buf)}\n```")
            self.in_pre = False
            self.buf = []
        elif tag == "figcaption":
            self.in_caption = False
            self.out.append("CAPTION: " + "".join(self.caption).strip())
        elif tag == "h3":
            self.out.append("\n## STEP: " + "".join(self.buf).strip())
            self.buf = []
        elif tag == "summary":
            self.out.append("FOLD: " + "".join(self.buf).strip())
            self.buf = []
        elif tag in ("p", "li", "dd", "dt") and not self.in_pre:
            text = re.sub(r"\s+", " ", "".join(self.buf)).strip()
            if text:
                self.out.append(text)
            self.buf = []

    def handle_data(self, data):
        if self.in_caption:
            self.caption.append(data)
            if len(self.caption) >= 2 and self.caption[-2] == " [":
                self.caption.append("]")
            return
        self.buf.append(data)


dumper = Dumper()
dumper.feed(section)
print("\n".join(dumper.out))
