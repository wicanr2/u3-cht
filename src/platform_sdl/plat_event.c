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

/* --- 腳本輸入 --- */
static FILE *gScript = NULL;
static int   gScriptDone = 0;
static int   gScriptInit = 0;

static void script_init(void) {
    gScriptInit = 1;
    const char *p = getenv("U3_SCRIPT");
    if (p && *p) gScript = fopen(p, "rb");
}
/* 回傳下一個腳本字元 (Mac char code),無則 -1 */
static int script_next_key(void) {
    if (!gScriptInit) script_init();
    if (!gScript || gScriptDone) return -1;
    int c = fgetc(gScript);
    if (c == EOF) { gScriptDone = 1; fclose(gScript); gScript = NULL; return -1; }
    if (c == '\n') return 0x0D;   /* 換行 → Return */
    return c;
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

Boolean WaitNextEvent(short mask, EventRecord *evt, unsigned long sleep, RgnHandle rgn) {
    (void)mask; (void)sleep; (void)rgn;
    U3_PlatPresent();   /* frame 邊界:先把上一幀畫上螢幕 */

    if (evt) { evt->what = nullEvent; evt->message = 0; evt->modifiers = 0; }

    /* 腳本輸入優先 */
    int sk = script_next_key();
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
            evt->what = mouseDown;
            evt->where.h = (SInt16)ev.button.x;
            evt->where.v = (SInt16)ev.button.y;
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
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    if (pt) { pt->h = (SInt16)x; pt->v = (SInt16)y; }
}

Boolean Button(void) {
    return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) ? true : false;
}

Boolean StillDown(void) {
    SDL_PumpEvents();
    return Button();
}
