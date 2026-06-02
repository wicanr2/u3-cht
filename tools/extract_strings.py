#!/usr/bin/env python3
"""抽出 Ultima III 英文 plist 字串表,產出:
  1. 雙語 JSON 骨架 (translations/<Table>.json) — 供翻譯填 zh 欄
  2. 執行期字串表 .u3s (assets/strings/<lang>/<Table>.u3s)

.u3s 格式 (little-endian):
  magic "U3S1" | uint32 count | count × uint32 offset | UTF-8 blob (null 結尾)
  offset 相對 blob 起點。支援 >255 byte (中文化必要)。

用法 (容器內):
  python3 tools/extract_strings.py extract   # plist → translations/*.json 骨架
  python3 tools/extract_strings.py build en   # 英文 → assets/strings/en/*.u3s
  python3 tools/extract_strings.py build zh-Hant  # 中文 → assets/strings/zh-Hant/*.u3s
"""
import plistlib, glob, os, json, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLIST_DIR = os.path.join(ROOT, "..", "ultima3", "Resources", "English.lproj", "Strings")
TRANS_DIR = os.path.join(ROOT, "translations")


def load_plist_list(path):
    with open(path, "rb") as fp:
        data = plistlib.load(fp)
    return data if isinstance(data, list) else None


def table_names():
    out = []
    for f in sorted(glob.glob(os.path.join(PLIST_DIR, "*.plist"))):
        if load_plist_list(f) is not None:
            out.append(os.path.splitext(os.path.basename(f))[0])
    return out


def cmd_extract():
    os.makedirs(TRANS_DIR, exist_ok=True)
    total = 0
    for f in sorted(glob.glob(os.path.join(PLIST_DIR, "*.plist"))):
        lst = load_plist_list(f)
        if lst is None:
            continue
        name = os.path.splitext(os.path.basename(f))[0]
        out_path = os.path.join(TRANS_DIR, name + ".json")
        # 保留既有翻譯
        existing = {}
        if os.path.exists(out_path):
            with open(out_path, encoding="utf-8") as fp:
                for e in json.load(fp):
                    existing[e["i"]] = e.get("zh", "")
        entries = [{"i": i, "en": s, "zh": existing.get(i, "")} for i, s in enumerate(lst)]
        with open(out_path, "w", encoding="utf-8") as fp:
            json.dump(entries, fp, ensure_ascii=False, indent=1)
        total += len(entries)
        print(f"  {name:22s} {len(entries):4d} 句 → translations/{name}.json")
    print(f"共 {total} 句")


def write_u3s(path, strings):
    blob = bytearray()
    offsets = []
    for s in strings:
        offsets.append(len(blob))
        blob += s.encode("utf-8") + b"\x00"
    with open(path, "wb") as fp:
        fp.write(b"U3S1")
        fp.write(struct.pack("<I", len(strings)))
        for off in offsets:
            fp.write(struct.pack("<I", off))
        fp.write(blob)


def cmd_build(lang):
    out_dir = os.path.join(ROOT, "assets", "strings", lang)
    os.makedirs(out_dir, exist_ok=True)
    for name in table_names():
        if lang == "en":
            strings = load_plist_list(os.path.join(PLIST_DIR, name + ".plist"))
        else:
            jpath = os.path.join(TRANS_DIR, name + ".json")
            if not os.path.exists(jpath):
                print(f"  跳過 {name} (無翻譯)"); continue
            with open(jpath, encoding="utf-8") as fp:
                data = json.load(fp)
            # 未翻譯 (zh 空) 回退英文,確保不缺字
            strings = [(e["zh"] if e.get("zh") else e["en"]) for e in data]
        write_u3s(os.path.join(out_dir, name + ".u3s"), strings)
        print(f"  {name:22s} {len(strings):4d} 句 → assets/strings/{lang}/{name}.u3s")


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "extract"
    if cmd == "extract":
        cmd_extract()
    elif cmd == "build":
        cmd_build(sys.argv[2] if len(sys.argv) > 2 else "en")
    else:
        print(__doc__)
