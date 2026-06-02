/* qd_geometry.c — QuickDraw 純幾何運算實作 */
#include "qd_geometry.h"

void SetRect(Rect *r, SInt16 left, SInt16 top, SInt16 right, SInt16 bottom) {
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
}

void OffsetRect(Rect *r, SInt16 dh, SInt16 dv) {
    r->left   += dh;
    r->right  += dh;
    r->top    += dv;
    r->bottom += dv;
}

void InsetRect(Rect *r, SInt16 dh, SInt16 dv) {
    r->left   += dh;
    r->right  -= dh;
    r->top    += dv;
    r->bottom -= dv;
}

Boolean PtInRect(Point pt, const Rect *r) {
    return (pt.h >= r->left && pt.h < r->right &&
            pt.v >= r->top  && pt.v < r->bottom) ? true : false;
}

Boolean SectRect(const Rect *src1, const Rect *src2, Rect *dst) {
    SInt16 top    = src1->top    > src2->top    ? src1->top    : src2->top;
    SInt16 left   = src1->left   > src2->left   ? src1->left   : src2->left;
    SInt16 bottom = src1->bottom < src2->bottom ? src1->bottom : src2->bottom;
    SInt16 right  = src1->right  < src2->right  ? src1->right  : src2->right;

    if (top < bottom && left < right) {
        dst->top = top; dst->left = left;
        dst->bottom = bottom; dst->right = right;
        return true;
    }
    /* 空交集:QuickDraw 會清為 0 矩形 */
    dst->top = dst->left = dst->bottom = dst->right = 0;
    return false;
}

void UnionRect(const Rect *src1, const Rect *src2, Rect *dst) {
    if (EmptyRect(src1)) { *dst = *src2; return; }
    if (EmptyRect(src2)) { *dst = *src1; return; }
    dst->top    = src1->top    < src2->top    ? src1->top    : src2->top;
    dst->left   = src1->left   < src2->left   ? src1->left   : src2->left;
    dst->bottom = src1->bottom > src2->bottom ? src1->bottom : src2->bottom;
    dst->right  = src1->right  > src2->right  ? src1->right  : src2->right;
}

Boolean EmptyRect(const Rect *r) {
    return (r->right <= r->left || r->bottom <= r->top) ? true : false;
}

Boolean EqualRect(const Rect *a, const Rect *b) {
    return (a->top == b->top && a->left == b->left &&
            a->bottom == b->bottom && a->right == b->right) ? true : false;
}

SInt16 RectWidth(const Rect *r)  { return (SInt16)(r->right - r->left); }
SInt16 RectHeight(const Rect *r) { return (SInt16)(r->bottom - r->top); }
