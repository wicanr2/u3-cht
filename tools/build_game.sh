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
# 上游 ultima3/ 為 git clone;優先用 git apply (較穩健,reverse-check 做 idempotent;
# GNU patch 對部分檔案的 hunk 比對較挑剔)。非 git 或無 git 時回退 patch。
UPDIR="$SRC/.."
if [ -d "$PATCHES" ]; then
  for pf in "$PATCHES"/*.patch; do
    [ -e "$pf" ] || continue
    bn=$(basename "$pf")
    if [ -d "$UPDIR/.git" ] && command -v git >/dev/null 2>&1; then
      if git -C "$UPDIR" apply --reverse --check "$pf" 2>/dev/null; then
        echo "已套用,略過: $bn"
      elif git -C "$UPDIR" apply "$pf" 2>/dev/null; then
        echo "套用 patch: $bn"
      else
        echo "⚠ patch 套用失敗: $bn"; exit 1
      fi
    else
      if patch -p1 -d "$UPDIR" -N --dry-run <"$pf" >/dev/null 2>&1; then
        patch -p1 -d "$UPDIR" -N <"$pf" >/dev/null && echo "套用 patch: $bn"
      else
        echo "已套用或無法套用,略過: $bn"
      fi
    fi
  done
fi

CFLAGS="-std=gnu11 -w -O2 -g -include $COMPAT/mac_shim.h -I $COMPAT/fakeinc -I $COMPAT -I $TEXT -I $PLAT -I $SRC $(sdl2-config --cflags)"

# 上游檔以 clang -fpascal-strings 編譯:GCC 不支援 Classic Mac 的 "\p..." Pascal 字串
# 字面值 (243 處),否則 \p 被當普通字元 'p' → 首位元組 0x70 被當長度 → 讀 112 byte 亂碼。
# clang 較嚴格,需抑制上游大量隱式宣告/int↔ptr 轉換 (gcc -w 本就忽略)。
GAME_CFLAGS="$CFLAGS -fpascal-strings -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-incompatible-pointer-types-discards-qualifiers"
GAME="UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText"
for f in $GAME; do clang $GAME_CFLAGS -c $SRC/$f.c -o $OUT/$f.o; done
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
