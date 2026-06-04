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
#include <stdbool.h>

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
    osEvt = 15, app4Evt = 15, kHighLevelEvt = 23,
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

/* ===== 函式指標 / 雜項型別 ===== */
typedef void *UniversalProcPtr;
typedef void *ModalFilterUPP;
typedef FourCharCode ResType;
typedef Handle StringHandle;
typedef SInt16 DialogItemType;
typedef int boolean_t;
typedef struct CGPoint { Float64 x, y; } CGPoint;
typedef struct CGSize  { Float64 width, height; } CGSize;
typedef struct CGRect  { CGPoint origin; CGSize size; } CGRect;
typedef UInt32 CGDirectDisplayID;
typedef SInt32 CGDisplayErr;
typedef SInt32 CGError;
typedef struct AlertStdAlertParamRec {
    Boolean movable; Boolean helpButton; ModalFilterUPP filterProc;
    ConstStringPtr defaultText, cancelText, otherText;
    Boolean defaultButton, cancelButton; UInt16 position;
} AlertStdAlertParamRec;

/* ===== Carbon 常數批次 (移植期型別解析;相關函式多會以 SDL 重寫/停用) ===== */
enum {
    /* 文字 face / transfer 模式 */
    normal = 0, bold = 1, italic = 2, underline = 4,
    addOver = 35,
    /* 事件 / 修飾鍵 */
    kHighLevelEvent = 23, cmdKey = 0x0100, optionKey = 0x0800,
    shiftKey = 0x0200, controlKey = 0x1000, charCodeMask = 0x000000FF,
    /* 視窗部位 */
    inDesk = 0, inMenuBar = 1, inContent = 3, inDrag = 4, inGoAway = 6,
    kControlButtonPart = 10,
    /* Alert 類型 / 按鈕 */
    kAlertStopAlert = 0, kAlertNoteAlert = 1, kAlertCautionAlert = 2,
    kAlertStdAlertOKButton = 1, kAlertStdAlertCancelButton = 2,
    /* 檔案權限 / 定位 */
    fsRdPerm = 1, fsRdWrPerm = 3, fsFromStart = 1, fsCurPerm = 0,
    permErr = -54, kOnSystemDisk = -32768,
    kDontCreateFolder = 0, kPreferencesFolderType = 0x70726566 /* 'pref' */,
    /* Gestalt / 顯示管理 */
    gestaltDisplayMgrPresent = 0, gestaltDisplayMgrAttr = 0x6473706c,
    dmOnlyActiveDisplays = 1,
    /* HI command / 視窗位置 / script */
    kHICommandPreferences = 0x70726566, kWindowDefaultPosition = 0,
    smSystemScript = -1,
    /* 視窗類型 / 系統視窗 / 轉場 */
    plainDBox = 2, altDBoxProc = 3, documentProc = 0, inSysWindow = 2,
    kWindowHideTransitionAction = 1, kWindowShowTransitionAction = 0,
    /* Gestalt selectors / QD 版本 */
    gestalt32BitQD = 5, gestaltQuickdrawVersion = 0x71642020,
    gestaltQuickTimeVersion = 0x71747320, gestaltProcessorType = 0x70726f63,
    gestaltAddressingModeAttr = 0x61646472,
    gestalt68000 = 0, gd2BitQD = 1
};
#define kCFNotFound (-1)
/* CoreGraphics 顯示常數 */
#define kCGDirectMainDisplay ((CGDirectDisplayID)0)
#define kCGErrorSuccess      (0)
#define kCGDisplayWidth      ((CFStringRef)"Width")
#define kCGDisplayHeight     ((CFStringRef)"Height")

/* WindowPeek:Classic 視窗結構窺視 (僅用到 goAwayFlag);移植期供型別解析 */
typedef struct WindowRecord {
    GrafPort port;
    Boolean  goAwayFlag;
    Boolean  visible;
} WindowRecord;
typedef WindowRecord *WindowPeek;

/* QuickDraw 全域 (qd.thePort / qd.randSeed) */
typedef struct QDGlobals {
    GrafPtr thePort;
    SInt32  randSeed;
    BitMap  screenBits;
} QDGlobals;
extern QDGlobals qd;

typedef struct NumVersion { UInt8 majorRev, minorAndBugRev, stage, nonRelRev; } NumVersion;
typedef Handle MCTableHandle;
typedef struct CFRange { long location, length; } CFRange;
NumVersion SndSoundManagerVersion(void);

/* ===== Carbon 雜項型別/常數 (移植期型別解析用) ===== */
typedef SInt32 Size;
typedef UInt16 MenuItemIndex;
typedef Handle CursHandle;
typedef struct FSSpec { SInt16 vRefNum; SInt32 parID; Str63 name; } FSSpec;
typedef struct FSRef  { UInt8 hidden[80]; } FSRef;
typedef struct OpaqueComponentInstance *ComponentInstance;
typedef struct SndCommand { UInt16 cmd; SInt16 param1; SInt32 param2; } SndCommand;
typedef struct OpaqueSndChannel *SndChannelPtr;
typedef struct OpaquePicHandle  *PicHandle;
typedef struct OpaquePolyHandle *PolyHandle;
enum {
    kFSCatInfoNone = 0,
    ditherCopy = 64,          /* CopyBits 模式 */
    watchCursor = 4,          /* 游標資源 ID */
    soundCmd = 80, initMono = 0x0080
};

/* 常見 OSErr / 主題 / 文字對齊常數 */
enum {
    paramErr = -50, fnfErr = -43, memFullErr = -108,
    kThemeStateActive = 1, kThemeStateInactive = 0,
    teFlushDefault = 0, teCenter = 1, teFlushRight = -1, teFlushLeft = 0
};

/* ===== CoreFoundation (最小子集) ===== */
typedef const void *CFTypeRef;
typedef const struct __CFString *CFStringRef;
typedef const struct __CFURL    *CFURLRef;
typedef const struct __CFArray  *CFArrayRef;
typedef struct __CFArray  *CFMutableArrayRef;
typedef const struct __CFDictionary *CFDictionaryRef;
typedef struct __CFDictionary *CFMutableDictionaryRef;
typedef struct __CFBundle *CFBundleRef;
typedef const struct __CFAllocator *CFAllocatorRef;
#define kCFAllocatorDefault ((CFAllocatorRef)0)
typedef const struct __CFNumber *CFNumberRef;
typedef SInt32 CFNumberType;
enum { kCFNumberShortType = 3, kCFNumberIntType = 9, kCFNumberSInt32Type = 3 };
typedef unsigned long CFIndex;
CFRange CFStringFind(CFStringRef s, CFStringRef find, UInt32 flags);
CFRange CFRangeMake(CFIndex loc, CFIndex len);
typedef UInt32 CFStringEncoding;
typedef Boolean CFComparisonResult; /* 簡化 */

#define kCFStringEncodingUTF8     0x08000100
#define kCFStringEncodingMacRoman 0

/* CFSTR:在 shim 內以一般 C 字串字面值代用 (string table key 用) */
#define CFSTR(s) ((CFStringRef)(s))

/* CF 布林 / 比較常數 */
#define kCFBooleanTrue  ((CFTypeRef)1)
#define kCFBooleanFalse ((CFTypeRef)0)
enum { kCFCompareLessThan = -1, kCFCompareEqualTo = 0, kCFCompareGreaterThan = 1 };

/* CFPreferences (U3 用來讀設定;shim 先給保守預設) */
Boolean CFPreferencesGetAppBooleanValue(CFStringRef key, CFStringRef appID, Boolean *keyExistsAndHasValidFormat);
void    CFPreferencesSetAppValue(CFStringRef key, CFTypeRef value, CFStringRef appID);
CFIndex CFPreferencesGetAppIntegerValue(CFStringRef key, CFStringRef appID, Boolean *keyExistsAndHasValidFormat);
extern CFStringRef kCFPreferencesCurrentApplication;

/* ===== 字串表存取 (取代 Cocoa CocoaBridge.m) ===== */
/* 宣告與 src/text/strings.h 對齊;identifier 為 CFStringRef(實為 C 字串) */
void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index);

/* ===== 平台層原型 (回傳指標/handle/CFTypeRef 者務必正確宣告) =====
 * CF 函式 prototype 必須在此 (被 -include 到所有上游 .c),否則上游隱式宣告回 int,
 * 回 64-bit 指標 (CFArrayGetValueAtIndex 等) 者會被截斷 → GetGraphics 目錄掃描錯亂、
 * tileset 載不進 (clang -Wno-implicit-function-declaration 壓掉了警告所以難察覺)。 */
CFIndex            CFArrayGetCount(CFArrayRef arr);
const void        *CFArrayGetValueAtIndex(CFArrayRef arr, CFIndex idx);
CFTypeRef          CFPreferencesCopyAppValue(CFStringRef key, CFStringRef appID);
CFURLRef           CFURLCreateCopyAppendingPathComponent(CFAllocatorRef alloc, CFURLRef base,
                                                         CFStringRef comp, Boolean isDir);
CFComparisonResult CFStringCompare(CFStringRef a, CFStringRef b, UInt32 flags);
Boolean            CFStringHasPrefix(CFStringRef s, CFStringRef prefix);
Boolean            CFURLGetFSRef(CFURLRef url, FSRef *fsr);

#include "../platform_sdl/plat.h"

#endif /* U3_COMPAT_MAC_SHIM_H */
