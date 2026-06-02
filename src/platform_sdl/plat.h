/* plat.h — 平台層原型 (SDL2 取代 MacIF / Cocoa / AppleEvents / QuickTime)
 *
 * 重點:所有「回傳指標 / handle / CFTypeRef」的函式必須在此宣告正確原型,
 * 否則 64-bit 下隱式 int 宣告會把回傳值截成 32-bit,導致指標崩潰。
 * 由 mac_shim.h 末尾 #include。 */
#ifndef U3_PLATFORM_SDL_PLAT_H
#define U3_PLATFORM_SDL_PLAT_H

/* mac_shim.h 已先 include mac_types/quickdraw/CoreFoundation 子集,
 * 此處直接使用其型別。 */

/* ===== 視窗 (回傳 WindowPtr = GrafPort*) ===== */
WindowPtr NewWindow(void *storage, const Rect *bounds, ConstStringPtr title,
                    Boolean visible, SInt16 procID, WindowPtr behind,
                    Boolean goAway, SInt32 refCon);
WindowPtr NewCWindow(void *storage, const Rect *bounds, ConstStringPtr title,
                     Boolean visible, SInt16 procID, WindowPtr behind,
                     Boolean goAway, SInt32 refCon);
WindowPtr GetNewCWindow(SInt16 windowID, void *storage, WindowPtr behind);
DialogPtr GetNewDialog(SInt16 dialogID, void *storage, WindowPtr behind);
WindowPtr GetDialogWindow(DialogPtr dlg);
CGrafPtr  GetWindowPort(WindowPtr win);

/* ===== Region (回傳不透明指標) ===== */
RgnHandle NewRgn(void);
RgnHandle GetGrayRgn(void);

/* ===== 記憶體 (回傳指標/handle) ===== */
Ptr    NewPtr(Size byteCount);
Ptr    NewPtrClear(Size byteCount);
Handle NewHandle(Size byteCount);
Handle NewHandleClear(Size byteCount);

/* ===== 對話框過濾器 UPP ===== */
ModalFilterUPP NewModalFilterUPP(void *proc);

/* ===== 資源管理 (回傳 handle) ===== */
Handle    GetResource(ResType type, SInt16 id);
Handle    Get1Resource(ResType type, SInt16 id);
PicHandle GetPicture(SInt16 id);
CTabHandle GetCTable(SInt16 id);
CursHandle GetCursor(SInt16 id);

/* ===== 選單 / 控制項 (回傳 handle) ===== */
MenuHandle GetMenuHandle(SInt16 menuID);
MenuRef    GetControlPopupMenuHandle(ControlRef ctrl);

/* ===== CoreFoundation 目錄 / URL / 字串 (回傳 CFTypeRef) ===== */
CFURLRef    ResourcesDirectoryURL(void);
CFURLRef    GraphicsDirectoryURL(void);
CFArrayRef  CopyGraphicsDirectoryItems(void);
CFArrayRef  StringsArray(CFStringRef identifier);
CFStringRef CopyCatStrings(CFStringRef a, CFStringRef b);
CFStringRef CopyAppVersionString(void);
CFStringRef CFStringCreateWithPascalString(CFAllocatorRef alloc, ConstStr255Param pstr, CFStringEncoding enc);
CFStringRef CFStringCreateWithSubstring(CFAllocatorRef alloc, CFStringRef s, CFRange range);
ConstStringPtr CFStringGetPascalStringPtr(CFStringRef s, CFStringEncoding enc);
CFURLRef    CFURLCreateCopyAppendingPathComponent(CFAllocatorRef alloc, CFURLRef base,
                                                  CFStringRef comp, Boolean isDir);
CFNumberRef CFNumberCreate(CFAllocatorRef alloc, CFNumberType type, const void *valuePtr);
CFTypeRef   CFDictionaryGetValue(CFDictionaryRef dict, const void *key);
const void *CFArrayGetValueAtIndex(CFArrayRef arr, CFIndex idx);
CFTypeRef   CFPreferencesCopyAppValue(CFStringRef key, CFStringRef appID);

#endif /* U3_PLATFORM_SDL_PLAT_H */
