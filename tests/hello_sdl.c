/* P0 smoke test：驗證 SDL2 工具鏈可初始化。
 * headless (Xvfb) 下也能跑：初始化 video → 建立離屏 surface → 結束。 */
#include <SDL.h>
#include <stdio.h>

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init 失敗: %s\n", SDL_GetError());
        return 1;
    }
    SDL_version v;
    SDL_GetVersion(&v);
    printf("SDL2 OK: %d.%d.%d\n", v.major, v.minor, v.patch);

    SDL_Window *win = SDL_CreateWindow("U3-CHT hello",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        320, 200, SDL_WINDOW_HIDDEN);
    if (!win) {
        fprintf(stderr, "建窗失敗: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("視窗建立成功\n");
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
