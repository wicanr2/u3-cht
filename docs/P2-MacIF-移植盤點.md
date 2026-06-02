# P2 — `UltimaMacIF.c` 移植稅盤點

> 來源:`ultima3/Sources/UltimaMacIF.c`(約 2,765 行,Mac Carbon/QuickDraw 平台層)。
> 目的:產出 SDL2 平台層的實作 backlog 與依賴順序。
> 日期:2026-06-02

## 1. 四類分布

| 分類 | 函式數 | 總行數 | 占比 |
|---|---|---|---|
| 視窗/顯示 | 19 | ~1,150 | 41.5% |
| 事件/輸入 | 18 | ~915 | 33.0% |
| 檔案/IO | 7 | ~156 | 5.6% |
| 純邏輯/可攜 | 6 | ~53 | 1.9% |

預估產出:約 **2,000–2,500 行新 SDL2 程式碼**(含 `DrawFancyRecord` 重寫),約等於原檔 100% 工作量。

## 2. 移植依賴順序(讓主迴圈先跑起來)

### Phase A — 核心框架(先決)
1. `ToolBoxInit` (18) → SDL2 init + 事件佇列
2. `WindowInit` (71) → `SDL_Window` 建立、視窗尺寸
3. `SetUpGWorlds` (89) → 離屏 surface/texture(**架構關鍵**,11 個離屏埠)
4. `SetUpFont` (23) → 字型載入(接 text 模組 SDL_ttf)

### Phase B — 事件與輸入
5. `HandleMouseDown` (197) → SDL 滑鼠事件分派(去 `GlobalToLocal`)
6. `CursorUpdate` (272) → 游標邏輯(幾何可保留,換 `SDL_GetMouseState`)
7. `ReflectNewCursor` (39)、`WidgetClick` (88)

### Phase C — 顯示與渲染
8. `DrawFancyRecord` (445) → 角色狀態畫面(**最大單一挑戰**)
9. `ShowHideBackground` (54)、`ClearShroud` (13)

### Phase D — 選單與對話
10. `MenuBarInit` (18)、`ReflectPrefs` (24)
11. `RosterSelect` (99)、`CharacterCreateDialog` (249) → 改 SDL 自繪模態 UI

### Phase E — 雜項/清理
12. `TearDownGWorlds` (18)、`MyHideMenuBar`/`MyShowMenuBar` (41)、`CheckSystemRequirements` (79)

## 3. 最棘手 5 個函式

| 函式 | 行數 | 難度 | 核心障礙 | 策略 |
|---|---|---|---|---|
| `DrawFancyRecord` | 445 | 極難 | QuickDraw(MoveTo/FrameRect/CopyBits)+ `UDrawThemePascalString` + 浮點縮放 | 改 SDL_Render* + SDL_ttf;最後做,先用佔位矩形 |
| `CursorUpdate` | 272 | 難 | 大量 `PtInRect`/`GetMouse`/`LocalToGlobal` 幾何 | 邏輯 100% 可遷,僅換 API(SDL 已全域座標) |
| `CharacterCreateDialog` | 249 | 難 | `GetNewDialog`/`ModalDialog`/`SetControlValue`(無資源檔) | 做成 SDL 模態狀態機 + 自訂 widget |
| `SetUpGWorlds` | 89 | 難 | `NewGWorld`/`LockPixels`,11 個離屏埠 | 建 `OffscreenSurface` 抽象,內部 surface↔texture |
| `HandleMouseDown` | 197 | 難 | `FindWindow`/`MenuSelect`/`DragWindow`/`TrackGoAway` | 去 `GlobalToLocal`;自訂菜單分派;保留 WidgetClick/AddMacro |

## 4. MVP 最快路徑

`ToolBoxInit` → `WindowInit` → `SetUpGWorlds` → `SetUpFont` → `CursorUpdate` → `HandleMouseDown`(簡化)→ `DrawFancyRecord`(先佔位)。

## 5. 關鍵設計決策(待寫 ADR)

- **離屏緩衝抽象**:`SetUpGWorlds` 的 11 個 GWorld → 統一 `OffscreenSurface`。
  - 選項 A:SDL_Texture(ARGB,GPU,效率高)
  - 選項 B:SDL_Surface(軟體,直接沿用 PixMap 像素邏輯,相容性好)
  - 建議:抽象層內部可切換,初期用 B 降低風險。
- **座標系**:Mac 視窗相對座標 → SDL 已是視窗相對,移除所有 `GlobalToLocal`/`LocalToGlobal`。
- **對話框**:Mac Dialog Manager → SDL 自繪模態 UI 狀態機(複用於 `RosterSelect`/`CharacterCreateDialog`/`QuitDialog`/`HandleError`)。
