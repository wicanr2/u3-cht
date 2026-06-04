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
#include <signal.h>
#ifndef _WIN32
#include <unistd.h>
#include <execinfo.h>   /* backtrace:POSIX 限定 (Windows 無) */
#endif

/* 崩潰時印 backtrace (除錯用;-g -rdynamic 時有符號名)。Windows 無 backtrace。 */
static void crash_handler(int sig) {
#ifndef _WIN32
    void *bt[64];
    int n = backtrace(bt, 64);
    fprintf(stderr, "\n[CRASH] signal %d, backtrace (%d frames):\n", sig, n);
    backtrace_symbols_fd(bt, n, 2);
    _exit(139);
#else
    fprintf(stderr, "\n[CRASH] signal %d\n", sig);
    exit(139);
#endif
}

extern int Ultima3_main(void);
extern WindowPtr gMainWindow;   /* 上游全域:主視窗 (= GrafPort*) */
extern CGrafPtr  mainPort;      /* 上游全域:上螢幕繪圖埠 (原 ForceUpdateMain 即 flush 此埠) */
extern short          gUpdateWhere;   /* 畫面狀態:5=主選單 6=Organize 3=世界/地圖 */
extern unsigned char  Party[];        /* Party[3]:0=外界 1=地城 2=城鎮 3=城堡 */

/* 場景追蹤:U3_DBG_SCENE 設定時,於畫面狀態變化印 grep 用標記,供 smoke 驗證
 * 全流程 (主選單→選項→編組→世界→城堡)。party3 區分外界/城鎮/城堡。 */
static int gSceneDbg = -1, gLastWhere = -999, gLastParty3 = -999;
void U3_SceneTrace(void) {
    if (gSceneDbg < 0) gSceneDbg = getenv("U3_DBG_SCENE") ? 1 : 0;
    if (!gSceneDbg) return;
    int w = (int)gUpdateWhere, p3 = (int)Party[3];
    if (w != gLastWhere || p3 != gLastParty3) {
        const char *name = (w == 5) ? "menu" : (w == 6) ? "organize" :
            (w == 3 && p3 == 0) ? "world" : (w == 3 && p3 == 2) ? "town" :
            (w == 3 && p3 == 3) ? "castle" : (w == 3 && p3 == 1) ? "dungeon" : "other";
        fprintf(stderr, "[SCENE] where=%d party3=%d %s\n", w, p3, name);
        gLastWhere = w; gLastParty3 = p3;
    }
}

static SDL_Window   *gWin;
static SDL_Renderer *gRen;
static SDL_Texture  *gTex;
static int gTexW, gTexH;

int gU3Done = 0;                /* SDL_QUIT 時設 1,供事件層查 */

static int   gShotEvery = 0, gPresentCount = 0, gMaxFrames = 0;
static char  gShotDir[512] = "";

/* 影格率上限 = 遊戲速度節流。原 Mac 主迴圈每次輪詢都 present,快 GPU 上達上千 fps
 * → 回合/動畫快約 16 倍。把 present 節流到固定 fps,使「count=60 次輪詢 ≈ N 秒/回合」。
 * 速度檔位 (遊戲內「調整選項」可調,存 GameSpeed 偏好):
 *   慢=20fps(~3s/回合,最接近 8086 DOS 慢板,預設)、中=30fps(~2s)、快=60fps(~1s)。
 * U3_FPS_CAP 環境變數覆寫一切 (=0 關閉節流,供無頭測試)。 */
#define U3_SPEED_DEFAULT 20
static Uint32 gFrameMinMs = 1000 / U3_SPEED_DEFAULT;
static Uint32 gLastPresentMs = 0;

extern long U3_PrefGetLong(const char *k, long dflt);
extern void U3_PrefSetLong(const char *k, long v);

/* 目前速度 fps (0 = 不節流)。供選項對話顯示/循環。 */
int U3_GetGameSpeedFps(void) {
    return gFrameMinMs > 0 ? (int)(1000 / gFrameMinMs) : 0;
}
/* 設定速度 fps 並持久化 (選項對話呼叫)。 */
void U3_SetGameSpeedFps(int fps) {
    gFrameMinMs = (fps > 0) ? (Uint32)(1000 / fps) : 0;
    U3_PrefSetLong("GameSpeed", fps);
}

/* 視窗像素座標 → 畫布座標。
 * present 用 RenderCopy(NULL,NULL) 把 gTexW×gTexH 畫布等比拉伸填滿整個視窗,
 * 放大視窗後滑鼠座標需依「畫布/視窗」比例換回畫布座標,否則落點偏移
 * → 修正「放大後滑鼠無法點擊」。未放大時比例為 1,等同原行為 (不影響腳本點擊)。*/
void U3_MapMouse(int *x, int *y) {
    if (!gWin || gTexW <= 0 || gTexH <= 0) return;
    int ww = 0, wh = 0;
    SDL_GetWindowSize(gWin, &ww, &wh);
    if (ww <= 0 || wh <= 0) return;
    if (x) *x = (int)((long)(*x) * gTexW / ww);
    if (y) *y = (int)((long)(*y) * gTexH / wh);
}

void U3_PlatPresent(void) {
    if (!gRen) return;
    U3_SceneTrace();   /* 場景變化標記 (U3_DBG_SCENE) */
    /* mainPort 是上螢幕埠 (ForceUpdateMain 原本 flush 它);回退 gMainWindow */
    CGrafPtr src = mainPort ? mainPort : gMainWindow;
    SDL_Surface *s = (src ? src->surface : NULL);
    if (!s) return;

    if (!gTex || gTexW != s->w || gTexH != s->h) {
        if (gTex) SDL_DestroyTexture(gTex);
        gTex = SDL_CreateTexture(gRen, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, s->w, s->h);
        gTexW = s->w; gTexH = s->h;
        /* 畫布尺寸變動時讓視窗貼齊 (沿用原行為:不同畫面用不同尺寸的上螢幕埠)。
         * 使用者之後手動放大視窗時不再回彈,RenderCopy 等比填滿,
         * 滑鼠座標由 U3_MapMouse 依比例換算。*/
        if (gWin) SDL_SetWindowSize(gWin, s->w, s->h);
    }
    SDL_UpdateTexture(gTex, NULL, s->pixels, s->pitch);
    SDL_RenderClear(gRen);
    SDL_RenderCopy(gRen, gTex, NULL, NULL);
    SDL_RenderPresent(gRen);

    /* 影格率節流:維持每格至少 gFrameMinMs,使遊戲回合/動畫回到原速。*/
    if (gFrameMinMs > 0) {
        Uint32 now = SDL_GetTicks();
        Uint32 dt  = now - gLastPresentMs;
        if (gLastPresentMs != 0 && dt < gFrameMinMs) SDL_Delay(gFrameMinMs - dt);
        gLastPresentMs = SDL_GetTicks();
    }

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
    const char *fc = getenv("U3_FPS_CAP");
    if (fc) {   /* 環境變數覆寫 (測試用;=0 關閉節流) */
        int f = atoi(fc);
        gFrameMinMs = (f > 0) ? (Uint32)(1000 / f) : 0;
    } else {    /* 否則用 GameSpeed 偏好 (預設慢 20fps) */
        long fps = U3_PrefGetLong("GameSpeed", U3_SPEED_DEFAULT);
        gFrameMinMs = (fps > 0) ? (Uint32)(1000 / fps) : 0;
    }

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
