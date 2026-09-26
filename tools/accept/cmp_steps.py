import pathlib
import re
import sys


def parts(path):
    text = pathlib.Path(path).read_text(encoding="utf-8")
    code = re.findall(r"^```\S+\n(.*?)\n```$", text, re.S | re.M)
    captions = [l for l in text.split("\n") if l.startswith("CAPTION: ")]
    folds = [l for l in text.split("\n") if l.startswith("FOLD: ")]
    return code, captions, folds


old_code, old_captions, old_folds = parts(sys.argv[1])
new_code, new_captions, new_folds = parts(sys.argv[2])
print("code blocks:", len(old_code), len(new_code), "identical:", old_code == new_code)
for i, (a, b) in enumerate(zip(old_captions, new_captions)):
    if a != b:
        print("caption", i, "|", a, "=>", b)
for fold in sorted(set(old_folds) ^ set(new_folds)):
    print("fold diff:", fold)
