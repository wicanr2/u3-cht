/* plat_memory.c — Mac Memory Manager + Region shim (malloc 包裝)
 *
 * Handle = Ptr* (char**)。NewHandle 配一個 master pointer 區塊:
 *   handle 指向一個 Ptr,該 Ptr 指向實際資料。HLock/HUnlock 為 no-op。
 * Region 以一塊不透明 malloc 表示;剪裁邏輯一律忽略 (U3 多傳 nil)。 */
#include "mac_shim.h"

/* ---- Ptr ---- */
Ptr NewPtr(Size byteCount) {
    if (byteCount < 0) byteCount = 0;
    return (Ptr)malloc((size_t)byteCount);
}
Ptr NewPtrClear(Size byteCount) {
    if (byteCount < 0) byteCount = 0;
    return (Ptr)calloc(1, (size_t)byteCount);
}
void DisposePtr(Ptr p) { free(p); }

/* ---- Handle ----
 * 為了支援 GetHandleSize/SetHandleSize,在資料區塊前置 8 bytes 記錄大小。 */
static Handle handle_make(Size byteCount, int zero) {
    if (byteCount < 0) byteCount = 0;
    char *data = zero ? (char *)calloc(1, (size_t)byteCount + 8)
                      : (char *)malloc((size_t)byteCount + 8);
    *(Size *)data = byteCount;          /* 前 8 byte 存大小 (對齊用 Size=SInt32,佔 4,留 8 對齊) */
    Ptr realData = data + 8;
    Ptr *master = (Ptr *)malloc(sizeof(Ptr));
    *master = realData;
    return (Handle)master;
}
Handle NewHandle(Size byteCount)      { return handle_make(byteCount, 0); }
Handle NewHandleClear(Size byteCount) { return handle_make(byteCount, 1); }

void DisposeHandle(Handle h) {
    if (!h) return;
    char *data = (char *)(*h);
    if (data) free(data - 8);
    free(h);
}

Size GetHandleSize(Handle h) {
    if (!h || !*h) return 0;
    return *(Size *)((char *)(*h) - 8);
}
void SetHandleSize(Handle h, Size newSize) {
    if (!h || !*h) return;
    char *old = (char *)(*h) - 8;
    char *neu = (char *)realloc(old, (size_t)newSize + 8);
    *(Size *)neu = newSize;
    *h = (Ptr)(neu + 8);
}

void HLock(Handle h)   { (void)h; }
void HUnlock(Handle h) { (void)h; }

OSErr MemError(void) { return noErr; }
Size  FreeMem(void)      { return 0x40000000; }
Size  MaxMem(Size *grow) { if (grow) *grow = 0x40000000; return 0x40000000; }
Size  MaxApplZone(void)  { return 0x40000000; }

void BlockMove(const void *src, void *dst, Size count) {
    if (count > 0) memmove(dst, src, (size_t)count);
}
void BlockMoveData(const void *src, void *dst, Size count) {
    if (count > 0) memmove(dst, src, (size_t)count);
}

/* ---- Region (忽略剪裁;以不透明 1-byte 區塊表示) ---- */
RgnHandle NewRgn(void)               { return (RgnHandle)malloc(1); }
void      DisposeRgn(RgnHandle rgn)  { free(rgn); }
RgnHandle GetGrayRgn(void)           { static char g; return (RgnHandle)&g; }
void RectRgn(RgnHandle rgn, const Rect *r)                  { (void)rgn; (void)r; }
void SectRgn(RgnHandle a, RgnHandle b, RgnHandle out)       { (void)a; (void)b; (void)out; }
void UnionRgn(RgnHandle a, RgnHandle b, RgnHandle out)      { (void)a; (void)b; (void)out; }
void CopyRgn(RgnHandle src, RgnHandle dst)                  { (void)src; (void)dst; }
void GetClip(RgnHandle rgn)                                 { (void)rgn; }
void SetClip(RgnHandle rgn)                                 { (void)rgn; }
void ClipRect(const Rect *r)                                { (void)r; }
