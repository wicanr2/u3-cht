/* test_geometry.c — compat 幾何層單元測試 (無 SDL) */
#include "../src/compat/qd_geometry.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } \
    else { printf("ok  : %s\n", msg); } } while (0)

int main(void) {
    Rect r;
    SetRect(&r, 10, 20, 110, 120);  /* left,top,right,bottom */
    CHECK(r.left == 10 && r.top == 20 && r.right == 110 && r.bottom == 120, "SetRect 參數序");
    CHECK(RectWidth(&r) == 100 && RectHeight(&r) == 100, "Rect 寬高");

    OffsetRect(&r, 5, -5);
    CHECK(r.left == 15 && r.top == 15 && r.right == 115 && r.bottom == 115, "OffsetRect");

    InsetRect(&r, 5, 5);
    CHECK(r.left == 20 && r.top == 20 && r.right == 110 && r.bottom == 110, "InsetRect");

    Point in  = { 50, 50 };   /* v,h */
    Point edge = { 110, 50 }; /* 落在 bottom 邊界上 (不含) */
    Point out = { 200, 200 };
    CHECK(PtInRect(in, &r) == true,  "PtInRect 內部");
    CHECK(PtInRect(edge, &r) == false, "PtInRect 下邊界不含");
    CHECK(PtInRect(out, &r) == false, "PtInRect 外部");

    Rect a, b, sect;
    SetRect(&a, 0, 0, 100, 100);
    SetRect(&b, 50, 50, 150, 150);
    CHECK(SectRect(&a, &b, &sect) == true, "SectRect 有交集");
    CHECK(sect.left == 50 && sect.top == 50 && sect.right == 100 && sect.bottom == 100, "SectRect 結果");

    Rect c;
    SetRect(&c, 200, 200, 300, 300);
    CHECK(SectRect(&a, &c, &sect) == false, "SectRect 無交集");

    Rect uni;
    UnionRect(&a, &b, &uni);
    CHECK(uni.left == 0 && uni.top == 0 && uni.right == 150 && uni.bottom == 150, "UnionRect");

    Rect empty;
    SetRect(&empty, 10, 10, 10, 20);
    CHECK(EmptyRect(&empty) == true, "EmptyRect");
    CHECK(EmptyRect(&a) == false, "非空矩形");

    printf("\n%s (失敗 %d)\n", fails ? "測試失敗" : "全部通過", fails);
    return fails ? 1 : 0;
}
