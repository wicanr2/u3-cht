#!/bin/bash
# compile_probe.sh — 對上游 .c 做 -fsyntax-only 探測,列出對 compat 層的缺漏。
# 用法 (容器內):tools/compile_probe.sh UltimaAutocombat.c [更多檔...]
set -u
SRC=/work/ultima3/Sources
COMPAT=/work/u3-cht/src/compat
FAKE=$COMPAT/fakeinc

FLAGS=(
  -fsyntax-only
  -std=gnu11
  -w                     # 探測階段先靜音警告,只看 error
  -include "$COMPAT/mac_shim.h"
  -I "$FAKE"
  -I "$COMPAT"
  -I /work/u3-cht/src/text
  -I "$SRC"
  $(sdl2-config --cflags)
)

rc=0
for f in "$@"; do
  echo "==== probe: $f ===="
  if gcc "${FLAGS[@]}" -c "$SRC/$f" 2>&1 | sed 's#/work/##' | head -40; then
    echo "  (無 error 或已截斷)"
  fi
  # 真正回傳碼
  gcc "${FLAGS[@]}" -c "$SRC/$f" >/dev/null 2>&1 || rc=1
done
exit $rc
