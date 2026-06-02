/* main.c — SDL2 主程式:建立視窗、初始化中文化、跑遊戲、present 到螢幕。
 *
 * 遊戲邏輯沿用上游 Ultima3_main();本檔提供顯示與生命週期。
 * 畫面流向:遊戲畫進 gMainWindow(NewCWindow 回傳的 GrafPort surface)
 *           → U3_PlatPresent() 上傳到真實 SDL_Window。
 *
 * game tester 支援 (環境變數):
 *   U3_LANG       語言 (預設 zh-Hant)
 *   U3_FONT       字型 TTF 路徑
 *   U3_SHOT_DIR   截圖輸出目錄
 *   U3_SHOT_EVERY 每 N 次 present 存一張截圖
 *   U3_MAX_FRAMES present 超過此數自動結束 (背景測試逾時)
 */
#include "mac_shim.h"
#include "../text/strings.h"
#include "../compat/qd_text.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

/* 崩潰時印 backtrace (除錯用;-g -rdynamic 時有符號名) */
static void crash_handler(int sig) {
    void *bt[64];
    int n = backtrace(bt, 64);
    fprintf(stderr, "\n[CRASH] signal %d, backtrace (%d frames):\n", sig, n);
    backtrace_symbols_fd(bt, n, 2);
    _exit(139);
}

extern int Ultima3_main(void);
extern WindowPtr gMainWindow;   /* 上游全域:主視窗 (= GrafPort*) */
extern CGrafPtr  mainPort;      /* 上游全域:上螢幕繪圖埠 (原 ForceUpdateMain 即 flush 此埠) */

static SDL_Window   *gWin;
static SDL_Renderer *gRen;
static SDL_Texture  *gTex;
static int gTexW, gTexH;

int gU3Done = 0;                /* SDL_QUIT 時設 1,供事件層查 */

static int   gShotEvery = 0, gPresentCount = 0, gMaxFrames = 0;
static char  gShotDir[512] = "";

void U3_PlatPresent(void) {
    if (!gRen) return;
    /* mainPort 是上螢幕埠 (ForceUpdateMain 原本 flush 它);回退 gMainWindow */
    CGrafPtr src = mainPort ? mainPort : gMainWindow;
    SDL_Surface *s = (src ? src->surface : NULL);
    if (!s) return;

    if (!gTex || gTexW != s->w || gTexH != s->h) {
        if (gTex) SDL_DestroyTexture(gTex);
        gTex = SDL_CreateTexture(gRen, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, s->w, s->h);
        gTexW = s->w; gTexH = s->h;
        if (gWin) SDL_SetWindowSize(gWin, s->w, s->h);
    }
    SDL_UpdateTexture(gTex, NULL, s->pixels, s->pitch);
    SDL_RenderClear(gRen);
    SDL_RenderCopy(gRen, gTex, NULL, NULL);
    SDL_RenderPresent(gRen);

    gPresentCount++;
    if (gShotEvery > 0 && gShotDir[0] && (gPresentCount % gShotEvery) == 0) {
        char path[640];
        snprintf(path, sizeof(path), "%s/frame_%05d.png", gShotDir, gPresentCount);
        IMG_SavePNG(s, path);
    }
    if (gMaxFrames > 0 && gPresentCount >= gMaxFrames) {
        fprintf(stderr, "[game_tester] 達 U3_MAX_FRAMES=%d,結束\n", gMaxFrames);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init 失敗: %s\n", SDL_GetError());
        return 1;
    }
    IMG_Init(IMG_INIT_PNG);

    const char *lang = getenv("U3_LANG");
    Strings_SetLang(lang && *lang ? lang : "zh-Hant");
    Strings_SetRoot("assets/strings");
    const char *font = getenv("U3_FONT");
    if (font && *font) U3_SetFontPath(font);

    const char *sd = getenv("U3_SHOT_DIR");
    if (sd && *sd) { snprintf(gShotDir, sizeof(gShotDir), "%s", sd); }
    const char *se = getenv("U3_SHOT_EVERY"); if (se) gShotEvery = atoi(se);
    const char *mf = getenv("U3_MAX_FRAMES"); if (mf) gMaxFrames = atoi(mf);

    gWin = SDL_CreateWindow("Ultima III:末日 (中文版)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!gWin) { fprintf(stderr, "建窗失敗: %s\n", SDL_GetError()); return 1; }
    gRen = SDL_CreateRenderer(gWin, -1, SDL_RENDERER_SOFTWARE);
    if (!gRen) { fprintf(stderr, "建 renderer 失敗: %s\n", SDL_GetError()); return 1; }
    SDL_SetRenderDrawColor(gRen, 0, 0, 0, 255);
    SDL_RenderClear(gRen);
    SDL_RenderPresent(gRen);

    int rc = Ultima3_main();

    if (gTex) SDL_DestroyTexture(gTex);
    if (gRen) SDL_DestroyRenderer(gRen);
    if (gWin) SDL_DestroyWindow(gWin);
    Strings_Shutdown();
    U3_TextShutdown();
    IMG_Quit();
    SDL_Quit();
    return rc;
}
