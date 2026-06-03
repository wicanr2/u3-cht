# 在地化筆記(本專案)

本文記錄 **u3-cht** 繁體中文化專案的翻譯原則、名詞決策,以及字型/截圖/SDL/AppImage 的
測試注意事項。術語以 [CONTEXT.md](../CONTEXT.md) 為準。

## 技術路線(摘要)

- **基礎**:LairWare 的 Macintosh 版 Ultima III(程式碼 MIT,© Leon McNeill)。
- **繪圖層**:以 SDL2 重建 Classic Mac QuickDraw / Toolbox API(`src/compat/` compat shim),
  讓上游 C 遊戲邏輯幾乎不改即可在 Linux 編譯執行。
- **文字繪製單一出口**:全遊戲繪字收斂於 `UDrawThemePascalString` / `UThemePascalStringWidth`,
  改接 **SDL_ttf + CJK TTF**(Noto Sans CJK),一處替換即全遊戲中文。
- **字串外部化**:原 Cocoa plist 字串改為執行期 **`.u3s`** 二進位表(`U3S1` magic + offset 表
  + UTF-8 blob),由 `tools/extract_strings.py` 從 `translations/*.json` 產生。

## 翻譯原則

1. **不改原始資料檔 offset**:中文走外部 `.u3s`,以 `(table, index)` 為 key 覆蓋;原始
   `.ULT`/資源不動,避免破壞位移。
2. **未翻譯自動回退英文**:build 時某句 `zh` 為空則用 `en`,確保畫面不缺字。
3. **標點全形化**:對話/訊息統一全形中文標點(,。!?:→:),與旁白風格一致。
4. **行寬限制**:走 Str255(255 byte)的舊路徑靠 `RewrapString` 切行;新繪字路徑
   `U3_DrawUTF8` 無此限。
5. **咒語不翻譯**:符文施法字(REPOND/MITTAR…)是玩家輸入的咒文,保留原文。

## 名詞決策(擇要,完整見 CONTEXT.md)

- **職業**:鬥士/牧師/巫師/竊賊/聖騎士/野蠻人/雲雀/幻術師/德魯伊/鍊金術士/遊俠。
- **種族**:人類/精靈/矮人/哈比族/毛絨族(Fuzzy)。
- **Exodus**:本專案視窗標題用「**末日**」;EA 官方繁中作「**魔胎**」(《創世紀3:魔胎》)。
  兩者並存,屬譯名取捨,非錯譯。
- **隊伍面板短碼**:右側 sidebar 受寬度限制,顯示「**種族全名(2字)+職業(1字)**」,
  例:精靈賊 / 人類俠 / 哈比牧 / 毛絨巫;等級標「**級**」、食物標「**糧**」、
  無法力職業的法力欄標「**不可施法**」(早期曾用「非法師」「不可魔法/無法力」等,均已淘汰)。
- **世界提示**:Pass(過一回合)譯「**等待**」(非「通過」);`<-WHAT?` 作「**←什麼?**」。

## 美術資產中文化

- **RaceClassInfo.gif**(創角的種族屬性/職業說明圖)原為整張英文圖。本專案以
  `tools/make_raceclass_gif.py` **重繪為繁中**(屬性數值取自原圖權威值),
  並於 `tools/package_appimage.sh` 打包時自動重生,確保 AppImage 內為中文版。
- 該圖屬執行期資產(`.gitignore`),故 repo 內**提交產生器**而非二進位;
  文件展示用的截圖另存於 `docs/screenshots/`。

## 已知踩雷與處置

- **缺字方框**:`絨`(U+7D68)經 `UPrint→NewPrint` 暫存 GWorld 路徑會顯示方框;改用
  `UDrawThemePascalString` 直繪(同 級/糧 標籤路徑)即正常。隊伍面板因此採直繪。
- **主選單裁切**:`WindowInit` 經 stub 的 `CGDisplayCurrentMode`(回 NULL)推算螢幕尺寸
  讀到未初始化值 → `blkSiz` 不確定變 32,以 2 倍繪入 640×480 埠而裁切。修法:讓
  `OriginalSize` 偏好預設 `true+exists`,固定 `blkSiz=16`(640×384)。
- **音訊無裝置**:`Mix_OpenAudio` 失敗只警告、停用音效,遊戲續跑(headless/無音效機可玩)。

## 測試與截圖注意事項

- **容器優先**:一律於 `u3cht` 容器內 build/test,不污染宿主;宿主缺 `sdl2` pkg-config 時,
  `tools/run_tests.sh` 須在容器執行。
- **測試掛勾(僅測試用,正式版不啟用)**:
  - `U3_SKIPINTRO=1` 略過片頭動畫直達主選單(讓 smoke 不受動畫時長影響);
  - `U3_DBG_SCENE=1` 在場景轉換印 `[SCENE] ...` 標記(主選單/選項/編組/世界/城堡);
  - `U3_TELEPORT=x,y`、`U3_SHOT_DIR`、`U3_SHOT_EVERY`、`U3_MAX_FRAMES` 供截圖/驅動。
- **AppImage release smoke**:`tools/smoke_appimage.sh` 可從 repo 外任意 cwd 跑
  `dist/*.AppImage`,以 `[SCENE]` 斷言五場景皆抵達;音訊警告不致命。
- **截圖規範**:展示用截圖放 `docs/screenshots/`(穩定檔名);臨時輸出走 `tests/_*`
  與 `tests/shots/`(均 `.gitignore`),不得提交。

## 參考

- 專案術語表:[CONTEXT.md](../CONTEXT.md)
- 可玩性/移植狀態:[GAMEPLAY-STATUS.md](GAMEPLAY-STATUS.md)
- 架構決策:`docs/adr/`
