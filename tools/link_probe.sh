#!/bin/bash
# link_probe.sh — 編譯 9 個可編檔 + compat + text,連結並列出 undefined references。
# 產出平台層 (取代 MacIF/Cocoa/AppleEvents) 必須實作的符號 backlog。
set -u
SRC=/work/ultima3/Sources
COMPAT=/work/u3-cht/src/compat
OUT=/tmp/u3obj
mkdir -p $OUT

CFLAGS="-std=gnu11 -w -O0 -g -include $COMPAT/mac_shim.h -I $COMPAT/fakeinc -I $COMPAT -I /work/u3-cht/src/text -I $SRC $(sdl2-config --cflags)"

# 9 個可編譯的上游檔 (排除 MacIF/AppleEvents 平台層)
GAME="UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText"

echo "== 編譯上游遊戲檔 =="
for f in $GAME; do
  gcc $CFLAGS -c $SRC/$f.c -o $OUT/$f.o 2>/dev/null || echo "  編譯失敗: $f"
done

echo "== 編譯 compat + text =="
gcc $CFLAGS -c $COMPAT/qd_geometry.c -o $OUT/qd_geometry.o
gcc $CFLAGS -c $COMPAT/qd_port.c     -o $OUT/qd_port.o
gcc $CFLAGS -c $COMPAT/qd_draw.c     -o $OUT/qd_draw.o
gcc $CFLAGS -c $COMPAT/qd_text.c     -o $OUT/qd_text.o
gcc $CFLAGS -c $COMPAT/cf_bridge.c   -o $OUT/cf_bridge.o
gcc $CFLAGS -c /work/u3-cht/src/text/strings.c -o $OUT/strings.o
# 平台層 (若存在)
for p in /work/u3-cht/src/platform_sdl/*.c; do
  [ -e "$p" ] || continue
  gcc $CFLAGS -c "$p" -o $OUT/$(basename ${p%.c}).o 2>/dev/null || echo "  平台層編譯失敗: $p"
done

echo "== 連結,列出 undefined references =="
gcc $OUT/*.o -o /tmp/u3_test $(sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lm 2>&1 \
  | grep -oE "undefined reference to .[A-Za-z_][A-Za-z0-9_]*." \
  | sed -E "s/.*reference to .([A-Za-z_][A-Za-z0-9_]*)./\1/" \
  | sort | uniq -c | sort -rn
echo "== 若上面無輸出且 /tmp/u3_test 存在 = 連結成功 =="
ls -la /tmp/u3_test 2>/dev/null && echo "LINK OK"
