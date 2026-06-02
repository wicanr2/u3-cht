# P3 — 上游檔案對 compat 層編譯狀態

> 以 `-fsyntax-only` + `mac_shim.h` umbrella 探測 (tools/compile_probe.sh)。
> 更新:2026-06-02

## 儀表板

| 檔案 | 大小 | QD 呼叫 | 狀態 | 備註 |
|---|---|---|---|---|
| UltimaSpellCombat.c | 42KB | 1 | ✅ OK | 純邏輯 |
| UltimaAutocombat.c | 20KB | 0 | ✅ OK | 純邏輯 |
| UltimaGraphics.c | 69KB | 69 | ✅ OK | 繪圖引擎,全靠 compat shim |
| UltimaDngn.c | 31KB | 27 | ✅ OK | 地牢繪圖 |
| UltimaNewMap.c | 18KB | 17 | ✅ OK | 地圖管理 |
| UltimaText.c | 25KB | 14 | ✅ OK | ⚠ 自帶 Mac 版 UDrawThemePascalString,build 時需 patch 掉改用 qd_text.c |
| UltimaMain.c | 107KB | 28 | 🟡 25 err | 主流程,少量 Mac 滲漏 |
| UltimaNew.c | 59KB | 80 | 🟡 33 err | 新遊戲+prefs |
| UltimaMisc.c | 87KB | 18 | 🟡 36 err | 雜項 |
| UltimaMacIF.c | 106KB | 93 | 🟠 53 err | 平台層,本就要以 SDL2 重寫 (見 P2 盤點) |
| UltimaAppleEvents.c | 2KB | — | 🟠 32 err | Mac AppleEvents,移植可丟棄 |

**6 / 11 檔案乾淨編譯**(含最大的繪圖引擎)。驗證 QuickDraw→SDL shim 可擴展到真實遊戲碼。

## compat umbrella 已涵蓋

mac_types + qd_geometry + quickdraw(GrafPort/PixMap/GDevice/CopyBits)+ qd_text +
CoreFoundation 最小子集(CFStringRef/CFURLRef/CFArray/CFAllocator/CFBoolean/CFSTR)+
Carbon 雜項(FSSpec/FSRef/Component/Snd/Dialog/Event/Pic/Poly handle、AlertType、
OSErr 常數、Theme/TE 對齊常數、pascal/TRUE/FALSE)。

## 下一步 backlog

1. **核心邏輯三檔 (Main/Misc/New)**:逐一補 umbrella 缺漏(多為共用 Mac 型別/常數),目標全綠。
2. **UltimaText.c patch**:#ifdef 掉自帶 UDrawThemePascalString/UThemePascalStringWidth,改用 qd_text.c(避免重複符號)。
3. **UltimaMacIF.c / AppleEvents**:依 P2 盤點以 SDL2 平台層重寫,非靠 shim 編譯。
4. 連結期:解 extern 全域 (gMainWindow、各 GWorld 埠…) + 實作 SDL 主迴圈 / 資源載入。
