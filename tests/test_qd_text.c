/* test_qd_text.c — 驗證 compat 文字 shim:Pascal/UTF-8 中文繪入 GWorld */
#include "../src/compat/quickdraw.h"
#include "../src/compat/qd_text.h"
#include <SDL.h>
#include <SDL_image.h>
#include <string.h>
#include <stdio.h>

static const char *OUT_PNG = "qd_text_out.png";

/* 建 Pascal 字串 (UTF-8 bytes) */
static void mkpstr(unsigned char *dst, const char *utf8) {
    size_t n = strlen(utf8);
    if (n > 255) n = 255;
    dst[0] = (unsigned char)n;
    memcpy(dst + 1, utf8, n);
}

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    IMG_Init(IMG_INIT_PNG);

    Rect b; SetRect(&b, 0, 0, 420, 200);
    GWorldPtr screen = NULL;
    if (NewGWorld(&screen, 32, &b, nil, nil, 0) != noErr) { fprintf(stderr, "NewGWorld 失敗\n"); return 1; }
    SetGWorld(screen, nil);

    /* 深藍底 */
    RGBColor bg = { 0x1000, 0x1800, 0x3000 };
    RGBBackColor(&bg);
    EraseRect(&b);

    RGBColor amber = { 0xFFFF, 0xC800, 0x5000 };
    RGBColor white = { 0xFFFF, 0xFFFF, 0xFFFF };

    unsigned char ps[256];

    TextSize(26);
    RGBForeColor(&amber);
    mkpstr(ps, "創世紀 III:末日");
    MoveTo(20, 40);                          /* pen.v = baseline */
    UDrawThemePascalString(ps, kThemeCurrentPortFont);

    TextSize(18);
    RGBForeColor(&white);
    mkpstr(ps, "汝欲扮演何種角色?");
    MoveTo(20, 80);
    UDrawThemePascalString(ps, kThemeCurrentPortFont);

    mkpstr(ps, "鬥士  巫師  竊賊  牧師");
    MoveTo(20, 110);
    UDrawThemePascalString(ps, kThemeCurrentPortFont);

    /* 測 DrawString 連續推進畫筆 */
    MoveTo(20, 145);
    mkpstr(ps, "向北 ");  DrawString(ps);
    RGBForeColor(&amber);
    mkpstr(ps, "向南 ");  DrawString(ps);
    RGBForeColor(&white);
    mkpstr(ps, "向東 向西"); DrawString(ps);

    /* 測寬度量測 */
    mkpstr(ps, "時間在你之下");
    SInt16 w = UThemePascalStringWidth(ps, kThemeCurrentPortFont);
    printf("「時間在你之下」寬度 = %d px\n", w);
    MoveTo(20, 178);
    RGBForeColor(&amber);
    UDrawThemePascalString(ps, kThemeCurrentPortFont);

    if (IMG_SavePNG(screen->surface, OUT_PNG) != 0) { fprintf(stderr, "存 PNG 失敗\n"); return 1; }
    printf("截圖已存: %s\n", OUT_PNG);

    U3_TextShutdown();
    DisposeGWorld(screen);
    IMG_Quit();
    SDL_Quit();
    printf("文字 shim 測試完成\n");
    return 0;
}
