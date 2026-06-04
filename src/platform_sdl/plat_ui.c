/* plat_ui.c — 視窗 / 對話框 / 控制項 / 選單 / 游標 / 文字 / Init / 顯示模式 /
 *              多邊形 / CarbonShunts (LW*) / 專案 Cocoa stub
 *
 * 規則 (見任務說明):
 *   視窗:NewWindow/GetNewCWindow 用 compat NewGWorld 建緩衝當視窗,回傳該 GrafPort;
 *         不開真 SDL_Window (由 main.c 統一顯示)。GetWindowPort/SetPortWindowPort = 視窗即埠。
 *   其餘 (對話框/控制項/選單/游標/多邊形/顯示模式/Movie 元件) 全 no-op。
 *
 * ⚠ 之後做「可玩」需補:游標顯示 (SetCursorNamed→SDL_Cursor)、
 *    對話框 (角色建立/Game Options→自繪 UI)。事件迴圈與 present 由 main.c/plat_event.c 處理。
 */
#include "mac_shim.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== 視窗 ===== */
/* 用 compat NewGWorld 建一塊與 bounds 同尺寸的 GrafPort 當視窗緩衝。 */
static WindowPtr make_window(const Rect *bounds) {
    Rect r;
    if (bounds) {
        r = *bounds;
    } else {
        r.left = r.top = 0; r.right = 640; r.bottom = 480;
    }
    /* NewGWorld 預期 bounds 由 0,0 起算的尺寸即可 */
    Rect local;
    local.left = 0; local.top = 0;
    local.right = r.right - r.left;
    local.bottom = r.bottom - r.top;
    GWorldPtr gw = NULL;
    NewGWorld(&gw, 32, &local, NULL, NULL, 0);
    return (WindowPtr)gw;
}

WindowPtr NewWindow(void *storage, const Rect *bounds, ConstStringPtr title,
                    Boolean visible, SInt16 procID, WindowPtr behind,
                    Boolean goAway, SInt32 refCon) {
    (void)storage; (void)title; (void)visible; (void)procID;
    (void)behind; (void)goAway; (void)refCon;
    return make_window(bounds);
}
WindowPtr NewCWindow(void *storage, const Rect *bounds, ConstStringPtr title,
                     Boolean visible, SInt16 procID, WindowPtr behind,
                     Boolean goAway, SInt32 refCon) {
    (void)storage; (void)title; (void)visible; (void)procID;
    (void)behind; (void)goAway; (void)refCon;
    return make_window(bounds);
}
WindowPtr GetNewCWindow(SInt16 windowID, void *storage, WindowPtr behind) {
    (void)windowID; (void)storage; (void)behind;
    return make_window(NULL);
}
CGrafPtr GetWindowPort(WindowPtr win) { return (CGrafPtr)win; }
void SetPortWindowPort(WindowPtr win) { if (win) SetGWorld((CGrafPtr)win, NULL); }

/* 視窗顯示 / 搬移 / 標題 / 更新 — no-op (main.c 處理顯示) */
void ShowWindow(WindowPtr w)               { (void)w; }
void HideWindow(WindowPtr w)               { (void)w; }
void DisposeWindow(WindowPtr w)            { if (w) DisposeGWorld((GWorldPtr)w); }
void BringToFront(WindowPtr w)             { (void)w; }
void MoveWindow(WindowPtr w, SInt16 h, SInt16 v, Boolean front) { (void)w; (void)h; (void)v; (void)front; }
void SizeWindow(WindowPtr w, SInt16 width, SInt16 height, Boolean update) { (void)w; (void)width; (void)height; (void)update; }
void DragWindow(WindowPtr w, Point startPt, const Rect *bounds) { (void)w; (void)startPt; (void)bounds; }
void SetWTitle(WindowPtr w, ConstStr255Param title) { (void)w; (void)title; }
void BeginUpdate(WindowPtr w)              { (void)w; }
void EndUpdate(WindowPtr w)                { (void)w; }
void InvalWindowRect(WindowPtr w, const Rect *r) { (void)w; (void)r; }
void TransitionWindow(WindowPtr w, UInt32 effect, UInt32 action, const void *opts) {
    (void)w; (void)effect; (void)action; (void)opts;
}
extern WindowPtr gMainWindow;
SInt16 FindWindow(Point thePoint, WindowPtr *theWindow) {
    (void)thePoint;
    /* 回主視窗 (非 NULL):否則 HandleMouseDown 會因 whichWindow==gShroudWindow(NULL) 而 break */
    if (theWindow) *theWindow = gMainWindow;
    return 3; /* inContent → HandleMouseDown default 分支處理按鈕 */
}
Boolean TrackGoAway(WindowPtr w, Point thePt) { (void)w; (void)thePt; return false; }

/* ===== 對話框 =====
 * 組隊對話框 (FormPartyDialog, GetNewDialog(BASERES+21=421)) 原為 Mac Dialog +
 * popup 選單,需玩家選 4 名角色。移植期以「自動選名冊前 4 名有效角色」協同驅動,
 * 讓 Journey 可進行 (Party[7]!=0)。FormPartyDialog 用 IDFP: FORM=1,MEM1..4=3..6。 */
extern unsigned char Player[21][65];   /* 上游名冊 (UltimaMain.c) */
static int  gAutoForm = 0;       /* 是否在組隊對話框自動模式 */
static int  gFormSeq = 0;        /* ModalDialog 步驟 */
static SInt16 gLastItem = 0;     /* 上次 ModalDialog 回的 item (供 GetControlValue) */
static char gDlgDummy;           /* 非 NULL dialog 佔位 */

/* 角色建立對話框 (BASERES+8=408):移植期協同自動建立。
 * 對話框 init 已用 SetControlValue/SetDialogItemText 預設性別/隨機名/種族/屬性,
 * 我們提供 per-item 值/文字 roundtrip 儲存,並讓 ModalDialog 直接回 IDCCD_CREATE,
 * OK 讀回即取得有效角色。IDCCD_CREATE=1, IDCCD_TYPE=21 (職業 popup)。 */
#define IDCCD_CREATE 1
#define IDCCD_NAME   3
#define IDCCD_SEX    19
#define IDCCD_RACE   20
#define IDCCD_TYPE   21
static int gAutoChar = 0;
static int gAutoRoster = 0;      /* RosterSelect (BASERES+10=410) 自動選空槽 */
static int gRosterSeq = 0;
static int gEmptySlot = 0;
static struct DlgItem { SInt16 value; unsigned char text[256]; } gItem[64];

static int roster_valid_count(void) {
    int n = 0;
    for (int i = 1; i < 21; i++) if (Player[i][0] > 22) n++;
    return n;
}
static int first_empty_slot(void) {       /* 第一個空名冊槽 (1-20),無則 0 */
    for (int i = 1; i < 21; i++) if (Player[i][0] <= 22) return i;
    return 0;
}

/* 互動式角色建立:玩家輸入名字 + 選職業 (職業決定種族/屬性預設,STR# 415)。
 * 渲染與讀鍵用遊戲既有 UPrintWin/UInputText/UInputNum (headless 由腳本餵鍵)。 */
extern int  wx, wy;
extern void UInputText(short x, short y, unsigned char *dest, short maxChar, Boolean numOnly);
extern short UInputNum(short x, short y);
extern void UPrintWin(unsigned char *gString);
extern void GetIndString(unsigned char *theString, SInt16 strListID, SInt16 index);
static void pstr(unsigned char *d, const char *s) {
    int n = 0; while (s[n] && n < 255) { d[n + 1] = (unsigned char)s[n]; n++; } d[0] = (unsigned char)n;
}
static void char_create_interactive(void) {
    unsigned char buf[256], nm[256];
    wx = 24; wy = 17;
    pstr(buf, "\n請輸入名字後按 Enter:\n"); UPrintWin(buf);
    UInputText(24, 19, nm, 12, false);
    if (nm[0] > 0) {
        int n = nm[0]; if (n > 12) n = 12;
        gItem[IDCCD_NAME].text[0] = (unsigned char)n;
        for (int i = 1; i <= n; i++) gItem[IDCCD_NAME].text[i] = nm[i];
    }
    wx = 24; wy = 20;
    pstr(buf, "\n職業:1鬥士2牧師3巫師4竊賊\n5聖騎士6野人7雲雀8幻術\n9德魯伊10鍊金11遊俠 選1-11:"); UPrintWin(buf);
    int c = UInputNum(24, 23);
    if (c < 1 || c > 11) c = 1;
    unsigned char ts[256];
    GetIndString(ts, 400 + 15, (SInt16)c);   /* STR# 415 職業預設 */
    if (ts[0] >= 14 && gEmptySlot > 0) {
        gItem[IDCCD_RACE].value = ts[2] - '0' + 1;
        Player[gEmptySlot][18] = (ts[4] - '0') * 10 + (ts[5] - '0');    /* STR */
        Player[gEmptySlot][19] = (ts[7] - '0') * 10 + (ts[8] - '0');    /* DEX */
        Player[gEmptySlot][20] = (ts[10] - '0') * 10 + (ts[11] - '0');  /* INT */
        Player[gEmptySlot][21] = (ts[13] - '0') * 10 + (ts[14] - '0');  /* WIS */
    }
    gItem[IDCCD_TYPE].value = c;
    /* 性別 (gItem[SEX]: 1=女 2=男;init 已隨機,玩家可改) */
    wx = 24; wy = 17;
    pstr(buf, "\n性別 1=女 2=男 (Enter 隨機):"); UPrintWin(buf);
    int sx = UInputNum(24, 18);
    if (sx == 1 || sx == 2) gItem[IDCCD_SEX].value = sx;
    /* 屬性分配:STR/DEX/INT 玩家輸入 (5-25,Enter 保留職業預設),WIS 自動補足 50 */
    if (gEmptySlot > 0) {
        int st[4]; for (int k = 0; k < 4; k++) st[k] = Player[gEmptySlot][18 + k];
        const char *an[3] = { "力量", "敏捷", "智力" };
        for (int k = 0; k < 3; k++) {
            char tmp[64]; snprintf(tmp, sizeof(tmp), "\n%s (5-25,Enter保留%d):", an[k], st[k]);
            wx = 24; wy = 19 + k; pstr(buf, tmp); UPrintWin(buf);
            int v = UInputNum(24, 20 + k);
            if (v >= 5 && v <= 25) st[k] = v;
        }
        int wis = 50 - (st[0] + st[1] + st[2]);   /* 智慧自動補足,使總和 50 */
        if (wis < 5) wis = 5;
        if (wis > 25) wis = 25;
        st[3] = wis;
        for (int k = 0; k < 4; k++) Player[gEmptySlot][18 + k] = st[k];
    }
}

DialogPtr GetNewDialog(SInt16 dialogID, void *storage, WindowPtr behind) {
    (void)storage; (void)behind;
    if (dialogID == 421) {       /* FormPartyDialog (BASERES+21) */
        gAutoForm = 1; gFormSeq = 0; gLastItem = 0;
        return (DialogPtr)&gDlgDummy;
    }
    if (dialogID == 408) {       /* CharacterCreateDialog (BASERES+8) */
        extern void U3_ProtectRange(void *lo, void *hi);
        gAutoChar = 1;
        for (int i = 0; i < 64; i++) { gItem[i].value = 0; gItem[i].text[0] = 0; }
        U3_ProtectRange(&gItem[0], &gItem[64]);   /* 防 ReleaseResource→DisposeHandle 誤 free 靜態 */
        return (DialogPtr)&gDlgDummy;
    }
    if (dialogID == 410) {       /* RosterSelect (BASERES+10) */
        extern void U3_ProtectRange(void *lo, void *hi);
        gAutoRoster = 1; gRosterSeq = 0; gEmptySlot = first_empty_slot();
        U3_ProtectRange(&gItem[0], &gItem[64]);
        return (DialogPtr)&gDlgDummy;
    }
    return NULL;
}
WindowPtr GetDialogWindow(DialogPtr dlg) { return (WindowPtr)dlg; }
void DisposeDialog(DialogPtr dlg) {
    (void)dlg;
    if (gAutoChar && getenv("U3_DBG_CHAR") && gEmptySlot > 0) {
        unsigned char *P = Player[gEmptySlot];
        char nm[16]; int j = 0;
        for (; j < 12 && P[j]; j++) nm[j] = (char)P[j];
        nm[j] = 0;
        fprintf(stderr, "[CHAR] slot=%d name=\"%s\" race=0x%02x class=0x%02x sex=%c STR=%d DEX=%d INT=%d WIS=%d\n",
                gEmptySlot, nm, P[22], P[23], P[24] ? P[24] : '?', P[18], P[19], P[20], P[21]);
    }
    gAutoForm = 0; gAutoChar = 0; gAutoRoster = 0;
}
void ModalDialog(ModalFilterUPP filter, SInt16 *itemHit) {
    (void)filter;
    if (gAutoForm) {
        /* 序列:MEM1(3)→MEM2(4)→MEM3(5)→MEM4(6)→FORM(1) */
        static const SInt16 seq[5] = { 3, 4, 5, 6, 1 };
        SInt16 it = (gFormSeq < 5) ? seq[gFormSeq] : 1;
        gFormSeq++;
        gLastItem = it;
        if (itemHit) *itemHit = it;
        return;
    }
    if (gAutoRoster) {
        /* 序列:先選第一個空槽 (item=slot+3),再按 OK(1)。OK 時上游設
         * Player[slot][0]=clss(預設 Fighter=1) → CharacterCreateDialog preset 有效。 */
        SInt16 it = (gRosterSeq == 0 && gEmptySlot > 0) ? (SInt16)(gEmptySlot + 3) : (SInt16)1;
        gRosterSeq++;
        if (itemHit) *itemHit = it;
        return;
    }
    if (gAutoChar) {
        char_create_interactive();   /* 玩家輸入名字 + 選職業 (重套預設) */
        if (itemHit) *itemHit = IDCCD_CREATE;
        return;
    }
    if (itemHit) *itemHit = kAlertStdAlertCancelButton;
}
/* 回非 1:HandleError 在 button==1 時會 ExitToShell,移植期避免誤退出 */
SInt16 Alert(SInt16 alertID, ModalFilterUPP filter) { (void)alertID; (void)filter; return 2; }
OSErr StandardAlert(AlertType type, ConstStringPtr error, ConstStringPtr explanation,
                    const AlertStdAlertParamRec *param, SInt16 *itemHit) {
    (void)type; (void)error; (void)explanation; (void)param;
    if (itemHit) *itemHit = kAlertStdAlertOKButton;
    return noErr;
}
/* 對話項以 gItem[item] 為穩定身分:供 Set/GetControlValue 與
 * Set/GetDialogItemText roundtrip (角色建立等對話框)。 */
static struct DlgItem *item_slot(SInt16 item) {
    return (item > 0 && item < 64) ? &gItem[item] : NULL;
}
void GetDialogItem(DialogPtr dlg, SInt16 item, SInt16 *kind, Handle *h, Rect *r) {
    (void)dlg;
    if (kind) *kind = 0;
    if (h) *h = (Handle)item_slot(item);
    if (r) { r->left = r->top = r->right = r->bottom = 0; }
}
OSErr GetDialogItemAsControl(DialogPtr dlg, SInt16 item, ControlRef *outControl) {
    (void)dlg; if (outControl) *outControl = (ControlRef)item_slot(item); return noErr;
}
void GetDialogItemText(Handle item, StringPtr text) {
    struct DlgItem *it = (struct DlgItem *)item;
    if (!text) return;
    if (it) { int n = it->text[0]; for (int i = 0; i <= n; i++) text[i] = it->text[i]; }
    else text[0] = 0;
}
void SetDialogItemText(Handle item, ConstStr255Param text) {
    struct DlgItem *it = (struct DlgItem *)item;
    if (!it || !text) return;
    int n = text[0]; if (n > 255) n = 255;
    for (int i = 0; i <= n; i++) it->text[i] = text[i];
}
void SelectDialogItemText(DialogPtr dlg, SInt16 item, SInt16 strtSel, SInt16 endSel) {
    (void)dlg; (void)item; (void)strtSel; (void)endSel;
}
void ParamText(ConstStr255Param a, ConstStr255Param b, ConstStr255Param c, ConstStr255Param d) {
    (void)a; (void)b; (void)c; (void)d;
}

/* ===== 控制項 ===== */
SInt16 GetControlValue(ControlRef c) {
    /* 自動組隊:第 k 個名冊 popup (gLastItem=MEM1..4=3..6 → k=0..3)。
     * popup value v: 1=不選;v>=2 → FormPartyDialog 取 menuEntry[v-2] (第 v-1 個有效角色)。
     * 名冊有第 (k+1) 個有效角色時回 k+2,否則回 1 (該槽空)。 */
    if (gAutoForm && gLastItem >= 3 && gLastItem <= 6) {
        int k = gLastItem - 3;
        return (roster_valid_count() > k) ? (SInt16)(k + 2) : (SInt16)1;
    }
    struct DlgItem *it = (struct DlgItem *)c;   /* 其他對話框:roundtrip 值 */
    return it ? it->value : 0;
}
void   SetControlValue(ControlRef c, SInt16 v)        { struct DlgItem *it = (struct DlgItem *)c; if (it) it->value = v; }
void   SetControlMaximum(ControlRef c, SInt16 m)      { (void)c; (void)m; }
void   GetControlTitle(ControlRef c, StringPtr title) { (void)c; if (title) title[0] = 0; }
void   SetControlTitle(ControlRef c, ConstStr255Param title) { (void)c; (void)title; }
void   HiliteControl(ControlRef c, SInt16 part)       { (void)c; (void)part; }
MenuRef GetControlPopupMenuHandle(ControlRef ctrl)    { (void)ctrl; return NULL; }

/* ===== 選單 ===== */
MenuHandle GetMenuHandle(SInt16 menuID)                  { (void)menuID; return NULL; }
MenuHandle GetNewMBar(SInt16 menuBarID)                  { (void)menuBarID; return NULL; }
void  SetMenuBar(MenuHandle mbar)                        { (void)mbar; }
void  DrawMenuBar(void)                                  {}
void  AppendMenu(MenuRef menu, ConstStr255Param data)    { (void)menu; (void)data; }
void  AppendResMenu(MenuRef menu, ResType type)          { (void)menu; (void)type; }
void  AppendMenuItemTextWithCFString(MenuRef menu, CFStringRef str, UInt32 attr,
                                     UInt32 cmdID, MenuItemIndex *outIndex) {
    (void)menu; (void)str; (void)attr; (void)cmdID;
    if (outIndex) *outIndex = 0;
}
void  DeleteMenuItem(MenuRef menu, SInt16 item)          { (void)menu; (void)item; }
void  CheckMenuItem(MenuRef menu, SInt16 item, Boolean checked) { (void)menu; (void)item; (void)checked; }
void  EnableMenuCommand(MenuRef menu, UInt32 cmdID)      { (void)menu; (void)cmdID; }
void  HiliteMenu(SInt16 menuID)                          { (void)menuID; }
SInt32 MenuSelect(Point startPt)                         { (void)startPt; return 0; }
SInt32 MenuKey(SInt16 ch)                                { (void)ch; return 0; }
SInt16 GetMBarHeight(void)                               { return 0; }
void  LMSetMBarHeight(SInt16 h)                          { (void)h; }
Boolean IsMenuBarVisible(void)                           { return false; }
/* 遊戲用法為 orgMenuColors = GetMCInfo();(無參數,取回傳 handle)。
 * 回非 NULL,否則 MenuBarInit 判定失敗 → HandleError → ExitToShell。 */
static char gDummyMCData[8];
static Ptr  gDummyMCPtr = gDummyMCData;
MCTableHandle GetMCInfo(void)                            { return (MCTableHandle)&gDummyMCPtr; }

/* ===== 游標 ===== */
void InitCursor(void)                  {}
void HideCursor(void)                  {}
void ShowCursor(void)                  {}
void ObscureCursor(void)               {}
void SetCursor(const void *crsr)       { (void)crsr; }
Boolean SetCursorNamed(CFStringRef name, float scale) { (void)name; (void)scale; return true; }
void LWSetArrowCursor(void)            {}

/* ===== 文字 (theme text / TextEdit / 字型);中文化繪字走 compat qd_text.c ===== */
void DrawText(const void *textBuf, SInt16 firstByte, SInt16 byteCount) {
    (void)textBuf; (void)firstByte; (void)byteCount;
}
/* Theme 文字:UltimaText.c 自帶的 UDrawThemePascalString 透過這兩個函式繪字
 * (intra-TU 呼叫無法被 objcopy weaken 重導向,故須在此用 SDL_ttf 真實作)。 */
extern void U3_DrawUTF8(const char *utf8, SInt16 h, SInt16 v, SInt16 ptSize);
extern SInt16 U3_UTF8Width(const char *utf8, SInt16 ptSize);
extern int U3_ThemeFontSize(ThemeFontID id);
extern int U3_FontAscent(int ptSize);
extern int U3_FontHeight(int ptSize);

OSStatus DrawThemeTextBox(CFStringRef str, ThemeFontID font, UInt32 state,
                          Boolean wrap, const Rect *bounds, SInt16 just, void *ctx) {
    (void)state; (void)wrap; (void)just; (void)ctx;
    if (!str || !bounds) return paramErr;
    int size = U3_ThemeFontSize(font);
    int asc = U3_FontAscent(size);
    /* bounds 由 UDrawThemePascalString 算出:top = baseline - ascent。
     * U3_DrawUTF8 以 v 為 baseline,故 baseline = bounds->top + ascent。 */
    U3_DrawUTF8((const char *)str, bounds->left, (SInt16)(bounds->top + asc), (SInt16)size);
    return noErr;
}
OSStatus GetThemeTextDimensions(CFStringRef str, ThemeFontID font, UInt32 state,
                                Boolean wrap, Point *ioBounds, SInt16 *baseline) {
    (void)state; (void)wrap;
    int size = U3_ThemeFontSize(font);
    int w = str ? U3_UTF8Width((const char *)str, size) : 0;
    int h = U3_FontHeight(size);
    int asc = U3_FontAscent(size);
    if (ioBounds) { ioBounds->h = (SInt16)w; ioBounds->v = (SInt16)h; }
    if (baseline) *baseline = (SInt16)(-(h - asc));   /* 負值 (descent) */
    return noErr;
}
OSStatus SetAntiAliasedTextEnabled(Boolean enable, SInt16 minSize) { (void)enable; (void)minSize; return noErr; }
void TEInit(void) {}
void InitFonts(void) {}
void GetFNum(ConstStr255Param name, SInt16 *familyID) { (void)name; if (familyID) *familyID = 0; }

/* ===== 繪圖補充 (CopyBits 圈外) ===== */
void DrawPicture(PicHandle pic, const Rect *r) { (void)pic; (void)r; }
void FrameRoundRect(const Rect *r, SInt16 ovalWidth, SInt16 ovalHeight) {
    (void)r; (void)ovalWidth; (void)ovalHeight;
}
void InvertRect(const Rect *r)   { (void)r; }
void ScrollRect(const Rect *r, SInt16 dh, SInt16 dv, RgnHandle updateRgn) {
    (void)r; (void)dh; (void)dv; (void)updateRgn;
}
void OpColor(const RGBColor *c)  { (void)c; }
void GetCPixel(SInt16 h, SInt16 v, RGBColor *cPix) {
    (void)h; (void)v; if (cPix) { cPix->red = cPix->green = cPix->blue = 0; }
}
/* 座標轉換 (視窗即埠,座標等同) */
void LocalToGlobal(Point *pt) { (void)pt; }
void GlobalToLocal(Point *pt) { (void)pt; }
void GetPen(Point *pt) { if (pt) { CGrafPtr p = CurrentPort(); pt->h = p ? p->pen.h : 0; pt->v = p ? p->pen.v : 0; } }

/* ===== 多邊形 (U3 用於某些繪圖;移植期 no-op) ===== */
PolyHandle OpenPoly(void)                  { return NULL; }
void ClosePoly(void)                       {}
void KillPoly(PolyHandle poly)             { (void)poly; }
void PaintPoly(PolyHandle poly)            { (void)poly; }

/* ===== Init 類 ===== */
void InitGraf(void *globalsPtr)            { (void)globalsPtr; }
void InitWindows(void)                     {}
void InitMenus(void)                       {}
void InitDialogs(void *resumeProc)         { (void)resumeProc; }
Boolean InitializeAEHandlers(void)         { return true; }
OSErr AEProcessAppleEvent(const void *event) { (void)event; return noErr; }

/* ===== 顯示模式 (CoreGraphics / Display Manager;停用) ===== */
CFDictionaryRef CGDisplayCurrentMode(CGDirectDisplayID display) { (void)display; return NULL; }
CFDictionaryRef CGDisplayBestModeForParameters(CGDirectDisplayID display, size_t bpp,
                                               size_t width, size_t height, Boolean *exactMatch) {
    (void)display; (void)bpp; (void)width; (void)height;
    if (exactMatch) *exactMatch = false;
    return NULL;
}
CGError CGDisplaySwitchToMode(CGDirectDisplayID display, CFDictionaryRef mode) {
    (void)display; (void)mode; return kCGErrorSuccess;
}
OSErr DMGetFirstScreenDevice(Boolean activeOnly, GDHandle *theDevice) {
    (void)activeOnly; if (theDevice) *theDevice = NULL; return noErr;
}
OSErr DMGetNextScreenDevice(GDHandle *theDevice, Boolean activeOnly) {
    (void)activeOnly; if (theDevice) *theDevice = NULL; return noErr;
}

/* ===== CarbonShunts (LW*) — 原定義於排除的 CarbonShunts.c =====
 * 注意:ForceUpdateMain 由 coordinator 在 main.c 實作 (present 到視窗),此處不實作。 */
void LWSetDialogPort(DialogPtr theDialog)                   { (void)theDialog; }
void LWGetScreenRect(Rect *rect) {
    if (rect) { rect->left = 0; rect->top = 0; rect->right = 640; rect->bottom = 480; }
}
OSErr LWGetDialogControl(DialogRef inDialog, SInt16 inItemNo, ControlRef *outControl) {
    (void)inDialog; (void)inItemNo; if (outControl) *outControl = NULL; return noErr;
}
void LWGetPortForeColor(CGrafPtr port, RGBColor *color) {
    if (color) { *color = port ? port->fgColor : (RGBColor){0,0,0}; }
}
void LWGetPortBackColor(CGrafPtr port, RGBColor *color) {
    if (color) { *color = port ? port->bgColor : (RGBColor){0xFFFF,0xFFFF,0xFFFF}; }
}
void LWGetPortBounds(CGrafPtr port, Rect *bounds) {
    if (bounds) { *bounds = port ? port->portRect : (Rect){0,0,0,0}; }
}
void LWGetPortPenLocation(CGrafPtr port, Point *point) {
    if (point) { *point = port ? port->pen : (Point){0,0}; }
}
void LWGetWindowBounds(WindowRef window, Rect *bounds) {
    if (bounds) { *bounds = window ? window->portRect : (Rect){0,0,0,0}; }
}
Boolean LWIsControlActive(ControlHandle control)            { (void)control; return false; }
Boolean LWIsMenuItemEnabled(MenuRef menu, MenuItemIndex item) { (void)menu; (void)item; return true; }
void LWDisableMenuItem(MenuRef theMenu, short item)         { (void)theMenu; (void)item; }
void LWEnableMenuItem(MenuRef theMenu, short item)          { (void)theMenu; (void)item; }
OSStatus LWValidWindowRect(WindowRef window, const Rect *bounds) { (void)window; (void)bounds; return noErr; }
OSStatus LWInvalWindowRect(WindowRef window, const Rect *bounds) { (void)window; (void)bounds; return noErr; }
Boolean GoodHandle(Handle h)                               { return h != NULL; }
void LWBlockZero(void *destPtr, Size byteCount) {
    if (destPtr && byteCount > 0) memset(destPtr, 0, (size_t)byteCount);
}
void DefineDefaultItem(DialogPtr theDialog, short item)    { (void)theDialog; (void)item; }

/* ===== 專案 Cocoa stub (原定義於排除的 .m / CocoaBridge) ===== */
void LWOpenURL(CFStringRef urlString)      { (void)urlString; }
void SetRefMenuIcons(MenuRef theMenu)      { (void)theMenu; }
int  EducateAboutFullScreen(void)          { return 0; }
/* 遊戲設定畫面 (取代 PrefsDialog.m 的 Cocoa 對話):列出可切換選項,鍵盤操作。
 * 重用 HandleSpecialChoice (切換偏好 + 副作用);偏好持久化於 cf_bridge。
 * 偏好為反向旗標 (Inactive/Manual/Unconstrain),顯示時轉成「開/關」。 */
void GameOptionsDialog(void) {
    extern char CursorKey(Boolean usePenLoc);
    extern void HandleSpecialChoice(int theItem);
    extern void ClearBottom(void);
    extern void ForceUpdateMain(void);
    extern Boolean gDone;
    extern Boolean CFPreferencesGetAppBooleanValue(CFStringRef, CFStringRef, Boolean *);
    extern CFStringRef U3PrefSoundInactive, U3PrefMusicInactive, U3PrefManualCombat,
                       U3PrefSpeedUnconstrain, kCFPreferencesCurrentApplication;
    extern int  U3_GetGameSpeedFps(void);
    extern void U3_SetGameSpeedFps(int fps);
    #define PREF_OFF(k) CFPreferencesGetAppBooleanValue(k, kCFPreferencesCurrentApplication, NULL)
    if (getenv("U3_DBG_SCENE")) fprintf(stderr, "[SCENE] options-open\n");
    unsigned char buf[256];
    int done = 0;
    while (!done && !gDone) {
        Boolean snd  = !PREF_OFF(U3PrefSoundInactive);
        Boolean mus  = !PREF_OFF(U3PrefMusicInactive);
        Boolean autc = !PREF_OFF(U3PrefManualCombat);
        Boolean fast =  PREF_OFF(U3PrefSpeedUnconstrain);
        int fps = U3_GetGameSpeedFps();
        const char *spd = (fps >= 60) ? "快" : (fps >= 30) ? "中" : "慢";
        char t[256];
        /* 逐行繪製,各行各自 UPrintWin 於明確 wy (13-18)。
         * 不用單一含 \n 的長字串:UPrint 對多換行會逐 UIncTy 累加 ty,
         * 6 行會讓末行落到 ty=20,NewPrint 在該列會卡住 (上游繪字邊界問題);
         * 逐行則每次 ty=wy 固定、單次 NewPrint,ty 不超過 18,穩定。 */
        ClearBottom();
        wx = 24; wy = 13; pstr(buf, "遊戲設定 (數字切換,0返回):"); UPrintWin(buf);
        snprintf(t, sizeof(t), " 1 音效 [%s]",     snd  ? "開" : "關"); wx = 24; wy = 14; pstr(buf, t); UPrintWin(buf);
        snprintf(t, sizeof(t), " 2 音樂 [%s]",     mus  ? "開" : "關"); wx = 24; wy = 15; pstr(buf, t); UPrintWin(buf);
        snprintf(t, sizeof(t), " 3 自動戰鬥 [%s]", autc ? "開" : "關"); wx = 24; wy = 16; pstr(buf, t); UPrintWin(buf);
        snprintf(t, sizeof(t), " 4 快速移動 [%s]", fast ? "開" : "關"); wx = 24; wy = 17; pstr(buf, t); UPrintWin(buf);
        snprintf(t, sizeof(t), " 5 速度 [%s]",     spd);                wx = 24; wy = 18; pstr(buf, t); UPrintWin(buf);
        ForceUpdateMain();
        char k = CursorKey(false);
        if (k > 95) k -= 32;
        switch (k) {
            case '1': HandleSpecialChoice(1); break;   /* SOUNDID */
            case '2': HandleSpecialChoice(2); break;   /* MUSICID */
            case '3': HandleSpecialChoice(5); break;   /* AUTOCOMBATID */
            case '4': HandleSpecialChoice(4); break;   /* CONSTRAINID */
            case '5':   /* 速度循環 慢(20)→中(30)→快(60)→慢 */
                U3_SetGameSpeedFps(fps >= 60 ? 20 : fps >= 30 ? 60 : 30);
                break;
            case '0': case 13: case 27: case 'Q': done = 1; break;
        }
    }
    #undef PREF_OFF
}

/* 指令表 overlay 由 qd_text.c:U3_DrawHelpOverlay 於 present 時繪製,
 * 開關 gHelpOverlay 由 plat_event.c (F1 / 腳本 H) 切換,不在此檔。 */

/* ===== 系統事件雜項 (非 main.c 範圍的) ===== */
void SystemClick(const EventRecord *event, WindowPtr window) { (void)event; (void)window; }
