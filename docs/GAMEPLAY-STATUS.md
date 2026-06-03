# 可玩性狀態與驗證紀錄

> 更新:2026-06-03（第六輪:城鎮/城堡進入驗證 + NPC 對話中文化管線）

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

## `\p` Pascal 字串字面值修正（第五輪，clang -fpascal-strings）

上游 243 處 Classic Mac `"\p..."` Pascal 字串字面值（`\p` 應由編譯器轉成長度前綴）：

- **GCC 不支援 `\p`**（也無 `-fpascal-strings`），會把 `\p` 當普通字元 `'p'` → `"\pTERRAFORM"` 變 C 字串 `"pTERRAFORM"`。傳給吃 `Str255` 的函式（`UPrint`/`UPrintWin`/`DrawString`…）時，首位元組 `'p'`(0x70=112) 被當長度 → 讀 112 byte rodata = 亂碼 blob（先前看到的 `Hit/Step/Ouch/DungeonShapes.jpg…` 即此）。
- **修正**：`docker/Dockerfile` 加 `clang`，`tools/build_game.sh` 改用 `clang -fpascal-strings` 編譯上游 10 檔（compat/plat 仍 gcc，混合連結）。clang 較嚴格，補 `-Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types*`（gcc `-w` 本就忽略）。`-fpascal-strings` 只影響 `\p` 字面值，CFSTR 音效/圖名與一般字串不變。
- **驗證**（U3_DBG_TEXT）：含 `.jpg/.png` 資源 blob 繪字 `20+ → 0`、無 `p` 前綴殘留；底部正確顯示「玩家/北/方向/通過/強盜/匕首 攻擊/命中!/未命中!/西」。

## 城鎮/城堡進入（#1，已驗證）

- 機制：在世界地圖站到已註冊地點（`LocationX/Y`）的 town/castle/dungeon tile 上，按 **`E`（Enter）** 進入子地圖（`UltimaMain.c:Enter()`）。tile：raw 0x14=地城、0x18=城鎮、0x1c=城堡（先前文件城鎮/地城標反，以程式為準）。
- 地點清單（runtime dump，起點 xpos=42/ypos=20）：城堡 (45,18)(10,53)；城鎮 (46,19)(34,16)(6,13)… 起點旁即有城堡 (45,18) 與城鎮 (46,19)。
- **驗證**：teleport 到城堡 (45,18) 按 E → 載入城堡子地圖、有 NPC、底部「城堡!」訊息。
- 測試工具（env-gated，patches/0002）：`U3_TELEPORT=x,y` 放置隊伍、`U3_DBG_LOC` dump 地點、`U3_DBG_SPEAK=N` 一次性呼叫 `Speak(N,23)`（NPC 會走動，腳本對位不可靠，故用此確定性鉤子驗證對話）。一般遊玩不受影響。
- 正常導航進城仍受地形限制（山脈/水域需船隻），屬內容/航行議題，非程式 bug。

## 航行 / 船隻導航（#3，移植層已驗證可用）

- 船隻系統為上游 gameplay,移植層編譯且運作:`Board`（按 `B`,`UltimaMain.c:1372`）站在 ship tile `0x2C` 上船 → `Party[1]=0x16`（frigate）→ 可於水上移動（`Party[1]==0x16` 才允許進入水/漩渦）。Board/上馬/啟航訊息（`UPrintMessage` 29-31）已在 `Messages.u3s` 翻譯。
- 地圖掃描:overworld（MAPS 420）**無**預置船;唯一可登船 ship tile 在城堡 #0 (9,23) 內。符合 U3 設計 —— 海域環繞的城鎮（如海之城 FAWN，resid 410/411）需透過遊戲進程取得船（海戰 / 城堡船）。
- **驗證**：`U3_TELEPORT=45,17`（水域）+ `U3_DBG_SHIP=1`（設 frigate）→ 隊伍以船型在水上自由移動（步行無法進入水域），截圖確認。
- 結論:**抵達海域城鎮是遊戲進程（取得船），非 localization 缺陷或移植 bug**;多數城鎮為陸路可達（#1 已驗證）。測試鉤子 `U3_DBG_SHIP` 見 patches/0002。

## NPC 對話中文化（#2，管線打通 + 樣本已驗證）

- NPC 對話**不在 `.u3s`**，在各地圖 `'TLKS'` 資源（原始英文 ASCII，NUL 分隔），`UltimaMisc.c:909` 載入 `Talk[256]`；`Speak(perNum, shnum)`（`UltimaText.c`）渲染第 `perNum` 段。`LoadUltimaMap` 設 `gCurMapID = resid`（城堡 #0 = 400）。
- **實作（patches/0001 的 Speak 鉤子）**：`Speak` 開頭以 `(gCurMapID, perNum)` 算複合索引 `(resid-400)*64+perNum` 查外部 `Talk` 表（`Strings_GetUTF8`）；命中即把中文 UTF-8 塞入 `Str255` → `RewrapString` → `UPrint`（走已修好的 line 366 路徑）並返回；未譯則回退原始 `Talk[]` 英文。**故不需動 line 711**（僅影響英文 fallback，`& 0x7F` 對 ASCII 無害），避免未驗證風險。
- **資料管線**：`translations/Talk.json`（`{map, npc, en, zh}` 記錄）→ `tools/build_talk.py` → `assets/strings/<lang>/Talk.u3s`（複合索引，未譯槽空字串→回退）。
- **驗證（pristine 上游 → 自動套 2 patch → 實機）**：城堡 #0 NPC#1 原文 `"WEST-8, SOUTH-35, AND AWAIT DAWN!"` → 譯「向西8，向南35，等待黎明！」，截圖底部正確顯示中文。
- **待辦（純內容）**：抽取各地圖 TLKS 原文 + 翻譯填入 `Talk.json`（目前 1 句樣本）。長對話若超出 `Str255` 需分段，屆時再處理。原始 `.ULT/TLKS` 不動（避免破壞 offset）。

## 仍待後續完善

- **NPC 對話全量翻譯**：管線已通（見上節），剩抽取各地圖 TLKS 原文 + 翻譯填 `Talk.json`（目前僅 1 句樣本）。
- **正常導航進城**：受地形限制（山脈/水域需船隻），屬航行系統，非中文化範圍。
- FormPartyDialog 是移植期自動組隊，非正式 SDL 對話框 UI。
- `WriteResource` 仍是 no-op：角色/隊伍/地圖的持久化未完整。
- 角色建立、正式存檔/讀檔、更多遊戲命令仍需逐項補齊。
