/* mac_types.h — Classic Mac / QuickDraw 基本型別 shim
 *
 * 目的:讓 beastie/ultima3 的遊戲邏輯與繪圖碼幾乎不改即可在 Linux/SDL2 編譯。
 * 只重建 Ultima III 實際用到的子集,不追求完整 Carbon 相容。
 *
 * 座標系沿用 Mac QuickDraw:Rect = {top, left, bottom, right};Point = {v, h}。
 */
#ifndef U3_COMPAT_MAC_TYPES_H
#define U3_COMPAT_MAC_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* --- 基本純量 --- */
typedef unsigned char  Boolean;
typedef int8_t         SInt8;
typedef uint8_t        UInt8;
typedef int16_t        SInt16;
typedef uint16_t       UInt16;
typedef int32_t        SInt32;
typedef uint32_t       UInt32;
typedef int32_t        OSErr;
typedef int32_t        OSStatus;
typedef char           SignedByte;
typedef unsigned char  Byte;

#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif

#ifndef nil
#define nil  NULL
#endif

/* noErr 等常見回傳 */
enum { noErr = 0 };

/* --- Pascal 字串 (長度前綴, byte[0] = 長度) --- */
typedef unsigned char  Str255[256];
typedef unsigned char  Str63[64];
typedef unsigned char  Str32[33];
typedef unsigned char  Str31[32];
typedef unsigned char *StringPtr;
typedef const unsigned char *ConstStr255Param;
typedef const unsigned char *ConstStringPtr;

/* --- 幾何 --- */
typedef struct Point {
    SInt16 v;   /* 縱 (y) */
    SInt16 h;   /* 橫 (x) */
} Point;

typedef struct Rect {
    SInt16 top;
    SInt16 left;
    SInt16 bottom;
    SInt16 right;
} Rect;

/* --- 色彩 (各通道 16-bit, 對齊 QuickDraw RGBColor) --- */
typedef struct RGBColor {
    UInt16 red;
    UInt16 green;
    UInt16 blue;
} RGBColor;

/* --- CopyBits 傳輸模式 (僅保留 U3 用到的) --- */
enum {
    srcCopy   = 0,
    srcOr     = 1,
    srcXor    = 2,
    srcBic    = 3,
    blend     = 32,      /* LairWare 自訂半透明,shim 以 alpha 近似 */
    transparent = 36
};

/* --- 主題字型 ID (對應 UDrawThemePascalString 參數;shim 內僅做尺寸映射) --- */
typedef UInt16 ThemeFontID;
enum {
    kThemeSystemFont        = 0,
    kThemeSmallSystemFont   = 1,
    kThemeCurrentPortFont   = 200
};

/* --- 通用 handle/ptr --- */
typedef char *Ptr;
typedef Ptr  *Handle;

#endif /* U3_COMPAT_MAC_TYPES_H */
