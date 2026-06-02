# 可玩性狀態與驗證紀錄

> 更新:2026-06-03（第三輪）

## 目前已驗證

- 開場、標題、展示動畫、主選單可顯示繁體中文。
- 主選單可進入 **Organize a Party**。
- SDL 平台層已支援 `FormPartyDialog()` 的移植期自動組隊:選取名冊前 4 名有效角色。
- `Journey Onward` 已可通過 `Party[7]` 檢查並進入 Sosaria 世界地圖。
- game tester 已用 Docker / Xvfb 實機驗證:組隊後進世界,畫面右側顯示 4 名隊員。
- 世界地圖可長時間穩定移動（70s timeout PASS，597 截圖）。
- **戰鬥系統可用**:party 成員 HP 隨怪物攻擊下降，確認基礎戰鬥迴圈運作。
- 世界畫面底部文字滾動區控制字元亂碼已修正（過濾 0x00–0x1F）。
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

## 底部文字協定說明

底部文字滾動區使用 **Classic mode tile 索引**（非 ASCII）。Mac 原版以 `textPort` bitmap 渲染 tile 索引；SDL 移植改用 SDL_ttf，tile 索引被誤解為 ASCII，出現「e7+MITTAR」等含 tile code 前綴的文字。

- 純 ASCII 文字（如怪物名稱 "MITTAR"、戰鬥音效 "AIEEEEEE!"）可正確顯示。
- Tile code bytes（如 0x65='e', 0x37='7'）無法在 SDL_ttf 層區分，是已知限制。
- 修正方向：需為 `textPort` 填入正確字型 bitmap，啟用 Classic mode；或解析 tile 索引協定建立映射表。

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

## 仍待後續完善

- **進入城鎮/地城**：需船隻或其他交通工具；地圖地形限制（Sosaria 各城鎮被山脈/水域包圍）。
- **底部文字 tile code**：需 `textPort` bitmap 或 tile 索引映射表才能正確渲染。
- FormPartyDialog 是移植期自動組隊，非正式 SDL 對話框 UI。
- `WriteResource` 仍是 no-op：角色/隊伍/地圖的持久化未完整。
- 角色建立、正式存檔/讀檔、更多遊戲命令仍需逐項補齊。
