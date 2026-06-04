/* cf_bridge.c — CoreFoundation / Carbon 殘留 API 的最小 shim
 *
 * - GetPascalStringFromArrayByIndex(CFStringRef):橋接到字串表讀取器。
 * - Absolute:Carbon 整數絕對值。
 * - CFPreferences*:保守預設 (回 false / 0),待 prefs 系統移植 (P 後續)。 */
#include "mac_shim.h"
#include "../text/strings.h"
#include <string.h>   /* strcmp */
#include <stdlib.h>   /* getenv */
#include <stdio.h>    /* prefs 檔讀寫 */

/* 上游呼叫 GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), N);
 * CFSTR 在 shim 下 = (CFStringRef)(const char*),故可還原成 C 字串表名。 */
void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index) {
    const char *table = (const char *)identifier;
    Strings_GetPascal(pstringPtr, table ? table : "", index);
}

/* Absolute 由上游 UltimaMisc.c 定義,此處不重複 */

/* --- CFPreferences:暫以預設值回應,不持久化 --- */
CFStringRef kCFPreferencesCurrentApplication = (CFStringRef)"U3CHT";

/* ===== CFPreferences:以鍵→int 儲存,持久化到 u3prefs.txt (供選項對話用) =====
 * key = CFSTR 字面值 = (const char*)。bool 存 0/1;CFNumber (視窗位置等) 存其 long。
 * 預設:AutoSave=1 (除非 U3_NOSAVE),其餘 0。 */
#define MAX_PREF 32
static struct { char name[40]; long val; } gPref[MAX_PREF];
static int gPrefN = 0, gPrefLoaded = 0;
static const char *prefs_path(void) {
    const char *p = getenv("U3_PREFS");
    return (p && *p) ? p : "u3prefs.txt";
}
static void prefs_load(void) {
    if (gPrefLoaded) return;
    gPrefLoaded = 1;
    FILE *f = fopen(prefs_path(), "r");
    if (!f) return;
    char nm[40]; long v;
    while (gPrefN < MAX_PREF && fscanf(f, "%39s %ld", nm, &v) == 2) {
        strncpy(gPref[gPrefN].name, nm, sizeof(gPref[0].name) - 1);
        gPref[gPrefN].val = v; gPrefN++;
    }
    fclose(f);
}
static void prefs_save(void) {
    FILE *f = fopen(prefs_path(), "w");
    if (!f) return;
    for (int i = 0; i < gPrefN; i++) fprintf(f, "%s %ld\n", gPref[i].name, gPref[i].val);
    fclose(f);
}
static long *pref_find(const char *name) {
    for (int i = 0; i < gPrefN; i++) if (strcmp(gPref[i].name, name) == 0) return &gPref[i].val;
    return NULL;
}
static void pref_set(const char *name, long v) {
    long *p = pref_find(name);
    if (p) { *p = v; }
    else if (gPrefN < MAX_PREF) { strncpy(gPref[gPrefN].name, name, sizeof(gPref[0].name) - 1); gPref[gPrefN].val = v; gPrefN++; }
    prefs_save();
}

Boolean CFPreferencesGetAppBooleanValue(CFStringRef key, CFStringRef appID, Boolean *exists) {
    (void)appID;
    prefs_load();
    const char *k = (const char *)key;
    long *p = k ? pref_find(k) : NULL;
    if (p) { if (exists) *exists = true; return *p ? true : false; }
    if (k && strcmp(k, "AutoSave") == 0) {     /* 存檔預設開 (除非 U3_NOSAVE) */
        if (exists) *exists = true;
        return getenv("U3_NOSAVE") ? false : true;
    }
    /* OriginalSize 預設「true 且 exists」:SDL 移植固定 640×480 邏輯畫布,
     * blkSiz 必須是 16 (40×24 tile = 640×384)。若回 exists=false,WindowInit 會走
     * CGDisplayCurrentMode (本移植回 NULL) 推算螢幕尺寸 → 讀到未初始化值 → blkSiz
     * 不確定地變成 32,畫面以 2 倍繪入 640×480 埠 → 裁切。固定回 true 消除此 UB。 */
    if (k && strcmp(k, "OriginalSize") == 0) {
        if (exists) *exists = true;
        return true;
    }
    if (exists) *exists = false;
    return false;
}
/* 只持久化「遊戲選項」相關鍵 (音效/音樂/語音/戰鬥/速度);Mac 顯示/視窗偏好
 * (FullScreen/視窗座標/DisplayMode 等) 不適用 SDL 移植,維持 no-op 避免污染與不穩。 */
static int pref_persist(const char *k) {
    static const char *wl[] = { "SoundInactive", "MusicInactive", "SpeechInactive",
                                "ManualCombat", "SpeedUnconstrain", "AutoSave",
                                "GameSpeed",     /* 遊戲速度檔位 (fps cap:20/30/60) */
                                "ColorMode" };   /* 畫面顏色模式 (0彩色1綠磷光2琥珀3灰階) */
    for (unsigned i = 0; i < sizeof(wl) / sizeof(wl[0]); i++)
        if (strcmp(k, wl[i]) == 0) return 1;
    return 0;
}

/* 供 main.c 直接讀寫 long 偏好 (遊戲速度);走同一持久化檔。 */
long U3_PrefGetLong(const char *k, long dflt) {
    prefs_load();
    long *p = k ? pref_find(k) : NULL;
    return p ? *p : dflt;
}
void U3_PrefSetLong(const char *k, long v) {
    if (!k) return;
    prefs_load();
    pref_set(k, v);
}
void CFPreferencesSetAppValue(CFStringRef key, CFTypeRef value, CFStringRef appID) {
    (void)appID;
    const char *k = (const char *)key;
    if (!k || !pref_persist(k)) return;        /* 非選項鍵不存 */
    prefs_load();
    long v;
    if (value == kCFBooleanTrue) v = 1;
    else if (value == kCFBooleanFalse) v = 0;
    else v = value ? *(long *)value : 0;       /* CFNumberObj* { long value; } */
    pref_set(k, v);
}
CFIndex CFPreferencesGetAppIntegerValue(CFStringRef key, CFStringRef appID, Boolean *exists) {
    (void)appID;
    prefs_load();
    const char *k = (const char *)key;
    long *p = k ? pref_find(k) : NULL;
    if (p) { if (exists) *exists = true; return (CFIndex)*p; }
    if (exists) *exists = false;
    return 0;
}
