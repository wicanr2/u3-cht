#!/bin/bash
# smoke_appimage.sh — release QA:從「任意 cwd」跑 dist/Ultima3-CHT-x86_64.AppImage,
# 以 U3_SKIPINTRO 直達主選單,腳本驅動 編組→啟程世界,固定截圖並判斷關鍵畫面。
#
# 用法 (可在 repo 外執行,例如 cd /tmp 後):
#   /home/anr2/u3-cht/u3-cht/tools/smoke_appimage.sh [AppImage路徑]
# 退出碼:0=通過 (抵達主選單 + 世界/隊伍 sidebar);非 0=失敗。
#
# 關鍵設計:所有路徑都解析為絕對路徑 (腳本所在位置回推 repo),
# 故與當前工作目錄無關;AppImage 自身會 cd 到內部 data 目錄,不影響截圖 (絕對路徑)。
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(dirname "$SCRIPT_DIR")"
APPIMAGE="${1:-$REPO/dist/Ultima3-CHT-x86_64.AppImage}"
SMOKE="$REPO/tests/scripts/smoke.txt"

if [ ! -x "$APPIMAGE" ]; then
  echo "[smoke] 找不到可執行 AppImage:$APPIMAGE"; exit 2
fi
if [ ! -f "$SMOKE" ]; then
  echo "[smoke] 找不到 smoke 腳本:$SMOKE"; exit 2
fi

SHOT_DIR="$(mktemp -d /tmp/u3-appsmoke.XXXXXX)"
LOG="$(mktemp /tmp/u3-appsmoke-log.XXXXXX)"
SAVE="$(mktemp -u /tmp/u3-appsmoke-save.XXXXXX)"
PREFS="$(mktemp -u /tmp/u3-appsmoke-prefs.XXXXXX)"

export U3_SKIPINTRO=1 U3_LANG=zh-Hant U3_NOSAVE=1
export U3_TELEPORT="${U3_TELEPORT:-45,18}"   # 啟程後把隊伍置於城堡 tile,腳本按 E 進城堡
export U3_SCRIPT="$SMOKE"
export U3_SHOT_DIR="$SHOT_DIR" U3_SHOT_EVERY="${U3_SHOT_EVERY:-15}" U3_MAX_FRAMES="${U3_MAX_FRAMES:-640}"
export U3_DBG_LOC=1                       # 印 "[POS] Game entry"
export U3_DBG_SCENE=1                     # 印 "[SCENE] ..." 場景轉換標記 (全流程判據)
export U3_SAVE="$SAVE" U3_PREFS="$PREFS"
export XDG_RUNTIME_DIR=/tmp

echo "[smoke] cwd=$(pwd)  (蓄意不在 repo 內也應通過)"
echo "[smoke] AppImage=$APPIMAGE"
echo "[smoke] 截圖輸出=$SHOT_DIR"

# 蓄意切到 /tmp 證明與 cwd 無關;以 xvfb 提供無頭顯示,timeout 保底。
( cd /tmp && timeout "${SMOKE_SECS:-60}" xvfb-run -a "$APPIMAGE" ) >"$LOG" 2>&1
rc=$?

# grep -c 在 0 匹配時印 "0" 並以 1 退出;用 grep -q 判斷布林,避免 "|| echo 0" 產生雙行。
count_lines() { ls "$1"/frame_*.png 2>/dev/null | wc -l | tr -d ' '; }
has() { grep -q "$1" "$LOG" 2>/dev/null && echo 1 || echo 0; }
shots=$(count_lines "$SHOT_DIR")
audio_warn=$(has "Mix_OpenAudio 失敗\|音訊")
# 全流程場景判據 (來自 [SCENE]/[POS] 標記)
s_menu=$(has "\[SCENE\] where=5")
s_options=$(has "\[SCENE\] options-open")
s_organize=$(has "\[SCENE\] where=6")
s_world=$(has "\[POS\] Game entry")
s_castle=$(has "\[SCENE\].* castle")

echo "[smoke] rc=$rc 截圖=$shots 音訊警告=$audio_warn"
echo "[smoke] 場景:主選單=$s_menu 選項=$s_options 編組=$s_organize 世界=$s_world 城堡=$s_castle"
[ "$audio_warn" -eq 1 ] && echo "[smoke] (音訊無裝置屬正常警告,不影響判定)"

# 保留最後一張截圖路徑供人工檢視
last=$(ls "$SHOT_DIR"/frame_*.png 2>/dev/null | tail -1)
[ -n "$last" ] && echo "[smoke] 最後截圖:$last"

fail=0
# rc:124=timeout (正常持續執行);0=主動退出。其餘視為崩潰。
if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ]; then
  echo "[smoke] FAIL:AppImage 非正常退出 rc=$rc"; sed -n '1,20p' "$LOG"; fail=1
fi
if [ "$shots" -lt 10 ]; then
  echo "[smoke] FAIL:截圖數 $shots < 10,畫面未持續渲染"; fail=1
fi
# 9.5 標準:主選單/選項/編組/世界/城堡 五個場景皆須抵達。
for chk in "主選單:$s_menu" "選項:$s_options" "編組:$s_organize" "世界:$s_world" "城堡:$s_castle"; do
  name="${chk%%:*}"; val="${chk##*:}"
  if [ "$val" -ne 1 ]; then echo "[smoke] FAIL:未抵達場景「$name」"; fail=1; fi
done

if [ "$fail" -eq 0 ]; then
  echo "[smoke] PASS:主選單→選項→編組→世界→城堡 全流程可重現,截圖於 $SHOT_DIR"
  echo "$SHOT_DIR"
  exit 0
fi
echo "[smoke] 截圖目錄保留供除錯:$SHOT_DIR  log:$LOG"
exit 1
