/* P1 垂直切片 PoC
 * 一次驗證三個最高風險假設:
 *   1. 繪圖換得掉 — 用 SDL2 載入 Standard-Tiles.png 並 blit tile region。
 *   2. plist 讀得到 — (此切片先用內嵌字串模擬;完整 plist 讀取於 text 模組。)
 *   3. 中文畫得出 — 用 SDL_ttf + Noto Sans CJK TC 渲染中文。
 *
 * 輸出:離屏 render → 存 poc_out.png,當決定性 pass/fail 截圖。
 * 執行 (容器內,headless):xvfb-run -a ./poc
 */
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>

#define WIN_W 640
#define WIN_H 400
#define TILE  32   /* blkSiz(16) * 2,對齊 GetTileRectForIndex */

static const char *TILESET = "assets/graphics/Standard-Tiles.png";
static const char *FONT    = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
static const char *OUT_PNG = "poc_out.png";

/* 對齊 ultima3 GetTileRectForIndex(blkSiz=16):tile 32x32,
 * top=(idx%16)*32, left=(idx/16)*64 (略過 swapped 變體)。 */
static SDL_Rect tile_src(int index) {
    SDL_Rect r;
    r.y = (index % 16) * TILE;
    r.x = (index / 16) * (TILE * 2);
    r.w = TILE;
    r.h = TILE;
    return r;
}

static void draw_text(SDL_Renderer *ren, TTF_Font *font,
                      const char *utf8, int x, int y, SDL_Color col) {
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, utf8, col);
    if (!s) { fprintf(stderr, "TTF render 失敗: %s\n", TTF_GetError()); return; }
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    SDL_Rect dst = { x, y, s->w, s->h };
    SDL_RenderCopy(ren, t, NULL, &dst);
    SDL_DestroyTexture(t);
    SDL_FreeSurface(s);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init 失敗: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "IMG_Init 失敗: %s\n", IMG_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init 失敗: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("U3-CHT PoC",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_HIDDEN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "建 renderer 失敗: %s\n", SDL_GetError()); return 1; }

    SDL_Texture *tiles = NULL;
    SDL_Surface *tsurf = IMG_Load(TILESET);
    if (!tsurf) {
        fprintf(stderr, "載入 tileset 失敗 (%s): %s\n", TILESET, IMG_GetError());
        return 1;
    }
    printf("tileset 載入成功: %dx%d\n", tsurf->w, tsurf->h);
    tiles = SDL_CreateTextureFromSurface(ren, tsurf);
    SDL_FreeSurface(tsurf);

    TTF_Font *font16 = TTF_OpenFont(FONT, 18);
    TTF_Font *font24 = TTF_OpenFont(FONT, 26);
    if (!font16 || !font24) {
        fprintf(stderr, "載入字型失敗 (%s): %s\n", FONT, TTF_GetError());
        return 1;
    }
    printf("字型載入成功: %s\n", FONT);

    /* --- 繪製 --- */
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);

    /* 左側:畫一塊 6x6 tile 拼圖,證明 tile region blit 可行 */
    int grid_indices[6][6];
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 6; c++)
            grid_indices[r][c] = (r * 6 + c) % 32; /* 取前 32 個 tile 展示 */
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 6; c++) {
            SDL_Rect src = tile_src(grid_indices[r][c]);
            SDL_Rect dst = { 16 + c * TILE, 64 + r * TILE, TILE, TILE };
            SDL_RenderCopy(ren, tiles, &src, &dst);
        }
    }

    /* 右側:中文文字,驗證 CJK 渲染 */
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color amber = { 255, 200, 80, 255 };
    int tx = 240;
    draw_text(ren, font24, "創世紀 III:末日", tx, 24, amber);
    draw_text(ren, font16, "Ultima III: Exodus 中文化", tx, 64, white);
    draw_text(ren, font16, "垂直切片 PoC — 三假設驗證", tx, 96, white);
    draw_text(ren, font16, "時間在你之下", tx, 140, amber);
    draw_text(ren, font16, "哎喲,好痛。", tx, 168, white);
    draw_text(ren, font16, "向北移動 / 上船 / 攻擊", tx, 196, white);
    draw_text(ren, font16, "繪圖 OK・字型 OK・中文 OK", tx, 240, amber);

    SDL_RenderPresent(ren);

    /* --- 存截圖 --- */
    SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(
        0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_ARGB8888,
                         shot->pixels, shot->pitch);
    if (IMG_SavePNG(shot, OUT_PNG) != 0) {
        fprintf(stderr, "存 PNG 失敗: %s\n", IMG_GetError());
        return 1;
    }
    printf("截圖已存: %s\n", OUT_PNG);
    SDL_FreeSurface(shot);

    TTF_CloseFont(font16);
    TTF_CloseFont(font24);
    SDL_DestroyTexture(tiles);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    printf("PoC 完成\n");
    return 0;
}
