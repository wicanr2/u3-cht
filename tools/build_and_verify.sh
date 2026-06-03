#!/bin/bash
# build_and_verify.sh — 容器內:重建 build/u3 後跑輸入腳本並截圖。一律帶 timeout。
# 用法:bash tools/build_and_verify.sh <script相對路徑> [秒數] [shot_every]
set -uo pipefail
SCRIPT="${1:-tests/scripts/play.txt}"
SECS="${2:-24}"
EVERY="${3:-20}"
echo "[bv] 重建 build/u3 ..."
bash /work/u3-cht/tools/build_game.sh >/tmp/build.log 2>&1 || { echo "建置失敗:"; tail -20 /tmp/build.log; exit 1; }
echo "[bv] 建置 OK"
export XDG_RUNTIME_DIR=/tmp U3_LANG=zh-Hant
# 測試保持確定性:關閉 autosave 並清除舊存檔,避免跨 run 狀態污染。
export U3_NOSAVE=1
# 略過開場動畫直達主選單,使驗證不受 ThreadSleepTicks 動畫時長影響 (可用 U3_SKIPINTRO=0 還原)。
export U3_SKIPINTRO="${U3_SKIPINTRO:-1}"
rm -f /work/u3-cht/u3save.dat /work/u3-cht/u3prefs.txt
export U3_SHOT_DIR=/work/u3-cht/tests/shots U3_SHOT_EVERY="$EVERY"
export U3_SCRIPT="/work/u3-cht/$SCRIPT"
cd /work/u3-cht
rm -f tests/shots/frame_*.png
log=/tmp/u3-run.log
echo "[bv] 跑 $SCRIPT (timeout ${SECS}s)"
set +e
timeout "$SECS" xvfb-run -a ./build/u3 >"$log" 2>&1
rc=$?
set -e
grep -ivE "^$|XIO|xvfb-run|after [0-9]+ requests" "$log" | tail -10 || true
echo "[bv] rc=$rc"
shots=$(ls tests/shots/frame_*.png 2>/dev/null | wc -l)
echo "[bv] 截圖數: $shots"
ls tests/shots/frame_*.png 2>/dev/null | tail -3
# rc=124:timeout 殺掉(正常:遊戲持續運行)。rc=0:遊戲主動正常退出(少見,視為通過)。
# rc=1..123:崩潰或錯誤退出,回傳失敗。
if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ]; then
  echo "[bv] FAIL: 非正常退出 rc=$rc"
  exit "$rc"
fi
# 截圖數過少表示遊戲在驗證期間過早退出或未能渲染。
if [ "$shots" -lt 20 ]; then
  echo "[bv] FAIL: 截圖數 $shots < 20,遊戲未能持續運行"
  exit 1
fi
echo "[bv] PASS"
