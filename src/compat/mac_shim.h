/* mac_shim.h — 「假 Carbon」umbrella
 *
 * 上游每個 .c 經 prefix pch 引入 <Carbon/Carbon.h>。Linux 上以本檔取代:
 * 提供 Mac 基本型別 + QuickDraw (compat) + CoreFoundation/Carbon 補充宣告。
 * 用法:編譯時 -include mac_shim.h,並把 fakeinc/ 放進 -I 讓
 * <Carbon/Carbon.h>、<QuickTime/QuickTime.h> 轉指本檔。
 *
 * 宣告為主;實作隨移植逐步補在 compat/。 */
#ifndef U3_COMPAT_MAC_SHIM_H
#define U3_COMPAT_MAC_SHIM_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "mac_types.h"
#include "qd_geometry.h"
#include "quickdraw.h"
#include "qd_text.h"

/* ===== 額外基本型別 ===== */
typedef SInt32  Fixed;
typedef UInt32  FourCharCode;
typedef FourCharCode OSType;
typedef double  Float64;
typedef float   Float32;

/* Classic Mac 語言慣用 */
#ifndef pascal
#define pascal   /* 移除呼叫慣例關鍵字 */
#endif
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ===== Carbon 視窗/對話框/事件 (opaque,移植期僅供型別解析) ===== */
typedef SInt16 AlertType;
typedef struct OpaqueDialogPtr   *DialogPtr;
typedef DialogPtr                 DialogRef;
typedef struct OpaqueControlRef  *ControlRef;
typedef ControlRef                ControlHandle;
typedef struct OpaqueMenuRef     *MenuRef;
typedef MenuRef                   MenuHandle;
typedef UInt32 EventModifiers;
typedef UInt16 EventKind;
/* Classic Mac 事件類型與遮罩 */
enum {
    nullEvent = 0, mouseDown = 1, mouseUp = 2, keyDown = 3, keyUp = 4,
    autoKey = 5, updateEvt = 6, activateEvt = 8,
    mDownMask = 1<<mouseDown, mUpMask = 1<<mouseUp,
    keyDownMask = 1<<keyDown, keyUpMask = 1<<keyUp,
    autoKeyMask = 1<<autoKey, updateMask = 1<<updateEvt,
    everyEvent = 0xFFFF
};
typedef struct EventRecord {
    EventKind what;
    UInt32    message;
    UInt32    when;
    Point     where;
    EventModifiers modifiers;
} EventRecord;

/* Absolute 由 UltimaMisc.h 宣告 (short Absolute(short)),實作在 cf_bridge.c */

/* ===== CoreFoundation (最小子集) ===== */
typedef const void *CFTypeRef;
typedef const struct __CFString *CFStringRef;
typedef const struct __CFURL    *CFURLRef;
typedef const struct __CFArray  *CFArrayRef;
typedef const struct __CFDictionary *CFDictionaryRef;
typedef struct __CFDictionary *CFMutableDictionaryRef;
typedef struct __CFBundle *CFBundleRef;
typedef unsigned long CFIndex;
typedef UInt32 CFStringEncoding;
typedef Boolean CFComparisonResult; /* 簡化 */

#define kCFStringEncodingUTF8     0x08000100
#define kCFStringEncodingMacRoman 0

/* CFSTR:在 shim 內以一般 C 字串字面值代用 (string table key 用) */
#define CFSTR(s) ((CFStringRef)(s))

/* CFPreferences (U3 用來讀設定;shim 先給保守預設) */
Boolean CFPreferencesGetAppBooleanValue(CFStringRef key, CFStringRef appID, Boolean *keyExistsAndHasValidFormat);
void    CFPreferencesSetAppValue(CFStringRef key, CFTypeRef value, CFStringRef appID);
CFIndex CFPreferencesGetAppIntegerValue(CFStringRef key, CFStringRef appID, Boolean *keyExistsAndHasValidFormat);
extern CFStringRef kCFPreferencesCurrentApplication;

/* ===== 字串表存取 (取代 Cocoa CocoaBridge.m) ===== */
/* 宣告與 src/text/strings.h 對齊;identifier 為 CFStringRef(實為 C 字串) */
void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index);

#endif /* U3_COMPAT_MAC_SHIM_H */
