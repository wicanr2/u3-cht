/* cf_bridge.c — CoreFoundation / Carbon 殘留 API 的最小 shim
 *
 * - GetPascalStringFromArrayByIndex(CFStringRef):橋接到字串表讀取器。
 * - Absolute:Carbon 整數絕對值。
 * - CFPreferences*:保守預設 (回 false / 0),待 prefs 系統移植 (P 後續)。 */
#include "mac_shim.h"
#include "../text/strings.h"
#include <string.h>   /* strcmp */
#include <stdlib.h>   /* getenv */

/* 上游呼叫 GetPascalStringFromArrayByIndex(str, CFSTR("Messages"), N);
 * CFSTR 在 shim 下 = (CFStringRef)(const char*),故可還原成 C 字串表名。 */
void GetPascalStringFromArrayByIndex(StringPtr pstringPtr, CFStringRef identifier, int index) {
    const char *table = (const char *)identifier;
    Strings_GetPascal(pstringPtr, table ? table : "", index);
}

/* Absolute 由上游 UltimaMisc.c 定義,此處不重複 */

/* --- CFPreferences:暫以預設值回應,不持久化 --- */
CFStringRef kCFPreferencesCurrentApplication = (CFStringRef)"U3CHT";

Boolean CFPreferencesGetAppBooleanValue(CFStringRef key, CFStringRef appID,
                                        Boolean *exists) {
    (void)appID;
    /* AutoSave 預設開啟:WriteResource 已實作 (plat_resource.c),進入地點時
     * 自動存檔 (角色名冊/隊伍/世界) 到 u3save.dat。測試可設 U3_NOSAVE 關閉以保持確定性。 */
    if (key && strcmp((const char *)key, "AutoSave") == 0) {
        if (exists) *exists = true;
        return getenv("U3_NOSAVE") ? false : true;
    }
    if (exists) *exists = false;
    return false;
}
void CFPreferencesSetAppValue(CFStringRef key, CFTypeRef value, CFStringRef appID) {
    (void)key; (void)value; (void)appID;
}
CFIndex CFPreferencesGetAppIntegerValue(CFStringRef key, CFStringRef appID,
                                        Boolean *exists) {
    (void)key; (void)appID;
    if (exists) *exists = false;
    return 0;
}
