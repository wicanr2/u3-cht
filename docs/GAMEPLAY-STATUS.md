# 可玩性狀態與驗證紀錄

> 更新:2026-06-03（第四輪:底部文字中文亂碼根因修正）

## 目前已驗證

- 開場、標題、展示動畫、主選單可顯示繁體中文。
- 主選單可進入 **Organize a Party**。
- SDL 平台層已支援 `FormPartyDialog()` 的移植期自動組隊:選取名冊前 4 名有效角色。
- `Journey Onward` 已可通過 `Party[7]` 檢查並進入 Sosaria 世界地圖。
- game tester 已用 Docker / Xvfb 實機驗證:組隊後進世界,畫面右側顯示 4 名隊員。
- 世界地圖可長時間穩定移動（70s timeout PASS，597 截圖）。
- **戰鬥系統可用**:party 成員 HP 隨怪物攻擊下降，確認基礎戰鬥迴圈運作。
- **底部文字滾動區中文亂碼根因已修正**（見下節「底部文字協定說明」）：戰鬥/方向/狀態訊息現正確顯示中文（「玩家」「命中!」「匕首 攻擊」「未命中!」「北/方向」「通過」等）。
- `非法師` 標籤已確認為正確翻譯（Female-Elf-Thief 不能施法，顯示 "non-mage"）。

## gUpdateWhere 狀態對照表

由反組譯 HandleUpdate / MainMenu / Organize / Game 確認:

| 值 | HandleUpdate 路徑 | 設定時機 |
|---|---|---|
| 0 | 無渲染 (blank) | Organize 按鍵後暫時清零 |
| 3 | TextScrollAreaUpdate + ShowChars | Game() 進入時（世界/城鎮/戰鬥） |
| 4 | TextScrollAreaUpdate + DngInfo + DrawDungeon | DungeonStart() |
| 5 | DrawMenu | MainMenu() |
| 6 | DrawOrganizeMenu + DrawExodusPict | Organize() 頂端 |
| 7 | DrawExodusPict + WideAreaUpdate | FormPartyDialog 開始前 / TerminateCharDialog |
| 8 | TextScrollAreaUpdate only | — |

注：Game() 主迴圈直接呼叫 DrawMap/ShowChars，不依賴 HandleUpdate 繪製地圖。

## 底部文字協定說明（第四輪更正：先前的「Classic tile 索引」診斷有誤）

**真實根因（已修正）**：底部文字並非走 Classic tile bitmap 路徑。`U3PrefClassicAppearance` 偏好在移植層回傳 `false`，故 `UPrint()` 走 **非 classic 路徑**（`NewPrint` → `UDrawThemePascalString` → SDL_ttf），全程文字渲染。

問題出在 `UltimaText.c:366`，非 classic 路徑逐字組字串時做了 `str[++str[0]] = gString[pos] & 0x7F;`。這個 `& 0x7F` 會**砍掉 UTF-8 多位元組中文字的高位元**，把中文打成控制碼亂碼：

| 原文 | UTF-8 bytes | `& 0x7F` 後 | 螢幕呈現 |
|---|---|---|---|
| 通 | `e9 80 9a` | `69 00 1a` | `i`+亂碼 |
| 過 | `e9 81 8e` | `69 01 0e` | 亂碼 |
| 玩 | `e7 8e a9` | `67 0e 29` | `g)` |
| 家 | `e5 ae b6` | `65 2e 36` | `e.6` |

⇒ 先前看到的「e7+MITTAR」「g)e.6」其實是「玩家」等中文被 `& 0x7F` 破壞。怪物名稱（MITTAR）等純 ASCII 不受影響，故能正常顯示，造成「tile code 前綴」的錯覺。

**修正**：移除 `UltimaText.c:366` 的 `& 0x7F`（該段已在 `if (!classic)` 分支內，僅影響中文化路徑；ASCII < 0x80 不受影響，UTF-8 ≥ 0x80 完整保留）。診斷方法：在 `CFStringCreateWithPascalString`（真實繪字漏斗）與 `GetPascalStringFromArrayByIndex` 加 env-gated 控制碼偵測 + `backtrace`，定位到 `UPrint` 而非字串表。

**驗證**：`U3_DBG_TEXT=1` 跑戰鬥，含控制碼字串數 `392 → 0`；截圖底部正確顯示「玩家」「命中!」「匕首 攻擊」「未命中!」「北/方向」「通過」。

> 殘留小議題（非亂碼、不影響可讀性）：底部滾動區由多次 `NewPrint` 片段拼接，截圖偶爾捕捉到拼接中的瞬時重疊；屬版面細節，後續再調。

## 世界地圖城鎮位置（MAPS 420/PRTY 500）

黨伍起始位置：(x=20, y=42)（Sosaria 64×64 地圖）

| Tile 值 | 類型 | 已知位置 |
|---|---|---|
| 0x14 (20) | 城鎮 | (57,6)(47,7)(10,28)(59,30)(50,34)(59,44)**(20,57)** |
| 0x18 (24) | 地城 | (31,2)(7,13)(35,16)(47,19)(19,31)(57,31)**(8,44)**(48,58)(50,58) |
| 0x1c (28) | 特殊 | (46,18)(11,53) |

最近城鎮為 (20,57)（直線 15 格）；但周圍被山脈/水域包圍，需船隻或特殊移動才能進入。
地城 (8,44) 位於島嶼上，同樣需要船隻。進入城鎮/地城的測試需船隻系統或其他交通方式支援。

## 關鍵修正（累積）

1. `plat_ui.c`：FormPartyDialog 移植期自動組隊
2. `plat_event.c`：U<state>/M<keycode> 腳本指令
3. `plat_resource.c`：MAPS/MONS 419 fallback
4. `main.c` + `build_game.sh`：crash backtrace + -rdynamic
5. `qd_text.c`：pascal_to_c 過濾控制字元 0x00–0x1F
5b. **`UltimaText.c:366`：移除 `& 0x7F`（第四輪）—— 底部文字中文 UTF-8 被砍高位元的根因**
6. `build_and_verify.sh`：PASS/FAIL + 截圖數最小值
7. `.gitignore` + `git rm --cached`：shots 清除
8. `tests/scripts/play.txt`：擴充世界移動腳本

## 驗證指令

```bash
docker run --rm --user "$(id -u):$(id -g)" \
  -v /home/anr2/u3-cht:/work \
  u3cht bash /work/u3-cht/tools/build_and_verify.sh tests/scripts/play.txt 70 20
```

實測輸出（第三輪，2026-06-03）:

```text
[bv] 建置 OK
[bv] 跑 tests/scripts/play.txt (timeout 70s)
[U3] MainResources.rsrc 載入:303 個資源
[bv] rc=124  截圖數: 597  [bv] PASS
```

## 戰鬥系統驗證（第三輪）

- 遊戲在多怪物（7+ 個同時在螢幕）的高強度戰鬥中保持穩定，120s PASS。
- HP 隨戰鬥下降，角色有「Dead」狀態（HP=0 時 Status → 'D'）。
- `CheckAllDead` 函數：只要有一名成員存活（Status='G' or 'P'），不觸發全員死亡流程。
- 到達地城 (19,31) 的路徑已確認存在（北6→東2→北5→西3），但途中怪物戰鬥使移動延遲，120s 內未完成路徑。

## 仍待後續完善

- **進入城鎮/地城**：路徑已規劃（地城 (19,31)），需在怪物包圍下維持移動；城鎮被山脈/水域包圍需船隻。
- **底部文字 tile code**：需 `textPort` bitmap 或 tile 索引映射表才能正確渲染。
- FormPartyDialog 是移植期自動組隊，非正式 SDL 對話框 UI。
- `WriteResource` 仍是 no-op：角色/隊伍/地圖的持久化未完整。
- 角色建立、正式存檔/讀檔、更多遊戲命令仍需逐項補齊。
