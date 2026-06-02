/* test_qd_blit.c — 驗證 compat 層 CopyBits 主路徑
 * 用 shim 載 tileset 進 GWorld → CopyBits tile 到螢幕 GWorld → 存 PNG。
 * 對齊 GetTileRectForIndex(blkSiz=16):tile 32x32, top=(idx%16)*32, left=(idx/16)*64。 */
#include "../src/compat/quickdraw.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#define TILE 32
static const char *TILESET = "assets/graphics/Standard-Tiles.png";
static const char *OUT_PNG = "qd_blit_out.png";

static Rect tile_rect(int index) {
    Rect r;
    r.top = (index % 16) * TILE;
    r.left = (index / 16) * (TILE * 2);
    r.bottom = r.top + TILE;
    r.right = r.left + TILE;
    return r;
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { fprintf(stderr, "IMG_Init: %s\n", IMG_GetError()); return 1; }

    SDL_Surface *tsurf = IMG_Load(TILESET);
    if (!tsurf) { fprintf(stderr, "載 tileset 失敗: %s\n", IMG_GetError()); return 1; }
    printf("tileset: %dx%d\n", tsurf->w, tsurf->h);

    GWorldPtr tiles = NewGWorldFromSurface(tsurf, true);

    Rect screenB; SetRect(&screenB, 0, 0, 12 * TILE, 8 * TILE);
    GWorldPtr screen = NULL;
    if (NewGWorld(&screen, 32, &screenB, nil, nil, 0) != noErr) {
        fprintf(stderr, "NewGWorld 失敗\n"); return 1;
    }

    /* 用 CopyBits 把前 96 個 tile 拼成 12x8 棋盤 */
    const BitMap *srcBM = LWPortCopyBits(tiles);
    const BitMap *dstBM = LWPortCopyBits(screen);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 12; col++) {
            int idx = (row * 12 + col) % 32;
            Rect sr = tile_rect(idx);
            Rect dr; SetRect(&dr, col * TILE, row * TILE, col * TILE + TILE, row * TILE + TILE);
            CopyBits(srcBM, dstBM, &sr, &dr, srcCopy, nil);
        }
    }

    /* 另測 scaled CopyBits:把 tile 0 放大成 64x64 疊在角落 */
    Rect sr0 = tile_rect(0);
    Rect big; SetRect(&big, 0, 0, 64, 64);
    CopyBits(srcBM, dstBM, &sr0, &big, srcCopy, nil);

    if (IMG_SavePNG(screen->surface, OUT_PNG) != 0) {
        fprintf(stderr, "存 PNG 失敗: %s\n", IMG_GetError()); return 1;
    }
    printf("截圖已存: %s\n", OUT_PNG);

    DisposeGWorld(tiles);
    DisposeGWorld(screen);
    IMG_Quit();
    SDL_Quit();
    printf("CopyBits 測試完成\n");
    return 0;
}
