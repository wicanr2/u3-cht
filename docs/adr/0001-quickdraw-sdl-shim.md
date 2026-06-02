# ADR 0001 — 以 QuickDraw→SDL2 相容 shim 移植,而非改寫呼叫端

- 狀態:已採用
- 日期:2026-06-02

## 脈絡

beastie/ultima3 約 18,600 行 C 碼,深度依賴 Classic Mac QuickDraw(`CGrafPtr`/`GWorld`/`CopyBits`/`RGBForeColor`/`UDrawThemePascalString`…)。
全 `.c` 檔的 QuickDraw 呼叫統計:`CopyBits` 59、`SetGWorld` 45、`NewGWorld` 41、`UDrawThemePascalString` 49、`PaintRect` 44、`MoveTo` 72、`SetRect` 134… 表面雖大但函式種類有限(約 40 個)。

兩條路:
1. **改寫每個呼叫端** 成 SDL2 — 動到上千處,易出錯、難回歸對照。
2. **建一層 shim**:用 SDL2 重新實作這 ~40 個 QuickDraw 函式,呼叫端幾乎不動。

## 決策

採方案 2 —— `src/compat/` 提供 QuickDraw 相容層:

- `GrafPort` 包一塊 `SDL_Surface`(ARGB8888);`CGrafPtr`/`GWorldPtr`/`GrafPtr` 皆 `GrafPort*`。
- `PixMapHandle = PixMap**`,保留原碼 `(**pm).rowBytes`、`GetPixBaseAddr` 存取慣用法。
- `CopyBits` → `SDL_BlitSurface`/`SDL_BlitScaled`;`CopyMask` → per-pixel 亮度遮罩。
- 文字出口 `UDrawThemePascalString`/`UThemePascalStringWidth`/`DrawString` → SDL_ttf,**UTF-8 處理**(中文化關鍵 hook),baseline 對齊。

## 理由

- 呼叫端零改動 → 可逐檔編譯、與原版行為逐畫面對照(§6 截圖 loop)。
- 介面收斂(deep module):複雜度藏在 ~6 個 compat 檔內,對外是既有 QuickDraw 介面。
- 中文化只需攔截單一文字出口 + 字串表來源,全遊戲文字一次到位。

## 後果

- 需補 CoreFoundation(`CFStringRef`/`CFURLRef`/bundle)shim 才能編譯資源載入碼(後續 P3d)。
- `blend`/`transparent` 模式、8-bit 轉場效果(intro)為近似,須日後對照修正。
- Pascal `Str255` 255-byte 上限:中文 UTF-8 約 85 字/行,靠上層 `RewrapString` 切行規避(風險 R2)。

## 驗證

- `test_geometry`(13 項)、`test_qd_blit`(CopyBits 1:1+縮放)、`test_qd_text`(CJK baseline + 畫筆推進 + 寬度)全綠。
