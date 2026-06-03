/* compat_test_stubs.c — cmake PoC 單元測試專用的平台層全域樁。
 *
 * u3compat 內 qd_text.c 的按鈕/標籤中文化函式 (U3_DrawButtonLabel 等) 會引用遊戲
 * 平台層全域 `mainPort`。正式遊戲由上游 (UltimaMain.c) 提供其強定義;但獨立的
 * 單元測試 (test_qd_*, test_strings_render) 只連結 u3compat,故需在此補一個 NULL
 * 預設使連結通過。測試本身不呼叫那些按鈕函式,故 NULL 不影響測試行為。 */
#include "mac_shim.h"

CGrafPtr mainPort = NULL;
