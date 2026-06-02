/* test_strings_render.c — 端到端:.u3s 字串表 → compat 文字 shim → PNG
 * 驗證「讀表 → 取中文 → 繪字」全鏈路。 */
#include "../src/compat/quickdraw.h"
#include "../src/compat/qd_text.h"
#include "../src/text/strings.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

static const char *OUT_PNG = "strings_render_out.png";

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    IMG_Init(IMG_INIT_PNG);

    Strings_SetRoot("assets/strings");
    Strings_SetLang("zh-Hant");

    int nClasses = Strings_Count("Classes");
    int nRaces   = Strings_Count("Races");
    printf("Classes=%d Races=%d (語言 zh-Hant)\n", nClasses, nRaces);
    if (nClasses <= 0) { fprintf(stderr, "字串表載入失敗\n"); return 1; }

    Rect b; SetRect(&b, 0, 0, 440, 260);
    GWorldPtr screen = NULL;
    NewGWorld(&screen, 32, &b, nil, nil, 0);
    SetGWorld(screen, nil);
    RGBColor bg = { 0x0800, 0x0800, 0x1800 }; RGBBackColor(&bg); EraseRect(&b);

    RGBColor amber = { 0xFFFF, 0xC800, 0x5000 };
    RGBColor white = { 0xF000, 0xF000, 0xF000 };

    TextSize(20); RGBForeColor(&amber);
    U3_DrawUTF8("職業 (CLASS)", 16, 30, 22);
    TextSize(18); RGBForeColor(&white);
    for (int i = 0; i < nClasses; i++) {
        const char *s = Strings_GetUTF8("Classes", i);
        int col = i / 6, row = i % 6;
        U3_DrawUTF8(s, 24 + col * 110, 60 + row * 28, 18);
    }

    RGBForeColor(&amber);
    U3_DrawUTF8("種族 (RACE)", 250, 30, 22);
    RGBForeColor(&white);
    for (int i = 0; i < nRaces; i++) {
        const char *s = Strings_GetUTF8("Races", i);
        U3_DrawUTF8(s, 260, 60 + i * 28, 18);
    }

    /* 也示範相容 API 取 Pascal 字串 */
    unsigned char ps[256];
    Strings_GetPascal(ps, "Classes", 4);
    printf("Classes[4] (Pascal len=%d) = %.*s\n", ps[0], ps[0], ps + 1);

    if (IMG_SavePNG(screen->surface, OUT_PNG) != 0) { fprintf(stderr, "存 PNG 失敗\n"); return 1; }
    printf("截圖已存: %s\n", OUT_PNG);

    Strings_Shutdown();
    U3_TextShutdown();
    DisposeGWorld(screen);
    IMG_Quit(); SDL_Quit();
    printf("端到端測試完成\n");
    return 0;
}
