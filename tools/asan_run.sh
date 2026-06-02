#!/bin/bash
# asan_run.sh — ASan 建置 + headless 執行,崩潰時印 backtrace。除錯迭代用。
# 容器內:docker run ... u3cht bash /work/u3-cht/tools/asan_run.sh [秒數] [腳本檔]
set -eu
SECS="${1:-20}"; SCRIPT="${2:-}"
SRC=/work/ultima3/Sources; C=/work/u3-cht/src/compat; T=/work/u3-cht/src/text; P=/work/u3-cht/src/platform_sdl
O=/tmp/asan; mkdir -p $O
F="-std=gnu11 -w -O1 -g -fsanitize=address -fno-omit-frame-pointer -include $C/mac_shim.h -I $C/fakeinc -I $C -I $T -I $P -I $SRC $(sdl2-config --cflags)"
for f in UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText; do gcc $F -c $SRC/$f.c -o $O/$f.o; done
objcopy --weaken-symbol=UDrawThemePascalString --weaken-symbol=UThemePascalStringWidth $O/UltimaText.o
for f in $C/qd_geometry $C/qd_port $C/qd_draw $C/qd_text $C/cf_bridge $T/strings; do gcc $F -c $f.c -o $O/$(basename $f).o; done
for p in $P/*.c; do gcc $F -c "$p" -o $O/$(basename ${p%.c}).o; done
gcc -fsanitize=address $O/*.o -o /tmp/u3_asan $(sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lm
echo "=== run (${SECS}s) ==="
export XDG_RUNTIME_DIR=/tmp ASAN_OPTIONS=detect_leaks=0
export U3_SHOT_DIR=/work/u3-cht/tests/shots U3_SHOT_EVERY=${U3_SHOT_EVERY:-1} U3_LANG=zh-Hant
mkdir -p $U3_SHOT_DIR; rm -f $U3_SHOT_DIR/frame_*.png
[ -n "$SCRIPT" ] && export U3_SCRIPT="$SCRIPT"
cd /work/u3-cht
timeout "$SECS" xvfb-run -a /tmp/u3_asan 2>&1 | grep -vE "Reading symbols|no debugging" | head -40 || true
echo "EXIT=${PIPESTATUS[0]:-?}"
echo "截圖: $(ls -1 $U3_SHOT_DIR/frame_*.png 2>/dev/null | wc -l) 張"
