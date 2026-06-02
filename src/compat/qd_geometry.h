/* qd_geometry.h — QuickDraw 純幾何運算 shim (無 SDL 依賴)
 * 對齊 Classic QuickDraw 語意。 */
#ifndef U3_COMPAT_QD_GEOMETRY_H
#define U3_COMPAT_QD_GEOMETRY_H

#include "mac_types.h"

/* 設定矩形 (注意 QuickDraw 參數序:left, top, right, bottom) */
void SetRect(Rect *r, SInt16 left, SInt16 top, SInt16 right, SInt16 bottom);

/* 平移 */
void OffsetRect(Rect *r, SInt16 dh, SInt16 dv);

/* 內縮 (負值為外擴) */
void InsetRect(Rect *r, SInt16 dh, SInt16 dv);

/* 點是否落在矩形內 (右/下邊界不含,對齊 QuickDraw) */
Boolean PtInRect(Point pt, const Rect *r);

/* 交集:寫入 dst,回傳是否非空 */
Boolean SectRect(const Rect *src1, const Rect *src2, Rect *dst);

/* 聯集 */
void UnionRect(const Rect *src1, const Rect *src2, Rect *dst);

/* 空矩形判斷 (right<=left 或 bottom<=top) */
Boolean EmptyRect(const Rect *r);

/* 兩矩形相等 */
Boolean EqualRect(const Rect *a, const Rect *b);

/* 寬 / 高 helper (非 QuickDraw 原生,移植便利用) */
SInt16 RectWidth(const Rect *r);
SInt16 RectHeight(const Rect *r);

#endif /* U3_COMPAT_QD_GEOMETRY_H */
