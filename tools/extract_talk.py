#!/usr/bin/env python3
"""抽取各地圖 TLKS 資源的 NPC 對話原文 → translations/Talk.json 骨架。

TLKS 編碼 (每張地圖一個資源,resid 400-418/421,256 byte):
  - 0x00 分隔不同 NPC 的對話字串 (字串索引 = Speak 的 perNum)。
  - 字串內:0xFF = 換行,其餘 byte 取 & 0x7F 還原 ASCII (原始為高位元設定)。

輸出 records:{ "map": resid, "npc": index, "en": 原文, "zh": 既有翻譯或空 }。
保留 Talk.json 既有 zh 翻譯 (以 (map,npc) 為 key 合併)。

用法 (容器內):
  python3 tools/extract_talk.py
"""
import os, json, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
from extract_rsrc import parse, RSRC


def decode_string(b):
    """TLKS 單一 NPC 字串 bytes → 還原文字 (0xFF→\\n, 其餘 &0x7F)。"""
    return "".join("\n" if x == 0xFF else chr(x & 0x7F) for x in b)


def clean(s):
    """正規化:換行轉空白、收斂多餘空白 (Speak 鉤子會 RewrapString 重排)。"""
    return " ".join(s.replace("\n", " ").split())


def is_dialogue(b):
    """判斷此 NUL 區段是否為真 NPC 對話 (純可印 ASCII):
    TLKS blob 在對話字串之後夾有商店/二進位資料,其 byte 含控制碼,須排除。"""
    for x in b:
        if x == 0xFF:           # 換行,允許
            continue
        c = x & 0x7F
        if c < 0x20 or c == 0x7F:  # 控制碼 → 非文字
            return False
    return True


def main():
    jpath = os.path.join(ROOT, "translations", "Talk.json")
    existing = {}
    if os.path.exists(jpath):
        with open(jpath, encoding="utf-8") as fp:
            for r in json.load(fp):
                existing[(r["map"], r["npc"])] = r.get("zh", "")

    recs = []
    for rtype, rid, data in parse(RSRC):
        if rtype != "TLKS":
            continue
        parts = data.split(b"\x00")
        for idx, p in enumerate(parts):
            if not p or not is_dialogue(p):
                continue
            en = clean(decode_string(p))
            # 真 NPC 對話為全大寫;含小寫或無大寫字母者為商店/二進位殘留,排除。
            if not en or any("a" <= ch <= "z" for ch in en) or not any("A" <= ch <= "Z" for ch in en):
                continue
            recs.append({
                "map": rid, "npc": idx,
                "en": en, "zh": existing.get((rid, idx), ""),
            })
    recs.sort(key=lambda r: (r["map"], r["npc"]))
    with open(jpath, "w", encoding="utf-8") as fp:
        json.dump(recs, fp, ensure_ascii=False, indent=1)
    done = sum(1 for r in recs if r["zh"])
    print(f"TLKS NPC 對話:共 {len(recs)} 句,已譯 {done}{jpath}")


if __name__ == "__main__":
    main()
