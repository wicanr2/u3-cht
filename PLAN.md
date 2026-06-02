# Ultima III: Exodus 中文化 — 執行計畫 (PLAN)

> 本檔由 `CLAUDE.md` 潤飾展開為可執行工程計畫。
> 立案日:2026-06-02 ・ 維護:L.CY (anr2) + Claude
> 上層依據:`ultima3-中文化評估.md`(可行性與格式分析,結論已定稿)。

---

## 0. TL;DR

把官方授權的 **`beastie/ultima3`(LairWare Ultima III,MIT C 源碼)** 作基礎,
剝離 Mac (Carbon/Cocoa/QuickDraw) 平台層、**改用 SDL2 重建繪圖/事件/音效**,
並把文字管線換成 **UTF-8 + SDL_ttf(優質中文 TTF 重建字庫,非 cubic / 非點陣)**,
翻譯 14 個 `.plist` 字串表,產出可在 Linux 執行的中文化 Ultima III。

* 風險最高三點(繪圖換得掉 / plist 讀得到 / 中文畫得出)用 **垂直切片 PoC** 一次打掉。
* 全程 **Docker build**;**逐畫面截圖**當決定性 pass/fail loop;**game tester 背景驗證**。
* **每完成一段落 commit + push** 到 `github.com/wicanr2/u3-cht`。

---

## 1. 目標與範圍

### 1.1 目標 (Goal)
在 Linux 上可遊玩、文字全中文化的 Ultima III: Exodus,畫面與行為對齊原版語意。

### 1.2 成功標準 (可驗證)
1. Docker 內 `make` 產出 Linux native ELF,啟動進入主畫面不崩潰。
2. 世界地圖 / 城鎮 / 戰鬥 / 地牢四類畫面可正常繪製與切換。
3. 所有對話、選單、訊息、角色/法術/物品名稱以**中文**顯示,換行/對齊正確(全形 2 倍寬)。
4. 可建立角色 → 走出 LCB → 進城對話 → 進入戰鬥 → 存讀檔 的完整最小遊玩迴圈通過。
5. game tester 在背景跑完上述腳本無 regression。

### 1.3 範圍外 (Out of scope, 初期)
* macOS / Windows 原生包(可後續用 SDL2 跨平台補,先聚焦 Linux)。
* 音樂 `.mov` → ogg 轉檔可延後(先靜音或只接音效 WAV)。
* 美術重繪 / 高解析 tileset(沿用既有 PNG tileset)。
* 散布授權處理(自用優先;引擎與資料分離以利日後)。

---

## 2. 專案結構與 repo 佈局決策

```
/home/anr2/u3-cht/                 ← 工作區 (非 git,放參考素材,不入庫)
├── CLAUDE.md                      ← 專案指示 (來源)
├── PLAN.md                        ← 本檔
├── ultima3-中文化評估.md           ← 可行性分析 (oracle)
├── ultima3/                       ← beastie/ultima3 上游源碼 (參考/取資產,唯讀)
├── u6-cht/                        ← 前例:U6 中文化 (字型 pipeline / 流程參考)
├── extracted/                     ← mcmagi 反組譯文件 (行為 ground-truth oracle)
└── u3-cht/                        ← ★ 專案 git repo (remote: wicanr2/u3-cht) ★
    ├── PLAN.md  CLAUDE.md         ← 計畫隨碼入庫 (複製一份)
    ├── src/                       ← 可攜 C 核心 + SDL2 後端
    │   ├── core/                  ← 從 ultima3/Sources 剝離的純遊戲邏輯
    │   ├── platform_sdl/          ← SDL2 繪圖/事件/檔案/音效 (取代 Mac 層)
    │   └── text/                  ← UTF-8 字串管線 + SDL_ttf CJK 渲染
    ├── assets/
    │   ├── graphics/              ← 由 ultima3/Resources/Graphics 篩選的 tileset PNG
    │   ├── fonts/                 ← 重建之中文 TTF/字庫
    │   └── strings/zh-Hant/       ← 翻譯後的 14 個字串表 (改 UTF-8 JSON/plist)
    ├── tools/                     ← 字庫建置、plist→JSON、截圖比對腳本
    ├── docker/                    ← Dockerfile (SDL2 + SDL_image + SDL_ttf + SDL_mixer)
    ├── tests/                     ← game tester 腳本 + 畫面快照基準
    └── docs/                      ← ADR、移植筆記、CONTEXT.md
```

**決策**:成品開發於 `u3-cht/`(git repo);上游 `ultima3/` 唯讀,只複製需要的源碼與資產進 `u3-cht/src` `assets`。理由:repo 乾淨、引擎與素材分離、符合「每段落 push」。

---

## 3. 技術堆疊

| 層 | 取代前 (Mac) | 取代後 |
|---|---|---|
| 繪圖 | QuickDraw `CGrafPtr` | SDL2 `SDL_Renderer` + `SDL_Texture` framebuffer |
| Tile | `DrawTiles`/`RectCopy`/`GetTileRectForIndex` | SDL2 texture region blit (保留抽象) |
| 圖片載入 | Mac resource / PNG | `SDL_image` |
| 事件/輸入 | Carbon event | `SDL_Event` |
| 音效 | Cocoa `LWSoundPlayer` | `SDL_mixer` (WAV;`.mov` 音樂延後) |
| 文字 | `UDrawThemePascalString` (系統字型) | `SDL_ttf` + **Noto Sans CJK TC**(預設,可換) |
| 字串表 | `GetPascalStringFromArrayByIndex` (Cocoa plist) | 重寫為無 Cocoa 的 UTF-8 JSON/plist 讀取 |
| 編碼 | Pascal string / MacRoman | UTF-8 (CJK-aware 寬度/換行) |
| Build | Xcode | **Docker** + Make/CMake → Linux ELF |

字型:CLAUDE.md 指定「優質系統中文 ttf 重建字庫,不要 cubic」。預設 **Noto Sans CJK TC**(已在系統 `/usr/share/fonts/opentype/noto/`),16px / 24px 兩級。整數倍 nearest-neighbor 放大邏輯畫布;文字層獨立以 TTF 清晰渲染。

---

## 4. 模組拆解 (deep modules / vertical slices)

對外介面收斂,內部隱藏複雜度:

1. **`platform_sdl` — 平台後端 (移植主戰場)**
   * 介面:`platform_init/shutdown`、`platform_present`、`platform_poll_event`、`platform_load_texture`、檔案讀寫。
   * 取代:`UltimaMacIF.c`(93 處 Mac 呼叫)、全部 `.m`、`CarbonShunts.c`(丟棄)。
2. **`gfx` — 繪圖引擎**
   * 介面:沿用 `DrawTiles`/`RectCopy`/`GetTileRectForIndex` 既有抽象,內部換 SDL_Renderer。
   * 取代:`UltimaGraphics.c`(69 處)。
3. **`text` — 文字渲染 + 字串管線 (中文化核心)**
   * 介面:`u_draw_string(utf8, x, y, font)`、`u_string_width(utf8)`、`u_rewrap(utf8, cols)`、`u_get_string(table, index)`。
   * hook:替換 `UDrawThemePascalString`/`UThemePascalStringWidth`/`UPrintChar`;重寫 `GetPascalStringFromArrayByIndex`;`RewrapString`/`PixelsWideString` 改 CJK-aware。
4. **純遊戲邏輯 (保留,最小改動)**
   * `UltimaMain/Misc/New/SpellCombat/Dngn/Autocombat/NewMap/Text` 的邏輯;`Autocombat`(0 處)、`SpellCombat`(1 處)幾乎可直接用。

---

## 5. 中文化資產管線

### 5.1 字串翻譯
* 來源:`ultima3/Resources/English.lproj/Strings/` 共 14 個 plist(`Messages` 為主對話 10KB,另 `MoreMessages`/`Classes`/`Male`/`Female`/`Intersex`/`Races`/`Spells`/`Tiles`/`TilesPlural`/`TilesVoices`/`WeaponsArmour`/`Pub`/`Radrion`)。
* 流程:plist → 抽成雙語對照表(en/zh)→ 翻譯 → 產出 `assets/strings/zh-Hant/*.json`(或 UTF-8 plist)。
* 術語一致:建立 `docs/CONTEXT.md` domain glossary(地名/角色/法術/物品/指令),翻譯與程式命名共用。
* gotcha:Pascal string 255 byte 上限 → 改 UTF-8 後解除長度假設;下游寬度/換行需 CJK-aware。

### 5.2 字庫重建 (TTF)
* 用 Noto Sans CJK TC 經 SDL_ttf 即時渲染(免預烘 atlas);或必要時預烘 16/24px glyph atlas 加速。
* 參考 `u6-cht` 的字型 pipeline,但**升級為 TTF**(不沿用 BDF 點陣/cubic)。

### 5.3 `MainResources.rsrc` 殘留抽取
* `CONS`/`SGNT`/`MISC`/`snd ` 仍走 `GetResource` → 從 Mac resource fork 抽出轉一般檔(星象/簽名圖/雜項)。延後到需要該畫面時處理。

---

## 6. 分階段交付 (每階段一個 pass/fail loop + commit/push)

| Phase | 內容 | 可驗證產出 (pass/fail) |
|---|---|---|
| **P0 基礎建設** | repo 骨架、Docker (SDL2 全套)、CMake/Make、CI smoke build | `docker build` 成功 + hello-SDL 開窗 |
| **P1 垂直切片 PoC** ★最高優先 | 載入 `Standard-Tiles.png` 用 SDL2 畫 64×64 地圖 + 讀 `Messages.plist` 一句翻中文用 SDL_ttf 畫出 | 截圖:地圖 + 一句中文同畫面 |
| **P2 平台層移植** | `UltimaMacIF.c` 逐段分類(視窗/事件/檔案/邏輯)→ SDL2 backend backlog,依賴序移植 | 主流程可初始化、收鍵盤事件 |
| **P3 繪圖引擎** | `gfx` 全面接 SDL_Renderer;四類畫面(世界/城鎮/戰鬥/地牢)繪製 | 四畫面截圖正確 |
| **P4 文字全中文化** | text 模組 hook 完成;14 plist 翻譯接上;CJK 換行/對齊 | 全畫面中文、無爆框/亂碼 |
| **P5 完整遊玩迴圈** | 存讀檔、音效、建角色→對話→戰鬥串接;game tester 腳本 | tester 跑完最小迴圈無 regression |
| **P6 收尾** | 音樂轉檔(選)、rsrc 殘留、打包、README/CREDITS、授權聲明 | 可散布(自用)版本 |

P2/P3 並行性高,移植時優先打通 P1→P2 主幹。

---

## 7. 測試與驗證策略

* **Docker first**:所有 build/test 在容器內,不污染系統(符合 [HARD] docker 規則)。
* **決定性 loop**(feedback-loop-priority):
  1. 編譯通過(最快訊號)。
  2. headless 啟動 → 指定 seed/腳本 → 自動截圖 → 與基準快照 diff。
  3. game tester 背景跑遊玩腳本,assert 關鍵畫面/狀態。
* **行為對照**:語意對不上時查 `extracted/`(mcmagi 反組譯)當 oracle。
* **背景測試**:遊戲測試一律背景執行(符合 CLAUDE.md)。

---

## 8. 風險與待決 (RAID)

| # | 風險/待決 | 等級 | 處置 |
|---|---|---|---|
| R1 | `UltimaMacIF.c` 2,765 行移植稅 | 🟠 | P2 先逐段盤點分類再動手 |
| R2 | Pascal string → UTF-8(255 上限、寬度假設) | 🟡 | text 模組集中處理,加 CJK 寬度表 |
| R3 | `MainResources.rsrc` 殘留資料抽取 | 🟡 | 延後;需要該畫面時抽 |
| R4 | 文字繪製單一 hook point 確認 | 🟡 | 已定位 `UDrawThemePascalString`,P1 驗證 |
| R5 | 32-bit / Carbon int 寬度慣用法 | 🟡 | 移植時逐一收斂型別 |
| R6 | 非程式資產授權(美術/音樂非 MIT) | 🟡 | 引擎/資料分離;自用優先 |
| D1 | 字型最終選用(預設 Noto Sans CJK TC) | 待確認 | 預設先行,可換 |
| D2 | 字串表落地格式(JSON vs UTF-8 plist) | 待定 | P1 試 JSON,簡單優先 |

---

## 9. Git / 交付節奏

* repo:`github.com/wicanr2/u3-cht`(已 authed)。
* 每完成一個 Phase / 子段落 → commit(繁中訊息)+ push。
* 重大設計決策寫 `docs/adr/NNNN-*.md`;術語進 `docs/CONTEXT.md`。
* commit 結尾附 Co-Authored-By(依環境規範)。

---

## 10. 立即下一步

1. 初始化 `u3-cht/` repo 骨架(目錄 + Docker + Make + 複製 PLAN/CLAUDE),首次 commit/push。**[P0]**
2. 寫垂直切片 PoC:SDL2 開窗 → 畫一張 tileset 地圖 → SDL_ttf 畫一句中文。**[P1]**
3. `UltimaMacIF.c` 移植稅盤點報告。**[P2 起手]**
