#!/bin/bash
# build_game.sh — 編譯完整遊戲 (上游 10 檔 + compat + text + SDL 平台層) → build/u3
# 容器內執行:docker run ... u3cht bash /work/u3-cht/tools/build_game.sh
set -eu
SRC=/work/ultima3/Sources
COMPAT=/work/u3-cht/src/compat
TEXT=/work/u3-cht/src/text
PLAT=/work/u3-cht/src/platform_sdl
OUT=/work/u3-cht/build/game_obj
BIN=/work/u3-cht/build/u3
PATCHES=/work/u3-cht/patches
mkdir -p $OUT

# 套用對上游唯讀源碼的中文化修正 (patches/*.patch)。
# 上游 ultima3/ 預設唯讀,修正以 patch 納入 repo;idempotent 套用 (已套用則略過)。
if [ -d "$PATCHES" ]; then
  for pf in "$PATCHES"/*.patch; do
    [ -e "$pf" ] || continue
    if patch -p1 -d "$SRC/.." -N --dry-run <"$pf" >/dev/null 2>&1; then
      patch -p1 -d "$SRC/.." -N <"$pf" >/dev/null && echo "套用 patch: $(basename "$pf")"
    else
      echo "patch 已套用或無法套用，略過: $(basename "$pf")"
    fi
  done
fi

CFLAGS="-std=gnu11 -w -O2 -g -include $COMPAT/mac_shim.h -I $COMPAT/fakeinc -I $COMPAT -I $TEXT -I $PLAT -I $SRC $(sdl2-config --cflags)"

GAME="UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText"
for f in $GAME; do gcc $CFLAGS -c $SRC/$f.c -o $OUT/$f.o; done
# 弱化 UltimaText.c 自帶 Mac 版繪字函式,改用 compat/qd_text.c 的 SDL_ttf 中文版
objcopy --weaken-symbol=UDrawThemePascalString \
        --weaken-symbol=UThemePascalStringWidth $OUT/UltimaText.o

for f in $COMPAT/qd_geometry $COMPAT/qd_port $COMPAT/qd_draw $COMPAT/qd_text $COMPAT/cf_bridge $TEXT/strings; do
  gcc $CFLAGS -c $f.c -o $OUT/$(basename $f).o
done
for p in $PLAT/*.c; do gcc $CFLAGS -c "$p" -o $OUT/$(basename ${p%.c}).o; done

gcc -rdynamic $OUT/*.o -o $BIN $(sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lm
echo "建置完成: $BIN"
ls -la $BIN
