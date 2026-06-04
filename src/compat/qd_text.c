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
void U3_DrawTextLabel(const char *zh, int left, int top, int right, int bottom);

/* 在 [textTop,bottom] 文字帶內「逐列重建背景」抹去烘焙英文,再置中畫中文。
 * 逐列取左右 margin 較亮像素當該列底色 (英文為深色;取較亮者避開字身),只填該帶 →
 * 保留上方插畫 (textTop 以上不動) 與按鈕漸層/bevel 質感 (逐列保留垂直漸層)。 */
static void label_reconstruct(const char *zh, int left, int top, int right, int bottom,
                              int textTop, int cr, int cg, int cb) {
    if (!zh || !*zh) return;
    CGrafPtr p = mainPort;
    if (!p || !p->surface) return;
    SDL_Surface *s = p->surface;
    int w = right - left, h = bottom - top;
    if (w < 8 || h < 8) return;
    if (left < 0 || top < 0 || right > s->w || bottom > s->h) return;
    int stride = s->pitch / 4;
    Uint32 *px = (Uint32 *)s->pixels;
    const int pad = 3;                       /* 內縮避開外框/圓角 bevel */
    int sxL = left + pad, sxR = right - 1 - pad;
    int fy0 = textTop + pad; if (fy0 < top + pad) fy0 = top + pad;
    int fy1 = bottom - pad;
    int fx0 = left + pad, fx1 = right - pad;
    for (int y = fy0; y < fy1; y++) {
        Uint32 cl = px[y * stride + sxL], crr = px[y * stride + sxR];
        int lumL = ((cl >> 16) & 255) + ((cl >> 8) & 255) + (cl & 255);
        int lumR = ((crr >> 16) & 255) + ((crr >> 8) & 255) + (crr & 255);
        Uint32 bg = (lumL >= lumR) ? cl : crr;   /* 較亮者 = 底色 (避開深色英文) */
        for (int x = fx0; x < fx1; x++) px[y * stride + x] = bg;
    }
    int regTop = textTop, regH = bottom - textTop;
    int size = (int)(regH * 0.5);
    if (size < 12) size = 12;
    if (size > 22) size = 22;
    int tw = utf8_width(zh, size);
    TTF_Font *f = font_for_size(size);
    int asc = f ? TTF_FontAscent(f) : size;
    int fh = f ? TTF_FontHeight(f) : size;
    int x = left + (w - tw) / 2;
    int y = regTop + (regH - fh) / 2 + asc;
    CGrafPtr save = CurrentPort();
    SetGWorld(mainPort, NULL);
    mainPort->fgColor.red = (unsigned short)(cr << 8);
    mainPort->fgColor.green = (unsigned short)(cg << 8);
    mainPort->fgColor.blue = (unsigned short)(cb << 8);
    draw_utf8_at(zh, x, y, size);
    SetGWorld(save, NULL);
}

/* 9 個主選單/Organize 按鈕 (Buttons.png) 的中文標籤,butNum 對應見 DrawButton。
 * 大按鈕 0/1/2/8 = 上方插畫 + 下方英文 → 只重建下方 ~47% 文字帶,保留插畫;
 * 小按鈕 3-7 = 純文字 → 逐列重建整顆 (保留漸層)。pressed/dim 由即時取樣底色自動相容。 */
void U3_DrawButtonLabel(int butNum, int left, int top, int right, int bottom) {
    static const char *kLabel[] = {
        "返回畫面", "編組隊伍", "啟程冒險", "建立角色", "刪除角色",
        "組成隊伍", "解散隊伍", "返回", "調整選項",
    };
    if (butNum < 0 || butNum > 8) return;
    int hasIcon = (butNum == 0 || butNum == 1 || butNum == 2 || butNum == 8);
    int textTop = hasIcon ? top + (int)((bottom - top) * 0.53) : top;   /* 插畫佔上方 53% */
    label_reconstruct(kLabel[butNum], left, top, right, bottom, textTop, 0, 0, 0);
}

/* 在 mainPort 的矩形內取樣底色「整塊」蓋掉烘焙英文,再以指定顏色置中畫中文。
 * 用於實心標題列 (如 Ztats「角色記錄」藍底),無插畫/漸層需保留。 */
void U3_DrawTextLabelC(const char *zh, int left, int top, int right, int bottom,
                       int cr, int cg, int cb) {
    if (!zh || !*zh) return;
    CGrafPtr p = mainPort;
    if (!p || !p->surface) return;
    SDL_Surface *s = p->surface;
    int w = right - left, h = bottom - top;
    if (w < 8 || h < 8) return;
    if (left < 0 || top < 0 || right > s->w || bottom > s->h) return;
    int stride = s->pitch / 4;
    Uint32 *px = (Uint32 *)s->pixels;
    Uint32 bg = px[(top + 4) * stride + (left + 4)];   /* 取樣底色蓋掉英文 */
    SDL_Rect fr = { left + 3, top + 3, w - 6, h - 6 };
    SDL_FillRect(s, &fr, bg);
    int size = (int)(h * 0.5);
    if (size < 12) size = 12;
    if (size > 22) size = 22;
    int tw = utf8_width(zh, size);
    TTF_Font *f = font_for_size(size);
    int asc = f ? TTF_FontAscent(f) : size;
    int fh = f ? TTF_FontHeight(f) : size;
    int x = left + (w - tw) / 2;
    int y = top + (h - fh) / 2 + asc;
    CGrafPtr save = CurrentPort();
    SetGWorld(mainPort, NULL);
    mainPort->fgColor.red = (unsigned short)(cr << 8);
    mainPort->fgColor.green = (unsigned short)(cg << 8);
    mainPort->fgColor.blue = (unsigned short)(cb << 8);
    draw_utf8_at(zh, x, y, size);
    SetGWorld(save, NULL);
}
/* 小按鈕中文標籤 (如 Ztats 分配食物/收集金幣):逐列重建保留按鈕漸層質感。 */
void U3_DrawTextLabel(const char *zh, int left, int top, int right, int bottom) {
    label_reconstruct(zh, left, top, right, bottom, top, 0, 0, 0);   /* 黑字,純文字按鈕 */
}
SInt16 U3_UTF8Width(const char *utf8, SInt16 ptSize) {
    return (SInt16)utf8_width(utf8, ptSize);
}

/* ===== 字型度量輔助 (供 DrawThemeTextBox/GetThemeTextDimensions) ===== */
int U3_ThemeFontSize(ThemeFontID id) { return size_for_theme(id); }
int U3_FontAscent(int ptSize) { TTF_Font *f = font_for_size(ptSize); return f ? TTF_FontAscent(f) : ptSize; }
int U3_FontHeight(int ptSize) { TTF_Font *f = font_for_size(ptSize); return f ? TTF_FontHeight(f) : ptSize; }

/* ===== F1 指令表 Help overlay =====
 * 在 main.c 的 present 之後、RenderPresent 之前呼叫,於整個視窗上覆蓋一張半透明
 * 深色面板 + 繁中指令表。純視覺,不動遊戲狀態;切換由 plat_event.c 的 F1 處理。
 * 自帶字型 (gFontPath),與遊戲畫面同字庫。 */
static void overlay_line(SDL_Renderer *ren, TTF_Font *f, const char *s,
                         int x, int y, SDL_Color col) {
    if (!s || !*s) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    if (tex) {
        SDL_Rect dst = { x, y, surf->w, surf->h };
        SDL_RenderCopy(ren, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void U3_DrawHelpOverlay(SDL_Renderer *ren, int winW, int winH) {
    if (!ren) return;
    /* 半透明深色底 (蓋住整個視窗,留邊框) */
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 16, 222);
    SDL_Rect bg = { 0, 0, winW, winH };
    SDL_RenderFillRect(ren, &bg);

    int pt = winH / 26; if (pt < 12) pt = 12; if (pt > 22) pt = 22;
    TTF_Font *f = font_for_size(pt);
    if (!f) return;
    int lh = TTF_FontHeight(f) + 2;
    SDL_Color title = { 255, 230, 90, 255 };
    SDL_Color head  = { 120, 220, 255, 255 };
    SDL_Color body  = { 235, 235, 235, 255 };

    int y = lh;
    overlay_line(ren, f, "指令表  (按 F1 或任意鍵關閉)", winW / 2 - winW / 4, y, title);
    y += lh * 2;

    /* 兩欄:左欄移動/進入互動,右欄狀態選單/戰鬥 */
    int colL = winW / 16, colR = winW / 2 + winW / 16;
    const char *left[] = {
        "= 移動 / 進入 =",
        "方向鍵  移動 / 行走",
        "空白    等待 (過一回合)",
        "E  進入城鎮 / 城堡 / 地城",
        "T  交談 / 交易 (對 NPC)",
        "B  登乘 (船 / 馬 / 載具)",
        "X  離開載具 / 下船",
        "G  開啟寶箱",
        "K  攀爬 (上 / 下樓)",
        "D  下樓 (地城)",
        "U  開鎖    I  點火把",
        "L  查看 (地城)",
        NULL };
    const char *right[] = {
        "= 狀態 / 選單 =",
        "Z  角色狀態 (Ztats)",
        "M  調整隊伍順序",
        "R  備妥武器  W  穿戴防具",
        "J  分配金幣  Q  存檔",
        "F1 顯示 / 關閉本指令表",
        "F2 顏色:彩色/綠磷光/琥珀/灰階",
        "F3 圖塊:標準/DOS/Apple2/C64...",
        "",
        "= 戰鬥 / 法術 =",
        "A  攻擊      C  施展法術",
        "F  發射船炮  S  偷竊",
        "Y  呼喊 (升 / 降帆)",
        "P  以寶石俯瞰地圖",
        NULL };
    int yy = y;
    for (int i = 0; left[i]; i++) {
        SDL_Color c = (left[i][0] == '=') ? head : body;
        overlay_line(ren, f, left[i], colL, yy, c);
        yy += lh;
    }
    yy = y;
    for (int i = 0; right[i]; i++) {
        SDL_Color c = (right[i][0] == '=') ? head : body;
        overlay_line(ren, f, right[i], colR, yy, c);
        yy += lh;
    }
}
