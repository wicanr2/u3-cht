/* qd_text.h — QuickDraw 文字 + 主題字串 shim (SDL_ttf, UTF-8/CJK)
 *
 * 中文化核心 hook:UDrawThemePascalString / UThemePascalStringWidth。
 * 字串以 UTF-8 處理 (ASCII 與中文皆可);Str255 長度前綴沿用,
 * >255 byte 由上層 RewrapString 切行避免 (見 PLAN 風險 R2)。
 */
#ifndef U3_COMPAT_QD_TEXT_H
#define U3_COMPAT_QD_TEXT_H

#include "mac_types.h"

/* 設定字庫 TTF 路徑 (預設 Noto Sans CJK TC)。需在第一次繪字前呼叫。 */
void U3_SetFontPath(const char *path);
/* 釋放字型快取 (程式結束時) */
void U3_TextShutdown(void);

/* 目前埠文字狀態 */
void TextFont(SInt16 font);
void TextSize(SInt16 size);
void TextMode(SInt16 mode);
void TextFace(SInt16 face);

/* 經典 QuickDraw:在畫筆位置畫 Pascal 字串,畫後推進 pen.h */
void DrawString(ConstStr255Param s);
/* 字串像素寬 (目前字型) */
SInt16 StringWidth(ConstStr255Param s);

/* 主題字串 (U3 主要繪字出口) — 以目前埠 fgColor 繪製 */
OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID);
SInt16   UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID);

/* shim 輔助:UTF-8 C 字串直繪 (供新中文化碼直接使用,免 Pascal 轉換) */
void   U3_DrawUTF8(const char *utf8, SInt16 h, SInt16 v, SInt16 ptSize);
SInt16 U3_UTF8Width(const char *utf8, SInt16 ptSize);

#endif /* U3_COMPAT_QD_TEXT_H */
