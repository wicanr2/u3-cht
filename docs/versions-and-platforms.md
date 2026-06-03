# 版本與平台差異(含 LairWare Mac 版與本專案)

> 原創中文整理,事實依據見文末。平台清單以可查證來源為準;不確定處標「待考」。

## 一、原版與各平台移植

《Ultima III: Exodus》以 **Apple II** 開發、1983 年發行,之後被移植到**十多個平台**
(英文 Wikipedia〈Ultima III: Exodus〉與 MobyGames 列出 13 個其他平台)。常見版本可概分為:

| 類別 | 平台 | 備註 |
|---|---|---|
| 原始 / 同期 | Apple II、Commodore 64、Atari 8-bit、IBM PC | 1983 前後 |
| 強化畫面移植 | **Macintosh**、Atari ST、Amiga | 畫面/介面較精緻 |
| 日本平台 | FM-7、MSX、PC-88、PC-98、Sharp X1、FM Towns | 多有日文化 |
| 家用主機 | **NES / Famicom**(美版名 *Ultima: Exodus*) | 重製、改動較大 |
| 現代 | Windows 等 | 後續再發行 |

> 各版在畫面、音樂、介面操作、甚至部分內容上互有差異(例如 NES 版為重製、配樂與圖像
> 大幅重做)。逐版差異的細節超出本專案範圍,屬「待考/延伸」項目。

## 二、本專案的基礎:LairWare 的 Macintosh 版

本繁中化專案**不是**直接改原始 Apple II/DOS 版,而是以 **LairWare 的 Ultima III
(Macintosh 版)** 的開源源碼為基礎:

- 該源碼由 **Leon McNeill** 以 **MIT 授權**釋出(GitHub:
  [`beastie/ultima3`](https://github.com/beastie/ultima3))。
- 程式語言主體為 **C**(遊戲邏輯),平台殼為 Classic Mac 的 **Carbon + Cocoa + QuickDraw**。
- 屬「強化畫面」一脈的 Mac 版:有人像、生命/法力條等較現代的介面元素
  (對應遊戲內 *classic appearance* 開關)。

> 「授權」釐清:此處的 **MIT** 指**程式碼**的開源授權(© Leon McNeill);Ultima 系列的
> **IP / 美術 / 音樂等非程式資產**著作權屬原權利人(現屬 EA)。本專案採「引擎與資料分離」。

## 三、本專案 vs. 上游 LairWare Mac 版

本專案在上游 Mac 版之上做了兩類改造:**平台移植(Mac→SDL2/Linux)** 與 **繁體中文化**。

| 面向 | 上游 LairWare Mac 版 | 本專案(u3-cht) |
|---|---|---|
| 繪圖 / 平台層 | Carbon / Cocoa / QuickDraw(Mac 專屬,32-bit) | **SDL2 重建**(`src/compat/` shim),可在 Linux 編譯執行 |
| 文字渲染 | Mac Theme Font(系統字型) | **SDL_ttf + CJK TTF**(Noto Sans CJK),單一出口替換 |
| 字串資料 | Cocoa plist / Mac 資源 | **`.u3s`**(UTF-8)字串表,外部化、可回退英文 |
| 語言 | 英文 | **繁體中文**(選單/對話/狀態/種族職業/說明圖) |
| 種族職業說明圖 | 英文 `RaceClassInfo.gif` | **重繪繁中**(`tools/make_raceclass_gif.py`) |
| 發行形式 | Mac 應用程式(現代系統難以執行) | **Linux AppImage + Windows zip** |
| 音效 / 音樂 | Mac 音效 / QuickTime Music | 音效走 **SDL_mixer**;音樂(`.mov` 類 MIDI)目前 stub |

### 與「原始 Apple II 版」的差異(摘要)
因為基礎是 **Mac 強化版**,本專案的觀感更接近 Mac 版而非 1983 Apple II 原貌:
有人像與長條狀態列、較大的視窗解析度(640×480 邏輯畫布)。若想體驗最接近原始的
8-bit 觀感,應另尋 Apple II/C64 原版模擬;本專案的目標是**讓 Mac 版的內容以繁體中文、
在現代 Linux/Windows 上可玩**。

## 延伸閱讀

- [發行與移植史](release-history.md) — 開發者、原版年份、平台清單、官方/第三方在地化。
- [在地化筆記](localization-notes.md) — SDL2 移植、字串管線、字型與測試細節。
- [世界觀](ultima-iii-overview.md)、[人物與勢力](characters-and-factions.md)。

## 參考來源

- Wikipedia, *Ultima III: Exodus* — https://en.wikipedia.org/wiki/Ultima_III:_Exodus
- MobyGames, *Exodus: Ultima III (1983)*(版本/平台)— https://www.mobygames.com/game/878/exodus-ultima-iii/
- The Codex of Ultima Wisdom, *Ultima III: Exodus* — https://wiki.ultimacodex.com/wiki/Ultima_III:_Exodus
- 上游源碼:`beastie/ultima3`(LairWare Ultima III,MIT)— https://github.com/beastie/ultima3
- 本專案技術細節:[localization-notes.md](localization-notes.md)、`docs/adr/`
