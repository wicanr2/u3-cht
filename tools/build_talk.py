#!/usr/bin/env python3
"""建置 NPC 對話 (TLKS) 翻譯表 → assets/strings/<lang>/Talk.u3s

NPC 對話來源是各地圖的 'TLKS' 資源 (非 plist),載入到 Talk[256],由
Speak(perNum, shnum) 渲染。本表以複合索引串接 (地圖, NPC):

    index = (map_resid - 400) * 64 + npc

Speak 的中文化鉤子用 (gCurMapID, perNum) 算出同一索引查表;查到非空字串
即直接渲染中文,否則回退原始 Talk[] (英文)。

translations/Talk.json 格式:[{ "map": <resid>, "npc": <perNum>, "en": ..., "zh": ... }, ...]

用法 (容器內):
  python3 tools/build_talk.py            # 產 en + zh-Hant
"""
import os, json, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASERES = 400
STRIDE = 64  # 每張地圖最多 64 個 NPC talk 槽


def composite_index(rec):
    return (rec["map"] - BASERES) * STRIDE + rec["npc"]


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


def main():
    jpath = os.path.join(ROOT, "translations", "Talk.json")
    with open(jpath, encoding="utf-8") as fp:
        recs = json.load(fp)
    if not recs:
        print("Talk.json 無資料"); return
    count = max(composite_index(r) for r in recs) + 1
    en = [""] * count
    zh = [""] * count
    for r in recs:
        idx = composite_index(r)
        en[idx] = r.get("en", "")
        zh[idx] = r.get("zh", "") or r.get("en", "")
    for lang, arr in (("en", en), ("zh-Hant", zh)):
        out_dir = os.path.join(ROOT, "assets", "strings", lang)
        os.makedirs(out_dir, exist_ok=True)
        write_u3s(os.path.join(out_dir, "Talk.u3s"), arr)
        print(f"  Talk {count:4d} 槽 ({len(recs)} 句已譯) → assets/strings/{lang}/Talk.u3s")


if __name__ == "__main__":
    main()
