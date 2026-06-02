/* quickdraw.h — QuickDraw 繪圖埠 / GWorld / CopyBits 的 SDL2 shim
 *
 * 模型:
 *   GrafPort 包一塊 SDL_Surface (ARGB8888) 當像素緩衝。
 *   CGrafPtr / GWorldPtr / GrafPtr 都是 GrafPort*。
 *   有「目前埠」概念,由 SetGWorld/SetPort 設定。
 *   PixMapHandle = PixMap**,(**pm).rowBytes / GetPixBaseAddr 對齊原碼存取。
 */
#ifndef U3_COMPAT_QUICKDRAW_H
#define U3_COMPAT_QUICKDRAW_H

#include "mac_types.h"
#include "qd_geometry.h"

struct SDL_Surface;  /* 前向宣告,避免 header 強制依賴 SDL */

/* --- PixMap --- */
typedef struct PixMap {
    Ptr      baseAddr;            /* 指向像素 (= surface->pixels) */
    SInt16   rowBytes;            /* pitch (top bit 旗標已對齊原碼 0x7FFF mask) */
    Rect     bounds;
    SInt16   pixelSize;           /* 32 */
    struct SDL_Surface *surface;  /* shim 擴充 */
} PixMap;
typedef PixMap  *PixMapPtr;
typedef PixMapPtr *PixMapHandle;

/* --- BitMap (CopyBits 來源/目的;LWPortCopyBits 回傳) --- */
typedef struct BitMap {
    Ptr      baseAddr;
    SInt16   rowBytes;
    Rect     bounds;
    struct SDL_Surface *surface;  /* shim 擴充:實際像素來源 */
} BitMap;
typedef BitMap *BitMapPtr;

/* --- GrafPort --- */
typedef struct GrafPort {
    struct SDL_Surface *surface;  /* 像素緩衝 */
    BitMap   portBits;            /* portBits.surface == surface */
    Rect     portRect;

    /* 像素存取代理 */
    PixMap       pm;
    PixMapPtr    pmPtr;           /* = &pm,供 PixMapHandle 雙重解參 */

    /* 畫筆 / 色彩 / 文字 狀態 */
    Point    pen;
    RGBColor fgColor;
    RGBColor bgColor;
    SInt16   penModeVal;
    SInt16   penW, penH;
    SInt16   txFont, txSize, txMode;

    Boolean  ownsSurface;         /* DisposeGWorld 是否釋放 surface */
} GrafPort;

typedef GrafPort *CGrafPtr;
typedef GrafPort *GrafPtr;
typedef GrafPort *GWorldPtr;
typedef GrafPort *WindowPtr;
typedef GrafPort *WindowRef;

typedef void *RgnHandle;  /* U3 多以 nil 傳入;shim 忽略剪裁區 */
typedef void *CTabHandle;

/* GDevice:直繪螢幕路徑用 ((**mainDevice).gdPMap)。SDL 移植下此路徑停用,
 * 僅供型別解析 + 必要時指向視窗 framebuffer 的 PixMap。 */
typedef struct GDevice {
    PixMapHandle gdPMap;
    Rect         gdRect;
    SInt16       gdFlags;
} GDevice;
typedef GDevice  *GDPtr;
typedef GDPtr    *GDHandle;

/* ===== 埠 / GWorld 生命週期 ===== */
OSErr NewGWorld(GWorldPtr *out, SInt16 depth, const Rect *bounds,
                CTabHandle ctab, GDHandle dev, SInt32 flags);
/* shim 擴充:把既有 SDL_Surface 包成 GWorld (測試/載圖用) */
GWorldPtr NewGWorldFromSurface(struct SDL_Surface *surf, Boolean takeOwnership);
void  DisposeGWorld(GWorldPtr gw);

void  SetGWorld(CGrafPtr port, GDHandle dev);
void  GetGWorld(CGrafPtr *port, GDHandle *dev);
void  SetPort(GrafPtr port);
void  GetPort(GrafPtr *port);
CGrafPtr CurrentPort(void);

PixMapHandle GetGWorldPixMap(GWorldPtr gw);
Boolean LockPixels(PixMapHandle pm);
void    UnlockPixels(PixMapHandle pm);
Ptr     GetPixBaseAddr(PixMapHandle pm);

const BitMap *LWPortCopyBits(CGrafPtr port);

/* ===== 色彩 / 畫筆 ===== */
void RGBForeColor(const RGBColor *c);
void RGBBackColor(const RGBColor *c);
void ForeColor(SInt32 classicColor);   /* blackColor/whiteColor 等 */
void BackColor(SInt32 classicColor);
void PenMode(SInt16 mode);
void PenSize(SInt16 w, SInt16 h);
void MoveTo(SInt16 h, SInt16 v);
void LineTo(SInt16 h, SInt16 v);

/* classic 8 色常數 (ForeColor/BackColor 參數) */
enum {
    blackColor = 33, whiteColor = 30,
    redColor = 205, greenColor = 341, blueColor = 409,
    cyanColor = 273, magentaColor = 137, yellowColor = 69
};

/* ===== 填色 / 框線 ===== */
void PaintRect(const Rect *r);
void EraseRect(const Rect *r);
void FrameRect(const Rect *r);
void FillRect(const Rect *r, const void *pat);  /* pat 忽略,用 fgColor */

/* ===== 位元傳輸 ===== */
void CopyBits(const BitMap *src, const BitMap *dst,
              const Rect *srcRect, const Rect *dstRect,
              SInt16 mode, RgnHandle maskRgn);
void CopyMask(const BitMap *src, const BitMap *mask, const BitMap *dst,
              const Rect *srcRect, const Rect *maskRect, const Rect *dstRect);

#endif /* U3_COMPAT_QUICKDRAW_H */
