/* weaken_upstream_text.h — 讓上游 UltimaText.c 自帶的 Mac Theme 繪字函式定義變 weak。
 *
 * 上游 UltimaText.c(第 73 / 106 行)自帶 UDrawThemePascalString / UThemePascalStringWidth
 * 的原始 Mac 實作;compat/qd_text.c 亦以 SDL_ttf CJK 版本定義同名強符號 → 連結期重複符號。
 * Linux/Windows build 靠 `objcopy --weaken-symbol` 把 UltimaText.o 那份弱化;macOS 無
 * objcopy(Apple 工具鏈不含),改在「編譯期」達成同效:在定義前先放一份 weak 宣告,clang
 * 便把後續的定義發成 weak 符號,連結時由 qd_text.c 的強符號覆蓋。全遊戲繪字統一走中文 hook。
 *
 * 僅在編譯 UltimaText.c 時以 -include 注入(其他 TU 只呼叫、不定義,無需注入)。
 * 型別(ConstStr255Param / ThemeFontID / OSStatus / SInt16)由先一步 -include 的 mac_shim.h
 * → qd_text.h 提供,故本檔不重複 include。 */
#ifndef U3_WEAKEN_UPSTREAM_TEXT_H
#define U3_WEAKEN_UPSTREAM_TEXT_H

OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID) __attribute__((weak));
SInt16   UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID) __attribute__((weak));

#endif
