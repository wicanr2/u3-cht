/* plat_event.c — 事件泵 + present 觸發 + game tester 腳本輸入。
 *
 * 上游遊戲迴圈每輪呼叫 WaitNextEvent / GetKeyMouse;以此為 frame 邊界:
 * 每次先 present 畫面,再抽 SDL 事件翻成 Mac EventRecord。
 *
 * game tester:環境變數 U3_SCRIPT 指向腳本檔,內容逐 byte 當鍵盤輸入注入。
 * 特殊:每行尾 \n → Return(0x0D);可用以驅動建角色/移動等流程。 */
#include "mac_shim.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

extern int gU3Done;
void U3_PlatPresent(void);
extern void HandleMouseDown(void);   /* UltimaMacIF.c:原由 Cocoa 事件迴圈呼叫 */
extern EventRecord gTheEvent;        /* 上游全域事件記錄 */
extern short gUpdateWhere;           /* 上游目前畫面狀態:5=主選單,6=Organize,3=世界 */

/* --- 腳本輸入 (game tester) ---
 * 行導向迷你語言 (每行一指令,每次 WaitNextEvent 處理一步):
 *   K<text>   逐字元送鍵盤 (該行讀完再進下一行)
 *   C x y     送滑鼠點擊於 (x,y)
 *   R         送 Return
 *   W <n>     等待 n 次 (不送事件)
 *   U <n>     等到 gUpdateWhere == n (5=主選單,6=組隊選單,3=世界)
 *   #...      註解 / 空行 略過
 */
static FILE *gScript = NULL;
static int   gScriptDone = 0;
static int   gScriptInit = 0;
static char  gLine[256];
static int   gLinePos = -1;     /* >=0 表示正在送 K 行的字元 */
static int   gWait = 0;
static int   gWaitUpdate = -1;
static int   gPendClickX = -1, gPendClickY = -1;

static void script_init(void) {
    gScriptInit = 1;
    const char *p = getenv("U3_SCRIPT");
    if (p && *p) gScript = fopen(p, "rb");
}
/* 取下一步:回傳 Mac key code,或 -1 (無鍵;可能設了 pending click 或 wait) */
static int script_next_key(void) {
    if (!gScriptInit) script_init();
    if (!gScript || gScriptDone) return -1;
    if (gWait > 0) { gWait--; return -1; }
    if (gWaitUpdate >= 0) {
        if (gUpdateWhere != gWaitUpdate) return -1;
        gWaitUpdate = -1;
    }
    /* 正在送 K 行 */
    if (gLinePos >= 0) {
        char c = gLine[gLinePos];
        if (c == '\0' || c == '\n') { gLinePos = -1; }
        else { gLinePos++; return (unsigned char)c; }
    }
    /* 讀新行 */
    while (fgets(gLine, sizeof(gLine), gScript)) {
        char *s = gLine;
        if (*s=='#' || *s=='\n' || *s=='\0') continue;
        char cmd = *s++;
        while (*s==' ') s++;
        if (getenv("U3_DBG_SCRIPT")) fprintf(stderr, "[SCRIPT] cmd=%c arg=%.20s gUpdateWhere=%d\n", cmd, s, gUpdateWhere);
        if (cmd=='K') { gLinePos = (int)(s - gLine); return script_next_key(); }
        if (cmd=='R') return 0x0D;
        if (cmd=='M') { int k = atoi(s); return k > 0 ? k : -1; }  /* 原始 Mac 鍵碼 (方向鍵 28-31) */
        if (cmd=='W') { gWait = atoi(s); return -1; }
        if (cmd=='U') { gWaitUpdate = atoi(s); return script_next_key(); }
        if (cmd=='C') { int x=0,y=0; if(sscanf(s,"%d %d",&x,&y)==2){gPendClickX=x;gPendClickY=y;} return -1; }
    }
    gScriptDone = 1; fclose(gScript); gScript = NULL;
    return -1;
}

/* SDL keycode → Mac char code */
static int sdl_to_mac_key(SDL_Keycode k, Uint16 mod, int *ok) {
    *ok = 1;
    switch (k) {
        case SDLK_LEFT:   return 0x1C;
        case SDLK_RIGHT:  return 0x1D;
        case SDLK_UP:     return 0x1E;
        case SDLK_DOWN:   return 0x1F;
        case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
        case SDLK_ESCAPE: return 0x1B;
        case SDLK_BACKSPACE: return 0x08;
        case SDLK_TAB:    return 0x09;
        case SDLK_SPACE:  return 0x20;
        default: break;
    }
    if (k >= SDLK_a && k <= SDLK_z) {
        int ch = (int)(k - SDLK_a) + 'a';
        if (mod & (KMOD_SHIFT)) ch = ch - 'a' + 'A';
        return ch;
    }
    if (k >= SDLK_0 && k <= SDLK_9) return (int)(k - SDLK_0) + '0';
    if (k < 128 && k > 0) return (int)k;
    *ok = 0;
    return 0;
}

/* 合成滑鼠 (腳本點擊用):點擊後數幀內 GetMouse 回該點、StillDown 回 true */
static int gSynthX = -1, gSynthY = -1, gSynthDown = 0;

/* 視窗像素 → 邏輯畫布座標 (放大視窗時必經;實作於 main.c) */
extern void U3_MapMouse(int *x, int *y);

Boolean WaitNextEvent(short mask, EventRecord *evt, unsigned long sleep, RgnHandle rgn) {
    (void)mask; (void)sleep; (void)rgn;
    U3_PlatPresent();   /* frame 邊界:先把上一幀畫上螢幕 */

    if (gSynthDown > 0) gSynthDown--;
    if (evt) { evt->what = nullEvent; evt->message = 0; evt->modifiers = 0; }

    /* 腳本輸入優先 */
    int sk = script_next_key();
    if (gPendClickX >= 0 && evt) {   /* 腳本點擊 → 合成 mouseDown 並自行分派 */
        evt->what = mouseDown;
        evt->where.h = (SInt16)gPendClickX;
        evt->where.v = (SInt16)gPendClickY;
        gSynthX = gPendClickX; gSynthY = gPendClickY; gSynthDown = 4;
        gPendClickX = gPendClickY = -1;
        /* 原由 Cocoa 事件迴圈分派;此處補上:填 gTheEvent 後呼叫 HandleMouseDown。
         * HandleButtonClick 同步輪詢 StillDown/GetMouse;處理完才清合成滑鼠,
         * 確保整個輪詢期間 GetMouse 都回點擊座標 (放開時仍在按鈕上 → 判定成功)。 */
        gTheEvent = *evt;
        HandleMouseDown();
        gSynthX = gSynthY = -1; gSynthDown = 0;
        return true;
    }
    if (sk >= 0 && evt) {
        evt->what = keyDown;
        evt->message = (UInt32)(sk & charCodeMask);
        return true;
    }

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { gU3Done = 1; exit(0); }
        if (ev.type == SDL_KEYDOWN && evt) {
            int ok = 0;
            int mc = sdl_to_mac_key(ev.key.keysym.sym, ev.key.keysym.mod, &ok);
            if (ok) {
                evt->what = keyDown;
                evt->message = (UInt32)(mc & charCodeMask);
                return true;
            }
        } else if (ev.type == SDL_MOUSEBUTTONDOWN && evt) {
            int mx = ev.button.x, my = ev.button.y;
            U3_MapMouse(&mx, &my);
            evt->what = mouseDown;
            evt->where.h = (SInt16)mx;
            evt->where.v = (SInt16)my;
            return true;
        }
    }
    SDL_Delay(1);   /* 避免空轉吃滿 CPU */
    return false;
}

void ForceUpdateMain(void) { U3_PlatPresent(); }

void FlushEvents(short eventMask, short stopMask) {
    (void)eventMask; (void)stopMask;
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) { gU3Done = 1; exit(0); }
    }
}

void GetMouse(Point *pt) {
    if (gSynthX >= 0) {   /* 合成點擊期間始終回點擊座標 (含放開瞬間) */
        if (pt) { pt->h = (SInt16)gSynthX; pt->v = (SInt16)gSynthY; }
        return;
    }
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    U3_MapMouse(&x, &y);
    if (pt) { pt->h = (SInt16)x; pt->v = (SInt16)y; }
}

Boolean Button(void) {
    if (gSynthDown > 0) return true;
    return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) ? true : false;
}

Boolean StillDown(void) {
    if (gSynthDown > 0) { gSynthDown--; return true; }
    SDL_PumpEvents();
    return Button();
}
