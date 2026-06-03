/* qd_text.c — 文字 / 主題字串 shim 實作 (SDL_ttf, UTF-8/CJK) */
#include "qd_text.h"
#include "quickdraw.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_FONT "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"

static char gFontPath[1024] = DEFAULT_FONT;

/* 字型快取:依點數開檔 */
#define MAX_FONTS 16
static struct { int size; TTF_Font *font; } gCache[MAX_FONTS];
static int gCacheN = 0;
static Boolean gTTFInit = false;

void U3_SetFontPath(const char *path) {
    if (path && *path) {
        strncpy(gFontPath, path, sizeof(gFontPath) - 1);
        gFontPath[sizeof(gFontPath) - 1] = '\0';
    }
}

static TTF_Font *font_for_size(int size) {
    if (size <= 0) size = 12;
    if (!gTTFInit) {
        if (TTF_Init() != 0) return NULL;
        gTTFInit = true;
    }
    for (int i = 0; i < gCacheN; i++)
        if (gCache[i].size == size) return gCache[i].font;
    if (gCacheN >= MAX_FONTS) return gCache[0].font; /* 退而求其次 */
    TTF_Font *f = TTF_OpenFont(gFontPath, size);
    if (!f) return NULL;
    gCache[gCacheN].size = size;
    gCache[gCacheN].font = f;
    gCacheN++;
    return f;
}

void U3_TextShutdown(void) {
    for (int i = 0; i < gCacheN; i++)
        if (gCache[i].font) TTF_CloseFont(gCache[i].font);
    gCacheN = 0;
    if (gTTFInit) { TTF_Quit(); gTTFInit = false; }
}

/* Pascal → C (UTF-8 視之);回傳指向靜態緩衝。
 * 過濾 0x01-0x1F:UPrint 路徑的文字已 &0x7f,剩餘控制碼只會讓 SDL_ttf 顯示 [X]。
 * UTF-8 多位元組序列 (0x80-0xFF) 原樣保留,供翻譯後的中文字串正確渲染。 */
static const char *pascal_to_c(ConstStr255Param p) {
    static char buf[512];
    if (!p) { buf[0] = '\0'; return buf; }
    int len = p[0];
    if (len > 510) len = 510;
    int out = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)p[1 + i];
        if (c >= 0x20) buf[out++] = (char)c;
        /* 0x00-0x1F:Mac 格式控制碼/空字元,跳過;0x80-0xFF:UTF-8 多位元組,保留 */
    }
    buf[out] = '\0';
    return buf;
}

/* ThemeFontID → 點數:current port 用埠 txSize,其餘給預設 */
static int size_for_theme(ThemeFontID id) {
    CGrafPtr port = CurrentPort();
    int portSize = (port && port->txSize > 0) ? port->txSize : 12;
    switch (id) {
        case kThemeSmallSystemFont: return 11;
        case kThemeSystemFont:      return 12;
        case kThemeCurrentPortFont:
        default:                    return portSize;
    }
}

/* 核心:UTF-8 在 (h,v) 以 baseline 對齊繪入目前埠 */
static void draw_utf8_at(const char *utf8, int h, int v, int ptSize) {
    CGrafPtr port = CurrentPort();
    if (!port || !port->surface || !utf8 || !*utf8) return;
    TTF_Font *f = font_for_size(ptSize);
    if (!f) return;

    SDL_Color col = { 0, 0, 0, 255 };
    col.r = (Uint8)(port->fgColor.red   >> 8);
    col.g = (Uint8)(port->fgColor.green >> 8);
    col.b = (Uint8)(port->fgColor.blue  >> 8);

    SDL_Surface *txt = TTF_RenderUTF8_Blended(f, utf8, col);
    if (!txt) return;

    /* pen.v 視為 baseline → 上緣 = v - ascent */
    int ascent = TTF_FontAscent(f);
    SDL_Rect dst = { h - port->portRect.left,
                     v - ascent - port->portRect.top,
                     txt->w, txt->h };
    SDL_SetSurfaceBlendMode(txt, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(txt, NULL, port->surface, &dst);
    SDL_FreeSurface(txt);
}

static int utf8_width(const char *utf8, int ptSize) {
    TTF_Font *f = font_for_size(ptSize);
    if (!f || !utf8) return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(f, utf8, &w, &h);
    return w;
}

/* ===== 文字狀態 ===== */
void TextFont(SInt16 font) { CGrafPtr p = CurrentPort(); if (p) p->txFont = font; }
void TextSize(SInt16 size) { CGrafPtr p = CurrentPort(); if (p) p->txSize = size; }
void TextMode(SInt16 mode) { CGrafPtr p = CurrentPort(); if (p) p->txMode = mode; }
void TextFace(SInt16 face) { (void)face; }

/* ===== 經典 DrawString ===== */
void DrawString(ConstStr255Param s) {
    CGrafPtr p = CurrentPort();
    if (!p) return;
    int sz = (p->txSize > 0) ? p->txSize : 12;
    const char *c = pascal_to_c(s);
    draw_utf8_at(c, p->pen.h, p->pen.v, sz);
    p->pen.h += (SInt16)utf8_width(c, sz);  /* 推進畫筆 */
}

SInt16 StringWidth(ConstStr255Param s) {
    CGrafPtr p = CurrentPort();
    int sz = (p && p->txSize > 0) ? p->txSize : 12;
    return (SInt16)utf8_width(pascal_to_c(s), sz);
}

/* ===== 主題字串 (主要繪字出口) ===== */
OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID) {
    CGrafPtr p = CurrentPort();
    if (!p) return -1;
    int sz = size_for_theme(inFontID);
    const char *c = pascal_to_c(inPString);
    draw_utf8_at(c, p->pen.h, p->pen.v, sz);
    p->pen.h += (SInt16)utf8_width(c, sz);
    return noErr;
}

SInt16 UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID) {
    return (SInt16)utf8_width(pascal_to_c(inPString), size_for_theme(inFontID));
}

/* ===== UTF-8 直繪輔助 (新中文化碼用) ===== */
void U3_DrawUTF8(const char *utf8, SInt16 h, SInt16 v, SInt16 ptSize) {
    draw_utf8_at(utf8, h, v, ptSize);
}

/* ===== 選單按鈕中文標籤 =====
 * 按鈕圖 (Buttons.png) 文字烘焙為英文;DrawButton CopyBits 到 mainPort 後,
 * 此函式在按鈕矩形內取樣底色蓋掉英文,再以 SDL_ttf 置中畫中文。butNum 對應見下。 */
extern CGrafPtr mainPort;
void U3_DrawButtonLabel(int butNum, int left, int top, int right, int bottom) {
    static const char *kLabel[] = {
        "返回畫面", "編組隊伍", "啟程冒險", "建立角色", "刪除角色",
        "組成隊伍", "解散隊伍", "返回", "調整選項",
    };
    if (butNum < 0 || butNum > 8) return;
    const char *zh = kLabel[butNum];
    CGrafPtr p = mainPort;
    if (!p || !p->surface) return;
    SDL_Surface *s = p->surface;
    int w = right - left, h = bottom - top;
    if (w < 8 || h < 8) return;
    if (left < 0 || top < 0 || right > s->w || bottom > s->h) return;
    int stride = s->pitch / 4;
    Uint32 *px = (Uint32 *)s->pixels;
    /* 取樣按鈕底色 (左上內側,避開中央文字) 蓋掉英文 */
    Uint32 bg = px[(top + 4) * stride + (left + 4)];
    SDL_Rect fr = { left + 3, top + 3, w - 6, h - 6 };
    SDL_FillRect(s, &fr, bg);
    /* 黑色中文置中 (draw_utf8_at 以 v 為 baseline) */
    int size = (int)(h * 0.5);
    if (size < 12) size = 12;
    if (size > 20) size = 20;
    int tw = utf8_width(zh, size);
    TTF_Font *f = font_for_size(size);
    int asc = f ? TTF_FontAscent(f) : size;
    int fh = f ? TTF_FontHeight(f) : size;
    int x = left + (w - tw) / 2;
    int y = top + (h - fh) / 2 + asc;
    CGrafPtr save = CurrentPort();
    SetGWorld(mainPort, NULL);
    mainPort->fgColor.red = mainPort->fgColor.green = mainPort->fgColor.blue = 0;
    draw_utf8_at(zh, x, y, size);
    SetGWorld(save, NULL);
}
SInt16 U3_UTF8Width(const char *utf8, SInt16 ptSize) {
    return (SInt16)utf8_width(utf8, ptSize);
}

/* ===== 字型度量輔助 (供 DrawThemeTextBox/GetThemeTextDimensions) ===== */
int U3_ThemeFontSize(ThemeFontID id) { return size_for_theme(id); }
int U3_FontAscent(int ptSize) { TTF_Font *f = font_for_size(ptSize); return f ? TTF_FontAscent(f) : ptSize; }
int U3_FontHeight(int ptSize) { TTF_Font *f = font_for_size(ptSize); return f ? TTF_FontHeight(f) : ptSize; }
