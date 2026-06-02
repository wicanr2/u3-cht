#!/bin/bash
# game_tester.sh — 在背景 (Xvfb) 跑遊戲並週期截圖,供自動驗證。
#
# 用法 (容器內):
#   tools/game_tester.sh <binary> [秒數] [腳本檔]
# 例:
#   tools/game_tester.sh build/u3 20 tests/scripts/newgame.txt
#
# 產出:tests/shots/frame_NNNNN.png(由遊戲自身 present hook 輸出)。
# 環境變數見 src/platform_sdl/main.c。
set -u

BIN="${1:-build/u3}"
SECS="${2:-20}"
SCRIPT="${3:-}"
SHOT_DIR="${U3_SHOT_DIR:-tests/shots}"

mkdir -p "$SHOT_DIR"
rm -f "$SHOT_DIR"/frame_*.png 2>/dev/null

export U3_SHOT_DIR="$SHOT_DIR"
export U3_SHOT_EVERY="${U3_SHOT_EVERY:-10}"
export U3_MAX_FRAMES="${U3_MAX_FRAMES:-600}"
export U3_LANG="${U3_LANG:-zh-Hant}"
export XDG_RUNTIME_DIR=/tmp
[ -n "$SCRIPT" ] && export U3_SCRIPT="$SCRIPT"

echo "[game_tester] 執行 $BIN (上限 ${SECS}s) script=${SCRIPT:-無}"
timeout "${SECS}" xvfb-run -a "$BIN"
rc=$?
echo "[game_tester] 結束 rc=$rc"
echo "[game_tester] 截圖:"
ls -1 "$SHOT_DIR"/frame_*.png 2>/dev/null | tail -5
echo "[game_tester] 共 $(ls -1 "$SHOT_DIR"/frame_*.png 2>/dev/null | wc -l) 張"
# timeout 回 124 視為正常 (背景跑滿時間)
[ $rc -eq 124 ] && exit 0 || exit $rc
