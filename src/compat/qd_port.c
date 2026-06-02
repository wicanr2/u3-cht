/* qd_port.c — GrafPort / GWorld / PixMap 生命週期與目前埠狀態 (SDL backed) */
#include "quickdraw.h"
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static CGrafPtr gCurrentPort = NULL;

/* 把 GrafPort 的 surface 同步到其 portBits / pm 代理欄位 */
static void sync_proxies(GrafPort *p) {
    SDL_Surface *s = p->surface;
    p->portBits.surface  = s;
    p->portBits.baseAddr = s ? (Ptr)s->pixels : NULL;
    p->portBits.rowBytes = s ? (SInt16)s->pitch : 0;
    p->portBits.bounds   = p->portRect;

    p->pm.surface  = s;
    p->pm.baseAddr = s ? (Ptr)s->pixels : NULL;
    p->pm.rowBytes = s ? (SInt16)s->pitch : 0;
    p->pm.bounds   = p->portRect;
    p->pm.pixelSize = 32;
    p->pmPtr = &p->pm;
}

static GrafPort *alloc_port(SDL_Surface *surf, Boolean owns) {
    GrafPort *p = (GrafPort *)calloc(1, sizeof(GrafPort));
    if (!p) return NULL;
    p->surface = surf;
    p->ownsSurface = owns;
    if (surf)
        SetRect(&p->portRect, 0, 0, (SInt16)surf->w, (SInt16)surf->h);
    p->fgColor.red = p->fgColor.green = p->fgColor.blue = 0;          /* 黑 */
    p->bgColor.red = p->bgColor.green = p->bgColor.blue = 0xFFFF;     /* 白 */
    p->penW = p->penH = 1;
    p->penModeVal = srcCopy;
    p->txSize = 12;
    sync_proxies(p);
    return p;
}

OSErr NewGWorld(GWorldPtr *out, SInt16 depth, const Rect *bounds,
                CTabHandle ctab, GDHandle dev, SInt32 flags) {
    (void)depth; (void)ctab; (void)dev; (void)flags;
    if (!out || !bounds) return -1;
    int w = bounds->right - bounds->left;
    int h = bounds->bottom - bounds->top;
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) { *out = NULL; return -108 /* memFullErr 近似 */; }
    /* 預設填黑不透明 */
    SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0, 0, 0, 255));
    GrafPort *p = alloc_port(surf, true);
    if (!p) { SDL_FreeSurface(surf); *out = NULL; return -108; }
    /* GWorld 的 portRect 沿用傳入 bounds 的原點 */
    p->portRect = *bounds;
    sync_proxies(p);
    *out = p;
    return noErr;
}

GWorldPtr NewGWorldFromSurface(SDL_Surface *surf, Boolean takeOwnership) {
    if (!surf) return NULL;
    /* 統一轉成 ARGB8888,確保 CopyBits 像素格式一致 */
    SDL_Surface *conv = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ARGB8888, 0);
    if (conv) {
        if (takeOwnership) SDL_FreeSurface(surf);
        return alloc_port(conv, true);
    }
    return alloc_port(surf, takeOwnership);
}

void DisposeGWorld(GWorldPtr gw) {
    if (!gw) return;
    if (gCurrentPort == gw) gCurrentPort = NULL;
    if (gw->ownsSurface && gw->surface) SDL_FreeSurface(gw->surface);
    free(gw);
}

/* 假主螢幕 GDevice:遊戲讀 (*(*mainDevice)->gdPMap)->pixelSize 判斷色深。
 * 直繪路徑已停用,給 32-bit 即可,僅需可安全解參。 */
static PixMap     gScreenPM   = { 0 };
static PixMapPtr  gScreenPMPtr = &gScreenPM;
static GDevice    gScreenGD   = { 0 };
static GDPtr      gScreenGDPtr = &gScreenGD;
static GDHandle   gScreenGDH  = &gScreenGDPtr;
static int        gScreenGDInit = 0;
static GDHandle screen_device(void) {
    if (!gScreenGDInit) {
        gScreenPM.pixelSize = 32;
        gScreenGD.gdPMap = &gScreenPMPtr;
        gScreenGDInit = 1;
    }
    return gScreenGDH;
}

/* 忽略 nil:nil port 非有效繪圖目標,設 nil 通常是 save/restore 還原了
 * 早期(視窗尚未建立時)捕捉到的 nil,會誤清掉主視窗埠 → 全畫面無渲染。 */
void SetGWorld(CGrafPtr port, GDHandle dev) { (void)dev; if (port) gCurrentPort = port; }
void GetGWorld(CGrafPtr *port, GDHandle *dev) {
    if (port) *port = gCurrentPort;
    if (dev)  *dev = screen_device();
}
void SetPort(GrafPtr port) { if (port) gCurrentPort = port; }
void GetPort(GrafPtr *port) { if (port) *port = gCurrentPort; }
CGrafPtr CurrentPort(void) { return gCurrentPort; }

PixMapHandle GetGWorldPixMap(GWorldPtr gw) {
    if (!gw) return NULL;
    sync_proxies(gw);
    return &gw->pmPtr;   /* PixMap** */
}

Boolean LockPixels(PixMapHandle pm) {
    if (!pm || !*pm) return false;
    if ((*pm)->surface && SDL_MUSTLOCK((*pm)->surface))
        SDL_LockSurface((*pm)->surface);
    return true;
}
void UnlockPixels(PixMapHandle pm) {
    if (pm && *pm && (*pm)->surface && SDL_MUSTLOCK((*pm)->surface))
        SDL_UnlockSurface((*pm)->surface);
}
Ptr GetPixBaseAddr(PixMapHandle pm) {
    if (!pm || !*pm) return NULL;
    return (*pm)->baseAddr;
}

const BitMap *LWPortCopyBits(CGrafPtr port) {
    if (!port) return NULL;
    sync_proxies(port);
    return &port->portBits;
}
