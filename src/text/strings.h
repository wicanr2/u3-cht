/* strings.h — 執行期 UTF-8 字串表讀取器 (取代 Cocoa GetPascalStringFromArrayByIndex)
 *
 * 讀 tools/extract_strings.py 產出的 .u3s 檔。支援 >255 byte UTF-8。
 * 語言由 Strings_SetLang() 決定 (預設 zh-Hant,缺表自動回退 en)。 */
#ifndef U3_TEXT_STRINGS_H
#define U3_TEXT_STRINGS_H

#include "../compat/mac_types.h"

/* 設定字串表根目錄 (內含 <lang>/<Table>.u3s),預設 "assets/strings" */
void Strings_SetRoot(const char *root);
/* 設定語言 (如 "zh-Hant" / "en");載入時找不到該語言會回退 en */
void Strings_SetLang(const char *lang);
/* 釋放所有已載入表 */
void Strings_Shutdown(void);

/* 取 UTF-8 字串 (常駐於表記憶體,勿釋放)。越界或載入失敗回 "" */
const char *Strings_GetUTF8(const char *table, int index);
/* 該表字串數;表不存在回 0 */
int Strings_Count(const char *table);

/* 相容原碼:填入 Pascal Str255 (UTF-8 bytes,截斷 255)。
 * 新繪字路徑請直接用 Strings_GetUTF8 + U3_DrawUTF8 以支援長中文。
 * (CFStringRef 版 GetPascalStringFromArrayByIndex 由 cf_bridge.c 橋接到此) */
void Strings_GetPascal(StringPtr pstringPtr, const char *table, int index);

#endif /* U3_TEXT_STRINGS_H */
