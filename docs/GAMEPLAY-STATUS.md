# 可玩性狀態與驗證紀錄

> 更新:2026-06-03（第二輪）

## 目前已驗證

- 開場、標題、展示動畫、主選單可顯示繁體中文。
- 主選單可進入 **Organize a Party**。
- SDL 平台層已支援 `FormPartyDialog()` 的移植期自動組隊:選取名冊前 4 名有效角色。
- `Journey Onward` 已可通過 `Party[7]` 檢查並進入 Sosaria 世界地圖。
- game tester 已用 Docker / Xvfb 實機驗證:組隊後進世界,畫面右側顯示 4 名隊員,地圖中可見隊伍/角色並可注入方向鍵移動。
- 世界畫面底部文字滾動區 **Mac 控制字元亂碼已修正**:過濾 0x01–0x1F 後,文字呈現可讀 ASCII。

## gUpdateWhere 狀態對照表

由反組譯 HandleUpdate / MainMenu / Organize / Game 確認:

| 值 | HandleUpdate 路徑 | 設定時機 |
|---|---|---|
| 0 | 無渲染 (blank) | Organize 按鍵後暫時清零 |
| 3 | DrawGamePortToMain(省略) + TextScrollAreaUpdate + ShowChars | Game() 進入時 |
| 4 | TextScrollAreaUpdate + DngInfo + DrawDungeon | DungeonStart() |
| 5 | DrawMenu | MainMenu() |
| 6 | DrawOrganizeMenu + DrawExodusPict | Organize() 頂端 |
| 7 | DrawExodusPict + WideAreaUpdate | FormPartyDialog 開始前 / TerminateCharDialog |
| 8 | TextScrollAreaUpdate only | — |

## 關鍵修正

1. `plat_ui.c`
   - `GetNewDialog(421)` 進入 FormPartyDialog 自動模式。
   - `ModalDialog` 依序送出 MEM1→MEM4→FORM。
   - `GetControlValue` 依目前 MEM 槽回傳對應 popup value,避免 `menuEntry[-2]` 越界。
2. `plat_event.c`
   - game tester 腳本新增 `M <key>` 原始 Mac keycode,可送方向鍵 28–31。
   - game tester 腳本新增 `U <gUpdateWhere>`,可等待指定遊戲畫面狀態,避免開場/展示動畫吃掉後續按鍵。
3. `plat_resource.c`
   - `MAPS` / `MONS` 的目前 Sosaria `419` 若不存在,回退到原始 Sosaria `420`。Mac 版原本會透過資源寫入建立 419,但目前移植層 `WriteResource` 是 no-op。
4. `main.c` / `build_game.sh`
   - 加入 crash backtrace 與 `-rdynamic`,讓 game tester 崩潰時可定位。
5. `src/compat/qd_text.c`（第二輪新增）
   - `pascal_to_c()` 過濾 0x00–0x1F 控制字元:UPrint 路徑的文字已 `& 0x7f`,剩餘控制碼只讓 SDL_ttf 顯示 `[X]` 亂碼。UTF-8 多位元組 (0x80–0xFF) 原樣保留。
6. `tools/build_and_verify.sh`（第二輪新增）
   - 加入截圖數最小值檢查（`< 20` 視為 FAIL）。
   - 明確輸出 PASS / FAIL。

## 驗證指令

```bash
docker run --rm --user "$(id -u):$(id -g)" \
  -v /home/anr2/u3-cht:/work \
  u3cht bash /work/u3-cht/tools/build_and_verify.sh tests/scripts/play.txt 70 20
```

實測輸出（第二輪，2026-06-03）:

```text
[bv] 重建 build/u3 ...
[bv] 建置 OK
[bv] 跑 tests/scripts/play.txt (timeout 70s)
[U3] MainResources.rsrc 載入:303 個資源
[bv] rc=124
[bv] 截圖數: 474
[bv] PASS
```

最後截圖顯示已進入世界畫面:右側有 Tatiana / Roderic / Norric / Ghiselle 四名隊員，地圖中可見角色移動與戰鬥（怪物受到攻擊後顯示 "AIEEEEEE!" 等文字，不再有 `[X]` 亂碼盒）。

## 仍待後續完善

- 世界底部文字仍有部分前綴數字字元（如 `e7MITTAR`），來自 Mac 文字緩衝的格式碼，需進一步解析 UPrint 文字協定。
- Tatiana 面板顯示「非法師」，可能是字串表索引對應問題，需核對 MISC 資源字串。
- FormPartyDialog 是移植期自動組隊，不是正式 SDL 對話框 UI。
- `WriteResource` 仍是 no-op，角色/隊伍/目前地圖的持久化還不是完整 Mac 版行為。
- 角色建立、存檔/讀檔、正式對話框與更多遊戲命令仍需逐項補齊與驗證。
