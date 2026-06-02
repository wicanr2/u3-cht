# CONTEXT.md — Ultima III 中文化 Ubiquitous Language

專案術語表。命名變數、寫文件、翻譯時優先用此處詞彙。
格式:`Term — 定義. _Avoid_: 禁用同義詞`

## 架構

- **compat 層 (compat shim)** — `src/compat/`,以 SDL2 重建 QuickDraw/Mac API,讓上游 C 碼幾乎不改即可編譯。見 ADR 0001。
- **GrafPort** — 繪圖埠,內含一塊 SDL_Surface 像素緩衝。`CGrafPtr`/`GWorldPtr` 皆指向它。
- **GWorld** — 離屏繪圖緩衝 (offscreen)。U3 用約 11 個 (tilesPort/gamePort/mainPort…)。
- **tile** — 16×16 邏輯、實際 32×32px (blkSiz=16,×2) 的地圖/角色圖塊。
- **.u3s** — 執行期 UTF-8 字串表二進位格式 (magic `U3S1` + offset 表 + blob)。由 `tools/extract_strings.py` 產生。
- **字串表 (string table)** — 一組具名字串陣列 (Messages/Classes/Spells…),原為 Cocoa plist,現為 `.u3s`。_Avoid_: resource、STR#。
- **文字出口 (text hook)** — `UDrawThemePascalString`/`UThemePascalStringWidth`,全遊戲繪字的單一攔截點 → SDL_ttf CJK。

## 遊戲領域 (譯名定案)

- **職業 (Class)** — 鬥士 / 牧師 / 巫師 / 竊賊 / 聖騎士 / 野蠻人 / 雲雀 / 幻術師 / 德魯伊 / 鍊金術士 / 遊俠。
- **種族 (Race)** — 人類 / 精靈 / 矮人 / 哈比族 / 毛絨族 (Fuzzy)。
- **咒語 (Spell)** — 符文施法字 (REPOND/MITTAR…),**不翻譯**,保留原文 (玩家輸入的咒文)。
- **Sosaria** — 索薩利亞 (遊戲世界名)。
- **Exodus** — 末日 (副標題 / 最終敵人)。_Avoid_: 出埃及記。
- **LCB (Lord British's Castle)** — 不列顛王城堡。

## 翻譯原則

- 不改原始資料檔 offset:中文走外部 `.u3s`,以 `(table, index)` 為 key。
- 未翻譯項自動回退英文 (build 時 zh 空 → 用 en),確保不缺字。
- 行寬受 Str255 (255 byte) 限制者,靠 `RewrapString` 切行;新繪字路徑用 `U3_DrawUTF8` 無此限。

## Flagged ambiguities (待釐清)

- "Lark" 職業譯名暫定「雲雀」,待確認是否有更貼切的中文 RPG 慣用譯。
- "Fuzzy" / "Bobbit" 種族譯名 (毛絨族 / 哈比族) 暫定,待風格確認。
