#!/bin/bash
# run_tests.sh — 在 docker 容器內以乾淨 build 目錄跑 cmake 單元測試 (ctest)。
# 宿主常缺 SDL2 pkg-config,且 build/ 可能殘留容器路徑,故一律用容器 + 獨立 build 目錄,
# 避免污染既有 build/ (那是 build_game.sh 的產物)。
#
# 用法 (host):
#   docker run --rm -v /home/anr2/u3-cht:/work u3cht:latest bash /work/u3-cht/tools/run_tests.sh
# 或本機已裝 SDL2 時直接:bash tools/run_tests.sh
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$REPO/build_tests"
# 在所有離開路徑 (含失敗) 移除暫時 build 目錄,並把它 chown 回宿主 (若以 root 容器跑且
# 傳了 HOSTUID),避免留下宿主無法清理的 root-owned 殘檔。
cleanup() {
  [ -n "${HOSTUID:-}" ] && chown -R "${HOSTUID}:${HOSTGID:-$HOSTUID}" "$BUILD" 2>/dev/null || true
  rm -rf "$BUILD" 2>/dev/null || true
}
trap cleanup EXIT
rm -rf "$BUILD"
echo "[tests] cmake configure ..."
cmake -S "$REPO" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/tmp/u3-cmake.log 2>&1 || { tail -20 /tmp/u3-cmake.log; exit 1; }
echo "[tests] build ..."
cmake --build "$BUILD" -j"$(nproc)" >>/tmp/u3-cmake.log 2>&1 || { tail -30 /tmp/u3-cmake.log; exit 1; }
echo "[tests] ctest ..."
ctest --test-dir "$BUILD" --output-on-failure
rc=$?
rm -rf "$BUILD"
echo "[tests] 完成 rc=$rc"
exit $rc
