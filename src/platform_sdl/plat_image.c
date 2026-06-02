/* plat_image.c — QuickTime GraphicsImporter → SDL_image 影像載入鏈 (真實作)
 *
 * 這是遊戲唯一的渲染素材來源,不可 stub。鏈路 (見 UltimaGraphics.c):
 *   CFURLGetFSRef(url, &fsr)            -> 把 path 放進 FSRef (plat_cf.c)
 *   FSGetCatalogInfo(&fsr,..,&fss,..)   -> 把 path 轉成 Pascal-string 放進 FSSpec.name
 *   GetGraphicsImporterForFile(&fss,&gi)-> IMG_Load(path) → SDL_Surface,包進 importer
 *   GraphicsImportGetBoundsRect(gi,&r)  -> r = {0,0,w,h}
 *   GraphicsImportSetGWorld(gi,world,?) -> 記住目標 GWorld
 *   GraphicsImportSetBoundsRect(gi,&r)  -> 記住繪製目標矩形
 *   GraphicsImportDraw(gi)              -> SDL_BlitScaled 到目標 GWorld.surface
 *   CloseComponent(gi)                  -> 釋放
 */
#include "mac_shim.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

/* ComponentInstance 在 mac_shim 為 (struct OpaqueComponentInstance*);
 * 我們把它當成自家 importer 結構指標。 */
typedef struct U3Importer {
    SDL_Surface *surface;   /* 載入的來源影像 (ARGB) */
    CGrafPtr     destWorld; /* 繪製目標 GWorld */
    Rect         destRect;  /* 繪製目標矩形 */
} U3Importer;

/* ---- FSSpec / FSRef 路徑橋接 ---- */

/* FSMakeFSSpec(vRef, dirID, pathStr, &fss):pathStr 是 Pascal string 路徑。
 * 直接複製進 fss.name。 */
OSErr FSMakeFSSpec(SInt16 vRefNum, SInt32 dirID, ConstStr255Param pathStr, FSSpec *fss) {
    (void)vRefNum; (void)dirID;
    if (!fss) return paramErr;
    fss->vRefNum = 0;
    fss->parID = 0;
    int len = pathStr ? pathStr[0] : 0;
    if (len > 63) len = 63;
    fss->name[0] = (unsigned char)len;
    if (pathStr) memcpy(fss->name + 1, pathStr + 1, len);
    return noErr;
}

/* FSGetCatalogInfo(&fsr, whichInfo, catInfo, name, &fss, parent):
 * 把 fsr.hidden 的 C 字串 path 以 Pascal string 寫進 fss.name。
 * 所有 U3 資產相對路徑 < 63 bytes。 */
OSErr FSGetCatalogInfo(const FSRef *fsr, UInt32 whichInfo, void *catInfo,
                       void *name, FSSpec *fss, FSRef *parent) {
    (void)whichInfo; (void)catInfo; (void)name; (void)parent;
    if (!fsr || !fss) return paramErr;
    const char *path = (const char *)fsr->hidden;
    int len = (int)strlen(path);
    if (len > 63) len = 63;
    fss->vRefNum = 0;
    fss->parID = 0;
    fss->name[0] = (unsigned char)len;
    memcpy(fss->name + 1, path, len);
    return noErr;
}

/* 從 FSSpec.name (Pascal string) 還原 C 路徑。 */
static void fss_to_cpath(const FSSpec *fss, char *out, size_t outsz) {
    int len = fss->name[0];
    if (len > (int)outsz - 1) len = (int)outsz - 1;
    memcpy(out, fss->name + 1, len);
    out[len] = '\0';
}

/* ---- GraphicsImporter ---- */

OSErr GetGraphicsImporterForFile(const FSSpec *fss, ComponentInstance *giOut) {
    if (!fss || !giOut) return paramErr;
    char path[512];
    fss_to_cpath(fss, path, sizeof(path));
    SDL_Surface *raw = IMG_Load(path);
    if (!raw) {
        /* 試 assets/ 前綴的相對路徑容錯 (FSGetCatalogInfo 已帶相對路徑) */
        fprintf(stderr, "[plat_image] IMG_Load 失敗: %s (%s)\n", path, IMG_GetError());
        *giOut = NULL;
        return fnfErr;
    }
    /* 轉成統一 ARGB8888,與 GWorld surface 格式一致,確保 BlitScaled 可用 */
    SDL_Surface *conv = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(raw);
    if (!conv) { *giOut = NULL; return fnfErr; }

    U3Importer *imp = (U3Importer *)calloc(1, sizeof(U3Importer));
    imp->surface = conv;
    imp->destWorld = NULL;
    imp->destRect.left = imp->destRect.top = 0;
    imp->destRect.right = conv->w;
    imp->destRect.bottom = conv->h;
    *giOut = (ComponentInstance)imp;
    return noErr;
}

OSErr GraphicsImportGetBoundsRect(ComponentInstance gi, Rect *r) {
    U3Importer *imp = (U3Importer *)gi;
    if (!imp || !imp->surface || !r) return paramErr;
    r->left = 0; r->top = 0;
    r->right = imp->surface->w;
    r->bottom = imp->surface->h;
    return noErr;
}

OSErr GraphicsImportSetGWorld(ComponentInstance gi, CGrafPtr world, void *gd) {
    (void)gd;
    U3Importer *imp = (U3Importer *)gi;
    if (!imp) return paramErr;
    imp->destWorld = world;
    return noErr;
}

OSErr GraphicsImportSetBoundsRect(ComponentInstance gi, const Rect *r) {
    U3Importer *imp = (U3Importer *)gi;
    if (!imp || !r) return paramErr;
    imp->destRect = *r;
    return noErr;
}

OSErr GraphicsImportDraw(ComponentInstance gi) {
    U3Importer *imp = (U3Importer *)gi;
    if (!imp || !imp->surface || !imp->destWorld) return paramErr;
    SDL_Surface *dst = imp->destWorld->surface;
    if (!dst) return paramErr;

    SDL_Rect dr;
    dr.x = imp->destRect.left;
    dr.y = imp->destRect.top;
    dr.w = imp->destRect.right - imp->destRect.left;
    dr.h = imp->destRect.bottom - imp->destRect.top;

    /* 來源不透明覆蓋:關閉 alpha blend 確保整塊覆寫 (tile sheet 本就不透明)。 */
    SDL_SetSurfaceBlendMode(imp->surface, SDL_BLENDMODE_NONE);
    if (dr.w == imp->surface->w && dr.h == imp->surface->h) {
        SDL_BlitSurface(imp->surface, NULL, dst, &dr);
    } else {
        SDL_BlitScaled(imp->surface, NULL, dst, &dr);
    }
    return noErr;
}

void CloseComponent(ComponentInstance gi) {
    U3Importer *imp = (U3Importer *)gi;
    if (!imp) return;
    if (imp->surface) SDL_FreeSurface(imp->surface);
    free(imp);
}

/* QuickTime importer 取得元件 (本鏈不用,stub)。 */
ComponentInstance OpenDefaultComponent(OSType a, OSType b) { (void)a; (void)b; return NULL; }
