#!/bin/bash
# package_windows.sh — 交叉編譯 Windows x64 版並打包成 zip (含 DLL + 啟動腳本 + 遊戲資料)。
# 用 clang --target=x86_64-w64-mingw32 (clang 支援 -fpascal-strings,GCC 不支援) + mingw CRT
# + libsdl.org 的 SDL2 mingw 開發庫。
#
# 用法 (host):
#   docker run --rm -e HOSTUID=$(id -u) -e HOSTGID=$(id -g) \
#     -v /home/anr2/u3-cht:/work ubuntu:24.04 bash /work/u3-cht/tools/package_windows.sh
set -eu
export DEBIAN_FRONTEND=noninteractive
REPO=/work/u3-cht
SRC=/work/ultima3/Sources
COMPAT=$REPO/src/compat; TEXT=$REPO/src/text; PLAT=$REPO/src/platform_sdl
DIST=$REPO/dist
SDL=/tmp/sdl; OBJ=/tmp/winobj; STAGE=/tmp/win/Ultima3-CHT
TRIPLE=x86_64-w64-mingw32

echo "[win] apt:clang lld mingw CRT ..."
apt-get update -qq >/dev/null
# gcc-mingw-w64-x86-64 提供 mingw 版 libgcc.a/libgcc_eh.a (clang mingw 連結需要)
apt-get install -y -qq clang lld binutils-mingw-w64-x86-64 mingw-w64-x86-64-dev \
    gcc-mingw-w64-x86-64 git wget tar xz-utils zip ca-certificates >/dev/null

echo "[win] 下載 SDL2 mingw 開發庫 ..."
mkdir -p "$SDL"; cd "$SDL"
dl() { wget -q "$1" -O "$2" || { echo "下載失敗 $1"; exit 1; }; }
dl https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-devel-2.30.9-mingw.tar.gz s.tgz
dl https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-devel-2.8.2-mingw.tar.gz si.tgz
dl https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-devel-2.22.0-mingw.tar.gz st.tgz
dl https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-devel-2.8.0-mingw.tar.gz sm.tgz
for f in *.tgz; do tar xzf "$f"; done
# 各庫的 x86_64-w64-mingw32 子目錄 (include/SDL2, lib, bin)
SDLROOTS=$(ls -d "$SDL"/SDL2*/$TRIPLE)
INCS=""; LIBS=""
for r in $SDLROOTS; do INCS="$INCS -I$r/include -I$r/include/SDL2"; LIBS="$LIBS -L$r/lib"; done

echo "[win] 套用中文化 patch (git apply) ..."
if [ -d "$SRC/../.git" ]; then
  for pf in "$REPO"/patches/*.patch; do
    git -C "$SRC/.." apply --reverse --check "$pf" 2>/dev/null && continue
    git -C "$SRC/.." apply "$pf" 2>/dev/null || echo "  patch 略過 $(basename "$pf")"
  done
fi

echo "[win] 交叉編譯 ..."
rm -rf "$OBJ"; mkdir -p "$OBJ"
CC="clang --target=$TRIPLE -fuse-ld=lld"
CF="-std=gnu11 -w -O2 -include $COMPAT/mac_shim.h -I $COMPAT/fakeinc -I $COMPAT -I $TEXT -I $PLAT -I $SRC $INCS -DWIN32 -D_WIN32"
GAME_SUPP="-fpascal-strings -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-incompatible-pointer-types-discards-qualifiers"
GAME="UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText"
for f in $GAME; do $CC $CF $GAME_SUPP -c "$SRC/$f.c" -o "$OBJ/$f.o" || { echo "編譯失敗 $f"; exit 1; }; done
$TRIPLE-objcopy --weaken-symbol=UDrawThemePascalString --weaken-symbol=UThemePascalStringWidth "$OBJ/UltimaText.o" 2>/dev/null || true
for f in $COMPAT/qd_geometry $COMPAT/qd_port $COMPAT/qd_draw $COMPAT/qd_text $COMPAT/cf_bridge $TEXT/strings; do
  $CC $CF -c "$f.c" -o "$OBJ/$(basename "$f").o" || { echo "編譯失敗 $f"; exit 1; }
done
for p in "$PLAT"/*.c; do $CC $CF -c "$p" -o "$OBJ/$(basename "${p%.c}").o" || { echo "編譯失敗 $p"; exit 1; }; done

echo "[win] 連結 u3.exe ..."
$CC $OBJ/*.o -o "$OBJ/u3.exe" $LIBS \
    -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -mwindows || { echo "連結失敗"; exit 1; }
file "$OBJ/u3.exe" 2>/dev/null || echo "  (u3.exe 已連結)"

echo "[win] 組 zip (exe + DLL + 資料 + 字型 + 啟動腳本) ..."
rm -rf "$STAGE"; mkdir -p "$STAGE"
cp "$OBJ/u3.exe" "$STAGE/"
for r in $SDLROOTS; do cp "$r"/bin/*.dll "$STAGE/" 2>/dev/null || true; done
cp -r "$REPO/assets" "$STAGE/assets"
mkdir -p "$STAGE/assets/fonts"
apt-get install -y -qq fonts-noto-cjk >/dev/null 2>&1 || true
cp /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc "$STAGE/assets/fonts/U3Font.ttc" 2>/dev/null || true
# 啟動腳本 (.bat):設字型與語言,就地執行
printf '@echo off\r\nset U3_LANG=zh-Hant\r\nset U3_FONT=assets\\fonts\\U3Font.ttc\r\ncd /d "%%~dp0"\r\nstart "" u3.exe\r\n' > "$STAGE/玩遊戲.bat"
echo "DLL: $(ls "$STAGE"/*.dll | wc -l) 個"
mkdir -p "$DIST"; cd /tmp/win
zip -qr "$DIST/Ultima3-CHT-windows-x64.zip" Ultima3-CHT
chown -R "${HOSTUID:-1000}:${HOSTGID:-1000}" "$DIST" /work/ultima3/Sources 2>/dev/null || true
echo "[win] 完成:$(ls -lh "$DIST"/*.zip | awk '{print $5,$9}')"
