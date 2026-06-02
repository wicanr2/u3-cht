/* qd_draw.c — CopyBits / CopyMask / 填色 / 畫筆 / 色彩 (SDL backed) */
#include "quickdraw.h"
#include <SDL.h>

/* 16-bit 通道 → 8-bit */
static Uint8 c8(UInt16 c) { return (Uint8)(c >> 8); }

/* 把 bitmap 座標空間的 Rect 轉成 surface 內 SDL_Rect (扣掉 bounds 原點) */
static SDL_Rect to_surf_rect(const BitMap *bm, const Rect *r) {
    SDL_Rect s;
    s.x = r->left - bm->bounds.left;
    s.y = r->top  - bm->bounds.top;
    s.w = r->right - r->left;
    s.h = r->bottom - r->top;
    return s;
}

void CopyBits(const BitMap *src, const BitMap *dst,
              const Rect *srcRect, const Rect *dstRect,
              SInt16 mode, RgnHandle maskRgn) {
    (void)maskRgn;
    if (!src || !dst || !src->surface || !dst->surface) return;

    SDL_Rect s = to_surf_rect(src, srcRect);
    SDL_Rect d = to_surf_rect(dst, dstRect);

    SDL_BlendMode bm = SDL_BLENDMODE_NONE;
    if (mode == blend || mode == transparent) bm = SDL_BLENDMODE_BLEND;
    SDL_SetSurfaceBlendMode(src->surface, bm);

    if (s.w == d.w && s.h == d.h)
        SDL_BlitSurface(src->surface, &s, dst->surface, &d);
    else
        SDL_BlitScaled(src->surface, &s, dst->surface, &d);
}

/* 讀/寫 ARGB8888 像素 (surface 須已鎖) */
static Uint32 getpx(SDL_Surface *s, int x, int y) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return 0;
    Uint8 *p = (Uint8 *)s->pixels + y * s->pitch + x * 4;
    return *(Uint32 *)p;
}
static void putpx(SDL_Surface *s, int x, int y, Uint32 v) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
    Uint8 *p = (Uint8 *)s->pixels + y * s->pitch + x * 4;
    *(Uint32 *)p = v;
}

/* CopyMask:mask 深色 (亮度 < 128) 視為不透明 → 複製 src;否則保留 dst。
 * 支援不等尺寸 (nearest 取樣)。 */
void CopyMask(const BitMap *src, const BitMap *mask, const BitMap *dst,
              const Rect *srcRect, const Rect *maskRect, const Rect *dstRect) {
    if (!src || !mask || !dst) return;
    SDL_Surface *ss = src->surface, *ms = mask->surface, *ds = dst->surface;
    if (!ss || !ms || !ds) return;

    SDL_Rect s = to_surf_rect(src,  srcRect);
    SDL_Rect m = to_surf_rect(mask, maskRect);
    SDL_Rect d = to_surf_rect(dst,  dstRect);
    if (d.w <= 0 || d.h <= 0) return;

    if (SDL_MUSTLOCK(ss)) SDL_LockSurface(ss);
    if (SDL_MUSTLOCK(ms)) SDL_LockSurface(ms);
    if (SDL_MUSTLOCK(ds)) SDL_LockSurface(ds);

    SDL_PixelFormat *sf = ss->format, *mf = ms->format;

    for (int dy = 0; dy < d.h; dy++) {
        for (int dx = 0; dx < d.w; dx++) {
            int sx = s.x + (d.w ? dx * s.w / d.w : 0);
            int sy = s.y + (d.h ? dy * s.h / d.h : 0);
            int mx = m.x + (d.w ? dx * m.w / d.w : 0);
            int my = m.y + (d.h ? dy * m.h / d.h : 0);

            Uint8 mr, mg, mb;
            SDL_GetRGB(getpx(ms, mx, my), mf, &mr, &mg, &mb);
            int lum = (mr * 30 + mg * 59 + mb * 11) / 100;
            if (lum < 128) {  /* 不透明 → 複製來源 */
                Uint8 r, g, b, a;
                SDL_GetRGBA(getpx(ss, sx, sy), sf, &r, &g, &b, &a);
                putpx(ds, d.x + dx, d.y + dy,
                      SDL_MapRGBA(ds->format, r, g, b, a));
            }
        }
    }

    if (SDL_MUSTLOCK(ds)) SDL_UnlockSurface(ds);
    if (SDL_MUSTLOCK(ms)) SDL_UnlockSurface(ms);
    if (SDL_MUSTLOCK(ss)) SDL_UnlockSurface(ss);
}

/* ===== 色彩 / 畫筆 (作用於目前埠) ===== */
static void classic_to_rgb(SInt32 c, RGBColor *out) {
    UInt16 hi = 0xFFFF, lo = 0;
    switch (c) {
        case whiteColor:   out->red=hi; out->green=hi; out->blue=hi; break;
        case redColor:     out->red=hi; out->green=lo; out->blue=lo; break;
        case greenColor:   out->red=lo; out->green=hi; out->blue=lo; break;
        case blueColor:    out->red=lo; out->green=lo; out->blue=hi; break;
        case cyanColor:    out->red=lo; out->green=hi; out->blue=hi; break;
        case magentaColor: out->red=hi; out->green=lo; out->blue=hi; break;
        case yellowColor:  out->red=hi; out->green=hi; out->blue=lo; break;
        case blackColor:
        default:           out->red=lo; out->green=lo; out->blue=lo; break;
    }
}

void RGBForeColor(const RGBColor *c) { CGrafPtr p = CurrentPort(); if (p && c) p->fgColor = *c; }
void RGBBackColor(const RGBColor *c) { CGrafPtr p = CurrentPort(); if (p && c) p->bgColor = *c; }
void ForeColor(SInt32 c) { CGrafPtr p = CurrentPort(); if (p) classic_to_rgb(c, &p->fgColor); }
void BackColor(SInt32 c) { CGrafPtr p = CurrentPort(); if (p) classic_to_rgb(c, &p->bgColor); }
void PenMode(SInt16 mode) { CGrafPtr p = CurrentPort(); if (p) p->penModeVal = mode; }
void PenSize(SInt16 w, SInt16 h) { CGrafPtr p = CurrentPort(); if (p) { p->penW=w; p->penH=h; } }
void MoveTo(SInt16 h, SInt16 v) { CGrafPtr p = CurrentPort(); if (p) { p->pen.h=h; p->pen.v=v; } }

void LineTo(SInt16 h, SInt16 v) {
    CGrafPtr p = CurrentPort();
    if (!p || !p->surface) return;
    SDL_Surface *s = p->surface;
    Uint32 col = SDL_MapRGBA(s->format, c8(p->fgColor.red), c8(p->fgColor.green), c8(p->fgColor.blue), 255);
    int x0 = p->pen.h - p->portRect.left, y0 = p->pen.v - p->portRect.top;
    int x1 = h - p->portRect.left,        y1 = v - p->portRect.top;
    /* Bresenham */
    int dx = x1>x0?x1-x0:x0-x1, sx = x0<x1?1:-1;
    int dy = y1>y0?-(y1-y0):-(y0-y1), sy = y0<y1?1:-1;
    int err = dx + dy;
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    for (;;) {
        putpx(s, x0, y0, col);
        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    p->pen.h = h; p->pen.v = v;
}

static void fill_rect_color(const Rect *r, RGBColor col) {
    CGrafPtr p = CurrentPort();
    if (!p || !p->surface || !r) return;
    SDL_Rect d = { r->left - p->portRect.left, r->top - p->portRect.top,
                   r->right - r->left, r->bottom - r->top };
    SDL_FillRect(p->surface, &d,
        SDL_MapRGBA(p->surface->format, c8(col.red), c8(col.green), c8(col.blue), 255));
}

void PaintRect(const Rect *r) { CGrafPtr p = CurrentPort(); if (p) fill_rect_color(r, p->fgColor); }
void EraseRect(const Rect *r) { CGrafPtr p = CurrentPort(); if (p) fill_rect_color(r, p->bgColor); }
void FillRect(const Rect *r, const void *pat) { (void)pat; PaintRect(r); }

void FrameRect(const Rect *r) {
    CGrafPtr p = CurrentPort();
    if (!p || !p->surface || !r) return;
    Rect top    = { r->top,        r->left, r->top+1,    r->right };
    Rect bottom = { r->bottom-1,   r->left, r->bottom,   r->right };
    Rect left   = { r->top,        r->left, r->bottom,   r->left+1 };
    Rect right  = { r->top,        r->right-1, r->bottom, r->right };
    fill_rect_color(&top, p->fgColor);
    fill_rect_color(&bottom, p->fgColor);
    fill_rect_color(&left, p->fgColor);
    fill_rect_color(&right, p->fgColor);
}
