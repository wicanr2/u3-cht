#!/usr/bin/env python3
"""make_raceclass_gif.py — 重繪 RaceClassInfo.gif 為繁體中文版。

角色建立時 DrawRaceHelp / DrawClassHelp (UltimaNew.c) 會從這張圖切條帶 blit:
  - 種族:每條 400x34,y = (race-1)*34,共 5 條 (頂端 170px)
  - 職業:每條 400x53,y = 170 + (clss-1)*53,共 11 條 (其餘 583px)
總尺寸固定 400x753,務必與切片座標一致,否則對位錯誤。

數值取自原英文圖 (= 遊戲 Shrine / 角色建立橫幅的權威顯示)。
於 docker 容器內執行 (PIL + Noto CJK):
  python3 tools/make_raceclass_gif.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT  = os.path.join(ROOT, "assets", "RaceClassInfo.gif")
FONT = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"

W = 400
RACE_H, CLASS_H = 34, 53
N_RACE, N_CLASS = 5, 11
H = N_RACE * RACE_H + N_CLASS * CLASS_H   # 170 + 583 = 753

# 種族:名稱 + 屬性上限 (力量/敏捷/智力/智慧)
RACES = [
    ("人類", 75, 75, 75, 75),
    ("精靈", 75, 99, 75, 50),
    ("矮人", 99, 75, 50, 75),
    ("哈比族", 75, 50, 75, 99),
    ("毛絨族", 25, 99, 99, 75),
]

# 職業:名稱 + 說明 (依原圖語意改寫為精簡繁中)
CLASSES = [
    ("鬥士", "可使用任何武器與盔甲,但不能施展法術,也不擅長偷竊或解除陷阱。"),
    ("牧師", "依智慧施展牧師法術。僅能用匕首或釘錘,最多穿鎖甲;不擅偷竊與解陷阱。"),
    ("巫師", "依智力施展巫師法術。僅能用匕首、穿布衣,不擅偷竊與解除陷阱。"),
    ("竊賊", "可用單手劍以下武器,僅能穿皮甲,但非常擅長偷竊與解陷阱;完全不能施法。"),
    ("聖騎士", "半鬥士半牧師。可用任何武器,最多穿板甲;能以半數智慧施展牧師法術。"),
    ("野蠻人", "半鬥士半竊賊。可用任何武器,但僅能穿皮甲;具部分偷竊與解陷阱能力。"),
    ("雲雀", "半鬥士半巫師。可用任何武器,僅能穿布衣;能以半數智力施展巫師法術。"),
    ("幻術師", "半竊賊半牧師。宜用釘錘與皮甲,能以半數智慧施牧師法術,略通偷竊與解陷阱。"),
    ("德魯伊", "半牧師半巫師。最多用釘錘與布衣,牧師與巫師法術皆可施;法力恢復為他人兩倍。"),
    ("鍊金術士", "半竊賊半巫師。僅能用匕首與布衣,以半數智力施巫師法術;偷竊與解陷阱有限。"),
    ("遊俠", "全才。可用 +2 劍與 +2 板甲,牧師與巫師法術皆可施,亦略能偷竊與解除陷阱。"),
]

C_TITLE   = (255, 230, 90)    # 標題字 (亮黃)
C_TITLEBG = (40, 40, 150)     # 標題列底 (深藍)
C_STATBG  = (244, 244, 248)   # 屬性列底 (近白)
C_STAT    = (20, 20, 40)
C_CLASSBG1 = (248, 248, 242)
C_CLASSBG2 = (236, 238, 246)  # 交錯底色,易讀
C_CLASSNAME = (150, 30, 30)   # 職業名 (暗紅)
C_CLASSTXT  = (25, 25, 30)


def cjk_wrap(text, font, draw, max_w):
    """逐字元換行 (CJK 無空白可斷)。"""
    lines, cur = [], ""
    for ch in text:
        if draw.textlength(cur + ch, font=font) <= max_w:
            cur += ch
        else:
            lines.append(cur)
            cur = ch
    if cur:
        lines.append(cur)
    return lines


def main():
    img = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(img)
    f_title = ImageFont.truetype(FONT, 14)
    f_stat  = ImageFont.truetype(FONT, 13)
    f_cname = ImageFont.truetype(FONT, 14)
    f_ctext = ImageFont.truetype(FONT, 12)

    # ---- 種族條帶 ---- (標題/屬性各佔 17px,文字以 anchor="mm" 垂直置中避免貼頂裁切)
    TBAR = 17
    for i, (name, s, dx, intel, wis) in enumerate(RACES):
        y0 = i * RACE_H
        # 標題列 (上半,藍底黃字)
        d.rectangle([0, y0, W, y0 + TBAR], fill=C_TITLEBG)
        title = f"{name}屬性上限"
        d.text((W / 2, y0 + TBAR / 2), title, font=f_title, fill=C_TITLE, anchor="mm")
        # 屬性列 (下半,白底)
        d.rectangle([0, y0 + TBAR, W, y0 + RACE_H], fill=C_STATBG)
        stat = f"力量={s}   敏捷={dx}   智力={intel}   智慧={wis}"
        d.text((W / 2, y0 + TBAR + (RACE_H - TBAR) / 2), stat, font=f_stat, fill=C_STAT, anchor="mm")

    # ---- 職業條帶 ----
    base = N_RACE * RACE_H
    for i, (name, desc) in enumerate(CLASSES):
        y0 = base + i * CLASS_H
        bg = C_CLASSBG1 if i % 2 == 0 else C_CLASSBG2
        d.rectangle([0, y0, W, y0 + CLASS_H], fill=bg)
        # 職業名 (首行起頭)
        d.text((6, y0 + 3), name, font=f_cname, fill=C_CLASSNAME)
        name_w = d.textlength(name, font=f_cname) + 12
        # 說明文字 (首行自職業名右側起,後續行回到左邊)
        first_w = W - name_w - 6
        lines = cjk_wrap(desc, f_ctext, d, W - 12)
        # 重新依「首行較窄」排版
        lines = []
        cur = ""
        first = True
        for ch in desc:
            limit = first_w if first else (W - 12)
            if d.textlength(cur + ch, font=f_ctext) <= limit:
                cur += ch
            else:
                lines.append((cur, first))
                cur = ch
                first = False
        if cur:
            lines.append((cur, first))
        ly = y0 + 4
        for text, isfirst in lines[:3]:
            lx = name_w if isfirst else 8
            d.text((lx, ly), text, font=f_ctext, fill=C_CLASSTXT)
            ly += 16

    img.save(OUT, "GIF")
    print(f"已輸出 {OUT}  ({img.size[0]}x{img.size[1]})")


if __name__ == "__main__":
    main()
