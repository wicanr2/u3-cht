/* plat_cf.c — CoreFoundation 最小 shim (字串 / URL / 陣列 / 數字 / 字典 / prefs)
 *
 * 模型:
 *   CFStringRef  = const char*  (UTF-8 C 字串)。CFSTR() 產生的是字面值,不可 free。
 *                  本檔自行 malloc 的字串以 cf_str_make() 包成可釋放 wrapper:
 *                  為簡化,malloc 的字串前 4 byte 放 magic,CFRelease 才 free。
 *                  但 game 也會把 CFSTR 字面值傳給 CFRelease → 用「指標範圍無法判斷」,
 *                  故策略:CFRelease 一律不 free (移植期可接受小量漏記憶體),
 *                  避免誤 free 字面值造成崩潰。
 *   CFURLRef     = CFURLObj*  (持有 strdup 的 path)。
 *   CFArrayRef   = CFArrayObj* (持有 char** 字串陣列)。
 *   CFNumberRef  = CFNumberObj* (持有 long 值)。
 *   CFDictionaryRef = 顯示模式查詢用,回 NULL (顯示模式路徑停用)。
 */
#include "mac_shim.h"
#include <stdio.h>
#include <dirent.h>
#include <strings.h>   /* strcasecmp */

/* ---- 物件型別 (與 CFTypeRef 相容,皆為 heap 指標) ---- */
typedef struct CFURLObj   { char *path; } CFURLObj;
typedef struct CFArrayObj { char **items; CFIndex count; } CFArrayObj;
typedef struct CFNumberObj { long value; } CFNumberObj;

/* CFStringRef 在本 shim = const char*;以下 helper 取 C 字串。 */
static const char *cf_cstr(CFStringRef s) { return (const char *)s; }

/* ===== CFString ===== */

CFStringRef CFStringCreateWithPascalString(CFAllocatorRef alloc, ConstStr255Param pstr,
                                           CFStringEncoding enc) {
    (void)alloc; (void)enc;
    if (!pstr) return (CFStringRef)strdup("");
    int len = pstr[0];
    char *c = (char *)malloc(len + 1);
    memcpy(c, pstr + 1, len);
    c[len] = '\0';
    return (CFStringRef)c;
}

CFStringRef CFStringCreateWithSubstring(CFAllocatorRef alloc, CFStringRef s, CFRange range) {
    (void)alloc;
    const char *src = cf_cstr(s);
    if (!src) return (CFStringRef)strdup("");
    long n = range.length;
    if (n < 0) n = 0;
    char *c = (char *)malloc(n + 1);
    memcpy(c, src + range.location, n);
    c[n] = '\0';
    return (CFStringRef)c;
}

/* 回傳指向內部 buffer 的 Pascal 字串指標。每次呼叫覆蓋同一 thread-local 緩衝。 */
ConstStringPtr CFStringGetPascalStringPtr(CFStringRef s, CFStringEncoding enc) {
    (void)enc;
    static unsigned char buf[256];
    const char *c = cf_cstr(s);
    if (!c) { buf[0] = 0; return buf; }
    int len = (int)strlen(c);
    if (len > 255) len = 255;
    buf[0] = (unsigned char)len;
    memcpy(buf + 1, c, len);
    return buf;
}

Boolean CFStringGetPascalString(CFStringRef s, StringPtr buffer, CFIndex bufferSize,
                                CFStringEncoding enc) {
    (void)enc;
    const char *c = cf_cstr(s);
    if (!c) { if (buffer && bufferSize > 0) buffer[0] = 0; return true; }
    int len = (int)strlen(c);
    if (len > 255) len = 255;
    if (bufferSize > 0 && len + 1 > bufferSize) len = (int)bufferSize - 1;
    if (buffer) {
        buffer[0] = (unsigned char)len;
        memcpy(buffer + 1, c, len);
    }
    return true;
}

CFStringEncoding CFStringGetSystemEncoding(void) { return kCFStringEncodingUTF8; }

CFRange CFStringFind(CFStringRef s, CFStringRef find, UInt32 flags) {
    (void)flags;
    CFRange r;
    const char *hay = cf_cstr(s);
    const char *need = cf_cstr(find);
    r.location = kCFNotFound;
    r.length = 0;
    if (hay && need) {
        const char *p = strstr(hay, need);
        if (p) { r.location = (long)(p - hay); r.length = (long)strlen(need); }
    }
    return r;
}

CFComparisonResult CFStringCompare(CFStringRef a, CFStringRef b, UInt32 flags) {
    (void)flags;
    const char *sa = cf_cstr(a), *sb = cf_cstr(b);
    int cmp = strcmp(sa ? sa : "", sb ? sb : "");
    return (cmp < 0) ? kCFCompareLessThan : (cmp > 0) ? kCFCompareGreaterThan : kCFCompareEqualTo;
}

Boolean CFStringHasPrefix(CFStringRef s, CFStringRef prefix) {
    const char *str = cf_cstr(s), *pre = cf_cstr(prefix);
    if (!str || !pre) return false;
    size_t n = strlen(pre);
    return strncmp(str, pre, n) == 0;
}

/* ===== CFRange ===== */
CFRange CFRangeMake(CFIndex loc, CFIndex len) {
    CFRange r; r.location = loc; r.length = len; return r;
}

/* ===== CFRelease (移植期保守:不 free,避免誤 free 字面值) ===== */
void CFRelease(CFTypeRef ref) { (void)ref; }

/* ===== CFURL ===== */

static CFURLObj *url_make(const char *path) {
    CFURLObj *u = (CFURLObj *)malloc(sizeof(CFURLObj));
    u->path = strdup(path ? path : ".");
    return u;
}

CFURLRef ResourcesDirectoryURL(void) {
    return (CFURLRef)url_make("assets");
}

CFURLRef GraphicsDirectoryURL(void) {
    return (CFURLRef)url_make("assets/graphics");
}

CFURLRef CFURLCreateCopyAppendingPathComponent(CFAllocatorRef alloc, CFURLRef base,
                                               CFStringRef comp, Boolean isDir) {
    (void)alloc; (void)isDir;
    const CFURLObj *b = (const CFURLObj *)base;
    const char *c = cf_cstr(comp);
    const char *bp = (b && b->path) ? b->path : ".";
    char *full = (char *)malloc(strlen(bp) + 1 + (c ? strlen(c) : 0) + 1);
    sprintf(full, "%s/%s", bp, c ? c : "");
    CFURLObj *u = (CFURLObj *)malloc(sizeof(CFURLObj));
    u->path = full;
    return (CFURLRef)u;
}

/* CFURLGetFSRef:把 url 的 path 塞進 FSRef.hidden (C 字串)。 */
Boolean CFURLGetFSRef(CFURLRef url, FSRef *fsr) {
    const CFURLObj *u = (const CFURLObj *)url;
    if (!u || !fsr) return false;
    strncpy((char *)fsr->hidden, u->path, sizeof(fsr->hidden) - 1);
    fsr->hidden[sizeof(fsr->hidden) - 1] = '\0';
    return true;
}

/* ===== CFArray (字串清單) ===== */

CFIndex CFArrayGetCount(CFArrayRef arr) {
    const CFArrayObj *a = (const CFArrayObj *)arr;
    return a ? a->count : 0;
}

const void *CFArrayGetValueAtIndex(CFArrayRef arr, CFIndex idx) {
    const CFArrayObj *a = (const CFArrayObj *)arr;
    if (!a || idx < 0 || idx >= a->count) return NULL;
    return (const void *)a->items[idx];   /* 回傳 CFStringRef = const char* */
}

/* 列出 assets/graphics 下的 *.png / *.gif 檔名 (供 GetGraphics 掃描)。 */
CFArrayRef CopyGraphicsDirectoryItems(void) {
    CFArrayObj *a = (CFArrayObj *)malloc(sizeof(CFArrayObj));
    a->items = NULL; a->count = 0;
    size_t cap = 0;
    DIR *d = opendir("assets/graphics");
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            const char *name = e->d_name;
            size_t ln = strlen(name);
            int ok = (ln > 4 && (strcasecmp(name + ln - 4, ".png") == 0 ||
                                 strcasecmp(name + ln - 4, ".gif") == 0)) ||
                     (ln > 4 && strcasecmp(name + ln - 4, ".pdf") == 0);
            if (!ok) continue;
            if (a->count == (CFIndex)cap) {
                cap = cap ? cap * 2 : 16;
                a->items = (char **)realloc(a->items, cap * sizeof(char *));
            }
            a->items[a->count++] = strdup(name);
        }
        closedir(d);
    }
    return (CFArrayRef)a;
}

/* ===== CFNumber ===== */

CFNumberRef CFNumberCreate(CFAllocatorRef alloc, CFNumberType type, const void *valuePtr) {
    (void)alloc;
    CFNumberObj *n = (CFNumberObj *)malloc(sizeof(CFNumberObj));
    long v = 0;
    switch (type) {
        case kCFNumberShortType:  v = *(const short *)valuePtr; break;
        case kCFNumberIntType:    v = *(const int *)valuePtr;   break;
        default:                  v = *(const int *)valuePtr;   break;
    }
    n->value = v;
    return (CFNumberRef)n;
}

Boolean CFNumberGetValue(CFNumberRef num, CFNumberType type, void *valuePtr) {
    const CFNumberObj *n = (const CFNumberObj *)num;
    if (!n || !valuePtr) return false;
    switch (type) {
        case kCFNumberShortType:  *(short *)valuePtr = (short)n->value; break;
        case kCFNumberIntType:    *(int *)valuePtr = (int)n->value;     break;
        default:                  *(int *)valuePtr = (int)n->value;     break;
    }
    return true;
}

/* ===== CFDictionary (僅顯示模式查詢;停用 → NULL) ===== */
CFTypeRef CFDictionaryGetValue(CFDictionaryRef dict, const void *key) {
    (void)dict; (void)key;
    return NULL;
}

/* ===== CFPreferences (讀:回 NULL → game 用預設值;寫/同步:no-op) ===== */
CFTypeRef CFPreferencesCopyAppValue(CFStringRef key, CFStringRef appID) {
    (void)appID;
    /* TileSet 偏好 — 讓既有 GetGraphics() 多平台 tileset 切換生效。
     * 原 stub 永遠回 NULL → GetGraphics 鎖死 "Standard" (彩色)。
     * 單色原始線條模式 POC:以 env U3_TILESET 餵入 tileset 名 (如 "Apple II Mono"
     * / "Macintosh B&W");env-gated,未設時回 NULL 維持 Standard,不影響正常遊玩。
     * 回 strdup 字串 (shim CFStringRef=const char*);CFRelease 不 free → 僅啟動時微 leak。
     * 未來:改接 cf_bridge 字串偏好持久化 + 選項對話切換。 */
    if (key && strcmp(cf_cstr(key), "TileSet") == 0) {
        const char *v = getenv("U3_TILESET");
        if (v && *v) return (CFStringRef)strdup(v);
    }
    return NULL;
}
Boolean CFPreferencesAppSynchronize(CFStringRef appID) { (void)appID; return true; }

/* ===== CopyCatStrings (CocoaBridge.h):串接兩個字串 ===== */
CFStringRef CopyCatStrings(CFStringRef a, CFStringRef b) {
    const char *sa = cf_cstr(a), *sb = cf_cstr(b);
    char *c = (char *)malloc((sa ? strlen(sa) : 0) + (sb ? strlen(sb) : 0) + 1);
    c[0] = '\0';
    if (sa) strcpy(c, sa);
    if (sb) strcat(c, sb);
    return (CFStringRef)c;
}

/* ===== CopyAppVersionString ===== */
CFStringRef CopyAppVersionString(void) { return (CFStringRef)"U3-CHT 1.0"; }

/* ===== StringsArray:回傳具名字串表 (用於計數)。game 以
 * CFArrayGetCount((CFArrayRef)StringsArray(id)) 取數量 (如隨機名字從 Female/Male/
 * Intersex 表挑選)。回一個 count=Strings_Count(表名) 的 CFArrayObj (items 不需,
 * 取字走 GetPascalStringFromArrayByIndex)。 */
extern int Strings_Count(const char *table);
CFArrayRef StringsArray(CFStringRef identifier) {
    static CFArrayObj a;
    const char *name = (const char *)identifier;
    a.items = NULL;
    a.count = name ? (CFIndex)Strings_Count(name) : 0;
    return (CFArrayRef)&a;
}
