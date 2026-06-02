/* plat_toolbox.c — Mac Toolbox 雜項工具 (時間 / 亂數 / 字串數字轉換 / 比較 / Gestalt)
 *
 * 多為純函式或最小 stub;時間類接 SDL_GetTicks。 */
#include "mac_shim.h"
#include <SDL.h>
#include <time.h>
#include <stdio.h>

/* ===== 時間 ===== */
/* TickCount:每秒 60 ticks。 */
UInt32 TickCount(void) {
    return (UInt32)((unsigned long long)SDL_GetTicks() * 60ULL / 1000ULL);
}
/* GetDateTime(*secs):Mac epoch 1904,但 game 只用其相對性 → 給 unix time。 */
void GetDateTime(UInt32 *secs) {
    if (secs) *secs = (UInt32)time(NULL);
}
/* Delay(numTicks, *finalTicks):睡 numTicks/60 秒。 */
void Delay(UInt32 numTicks, UInt32 *finalTicks) {
    SDL_Delay((Uint32)(numTicks * 1000UL / 60UL));
    if (finalTicks) *finalTicks = TickCount();
}
/* ThreadSleepTicks (CocoaBridge.h):同 Delay。 */
void ThreadSleepTicks(int numTicks) {
    if (numTicks > 0) SDL_Delay((Uint32)(numTicks * 1000 / 60));
}

/* ===== 亂數 (LCG,回 -32768..32767,對齊 Mac Random()) ===== */
static UInt32 g_randState = 1;
SInt16 Random(void) {
    g_randState = g_randState * 1103515245u + 12345u;
    /* 取高位元做 16-bit 有號值 */
    return (SInt16)((g_randState >> 16) & 0xFFFF);
}

/* ===== 字串 <-> 數字 (Pascal string) ===== */
void NumToString(SInt32 num, StringPtr s) {
    if (!s) return;
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%ld", (long)num);
    if (n < 0) n = 0;
    if (n > 255) n = 255;
    s[0] = (unsigned char)n;
    memcpy(s + 1, tmp, n);
}
void StringToNum(ConstStr255Param s, SInt32 *num) {
    if (!num) return;
    if (!s) { *num = 0; return; }
    int len = s[0];
    char tmp[256];
    if (len > 255) len = 255;
    memcpy(tmp, s + 1, len);
    tmp[len] = '\0';
    *num = (SInt32)strtol(tmp, NULL, 10);
}

/* ===== Pascal 字串比較 ===== */
/* EqualString(a, b, caseSens, diacSens):回 Boolean。 */
Boolean EqualString(ConstStr255Param a, ConstStr255Param b, Boolean cs, Boolean ds) {
    (void)ds;
    if (!a || !b) return a == b;
    int la = a[0], lb = b[0];
    if (la != lb) return false;
    if (cs) return memcmp(a + 1, b + 1, la) == 0;
    for (int i = 1; i <= la; i++) {
        unsigned char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return false;
    }
    return true;
}

/* ===== Point 比較 ===== */
Boolean EqualPt(Point a, Point b) { return a.h == b.h && a.v == b.v; }

/* ===== 字組拆解 ===== */
SInt16 LoWord(SInt32 x) { return (SInt16)(x & 0xFFFF); }
SInt16 HiWord(SInt32 x) { return (SInt16)((x >> 16) & 0xFFFF); }

/* ===== Gestalt / 系統版本 ===== */
OSErr Gestalt(OSType selector, SInt32 *response) {
    if (response) {
        switch (selector) {
            case gestaltQuickdrawVersion: *response = 0x0250; break; /* 假 8-bit+ QD */
            case gestalt32BitQD:          *response = 1;      break;
            default:                      *response = 0;      break;
        }
    }
    return noErr;
}
/* GetSystemVersion (CocoaBridge.h):回 Boolean,給 macOS 10.x 風格版本。 */
Boolean GetSystemVersion(unsigned *major, unsigned *minor, unsigned *bugfix) {
    if (major)  *major = 10;
    if (minor)  *minor = 15;
    if (bugfix) *bugfix = 0;
    return true;
}

/* ===== 程式結束 / 系統提示音 ===== */
void ExitToShell(void) { exit(0); }
void SysBeep(SInt16 duration) { (void)duration; }

/* ===== GetIndString:取索引字串資源。原碼多已改走字串表,殘留呼叫回空 Pascal 字串。
 *        (中文化主要文字走 GetPascalStringFromArrayByIndex,見 cf_bridge.c) ===== */
void GetIndString(StringPtr theString, SInt16 strListID, SInt16 index) {
    (void)strListID; (void)index;
    if (theString) theString[0] = 0;
}

/* ===== NewModalFilterUPP:對話框過濾器 UPP,直接回傳 proc 指標 ===== */
ModalFilterUPP NewModalFilterUPP(void *proc) { return (ModalFilterUPP)proc; }

/* ===== SndSoundManagerVersion (mac_shim.h 已宣告,回 majorRev=3) ===== */
NumVersion SndSoundManagerVersion(void) {
    NumVersion v = {0};
    v.majorRev = 3;
    return v;
}
