#!/bin/bash
# package_macos.sh — 在 macOS runner 上把 Ultima III 中文版建成 Universal .app + .dmg。
#
# 為什麼一定要在 macOS host 上跑:Linux 無法可靠跨編 Mach-O;codesign/hdiutil/lipo 只在 macOS。
# 走 GitHub Actions macos-14 runner(Apple Silicon)。整條 pipeline:
#   1) 從「源碼」編 Universal(arm64+x86_64)SDL2 / SDL2_image / SDL2_ttf / SDL2_mixer
#      ── [HARD] 絕不 brew install sdl2:2026 起 brew 的 sdl2 是架在 SDL3 上的 shim,
#         dylibbundler 不會把 runtime dlopen 的 libSDL3 打包進 .app → 玩家端「Failed loading
#         SDL3 library」黑畫面。源碼編的才是真 SDL2。
#   2) clang -arch arm64 -arch x86_64 一趟編出 fat 遊戲 binary(與 build_game.sh 同旗標;
#      這個 port 把 macOS 當純 POSIX+SDL 目標,靠 -include mac_shim.h + -I fakeinc 攔截真實
#      Carbon/QuickDraw SDK,完全不連 Apple 原生框架)。
#   3) 組 Ultima3-CHT.app:launch wrapper(cd 到 Resources)+ assets + Noto CJK 字型。
#      binary 內建 auto-chdir + U3_FONT fallback,故 wrapper 極簡、字型走相對路徑 fallback。
#   4) dylibbundler 把自編 SDL dylib 收進 .app(自編 dylib 的 install name 是 @rpath →
#      dylibbundler 需 -s "$PREFIX/lib" 才解得到,否則互動式無限 hang;</dev/null 當保險絲)。
#   5) lipo/otool 雙弧 + 非-SDL3 防呆斷言 → ad-hoc 簽 → zip + dmg。
#
# 用法(runner):bash tools/package_macos.sh
#   需要:$GITHUB_WORKSPACE 下有本 repo,且上游已 clone 於 $UPSTREAM(預設 ../ultima3)。
set -euo pipefail

ts() { date +%H:%M:%S; }   # 每階段時間戳:CI hang 時可從 job log 逐階段排除定位(見 kb §1.2d)

REPO="${GITHUB_WORKSPACE:-$(cd "$(dirname "$0")/.." && pwd)}"
UPSTREAM="${UPSTREAM:-$REPO/../ultima3}"
SRC="$UPSTREAM/Sources"
COMPAT="$REPO/src/compat"; TEXT="$REPO/src/text"; PLAT="$REPO/src/platform_sdl"
DIST="$REPO/dist"; PREFIX=/tmp/sdlprefix; OBJ=/tmp/u3obj
ARCHS_CMAKE="arm64;x86_64"; MIN=11.0
export MACOSX_DEPLOYMENT_TARGET="$MIN"

SDL_VER=2.30.9; IMG_VER=2.8.2; TTF_VER=2.22.0; MIX_VER=2.8.0

mkdir -p "$PREFIX" "$OBJ" "$DIST"

# ---------------------------------------------------------------------------
# 1) 從源碼編 Universal SDL 系列(release 源碼 tarball 自帶 vendored 相依,免 submodule）
# ---------------------------------------------------------------------------
build_sdl() {
  local name="$1" ver="$2" url="$3"; shift 3
  echo "[$(ts)] SDL::config $name-$ver"
  local d="/tmp/$name-$ver"
  rm -rf "$d"; mkdir -p "$d"
  curl -fsSL "$url" | tar xz -C /tmp
  cmake -S "$d" -B "$d/_b" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHS_CMAKE" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_SHARED_LIBS=ON \
    "$@"
  echo "[$(ts)] SDL::build $name"
  cmake --build "$d/_b" --parallel "$(sysctl -n hw.ncpu)"
  cmake --install "$d/_b"
  echo "[$(ts)] SDL::OK $name"
}

R=https://github.com/libsdl-org
build_sdl SDL2       "$SDL_VER" "$R/SDL/releases/download/release-$SDL_VER/SDL2-$SDL_VER.tar.gz"

# SDL2_image:STB backend 提供 PNG+JPG(免外部 libpng/libjpeg);GIF 為內建 loader。
# 資產含 .png/.jpg/.gif → 這三個涵蓋齊。
build_sdl SDL2_image "$IMG_VER" "$R/SDL_image/releases/download/release-$IMG_VER/SDL2_image-$IMG_VER.tar.gz" \
  -DSDL2IMAGE_BACKEND_STB=ON -DSDL2IMAGE_PNG=ON -DSDL2IMAGE_JPG=ON -DSDL2IMAGE_GIF=ON \
  -DSDL2IMAGE_AVIF=OFF -DSDL2IMAGE_JXL=OFF -DSDL2IMAGE_WEBP=OFF -DSDL2IMAGE_TIF=OFF \
  -DSDL2IMAGE_SAMPLES=OFF -DSDL2IMAGE_VENDORED=ON

# SDL2_ttf:vendored freetype(CJK 渲染唯一真相依);harfbuzz 關掉(基本 CJK 不需要）。
build_sdl SDL2_ttf   "$TTF_VER" "$R/SDL_ttf/releases/download/release-$TTF_VER/SDL2_ttf-$TTF_VER.tar.gz" \
  -DSDL2TTF_VENDORED=ON -DSDL2TTF_HARFBUZZ=OFF -DSDL2TTF_SAMPLES=OFF

# SDL2_mixer:WAV 內建(音效);OGG 用內建 stb_vorbis(音樂,免外部 libvorbis)。其餘 codec 全關。
build_sdl SDL2_mixer "$MIX_VER" "$R/SDL_mixer/releases/download/release-$MIX_VER/SDL2_mixer-$MIX_VER.tar.gz" \
  -DSDL2MIXER_VORBIS=STB -DSDL2MIXER_MP3=OFF -DSDL2MIXER_MIDI=OFF -DSDL2MIXER_FLAC=OFF \
  -DSDL2MIXER_MOD=OFF -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_WAVPACK=OFF -DSDL2MIXER_SAMPLES=OFF \
  -DSDL2MIXER_VENDORED=ON

# 防呆:確認裝出來的是「真 SDL2」非 sdl2-compat shim(shim 會 dlopen SDL3)。
if otool -L "$PREFIX/lib/libSDL2-2.0.0.dylib" | grep -qi SDL3; then
  echo "✗ 自編 SDL2 竟參照 SDL3(疑似抓到 shim)"; exit 1
fi
echo "[$(ts)] SDL 全數就緒:$(ls "$PREFIX"/lib/libSDL2*.dylib | wc -l) 個"

# ---------------------------------------------------------------------------
# 2) 套中文化 patch + 用 clang 一趟編 Universal 遊戲 binary
# ---------------------------------------------------------------------------
echo "[$(ts)] 套用中文化 patch"
if [ -d "$UPSTREAM/.git" ]; then
  for pf in "$REPO"/patches/*.patch; do
    [ -e "$pf" ] || continue
    git -C "$UPSTREAM" apply --reverse --check "$pf" 2>/dev/null && continue
    git -C "$UPSTREAM" apply "$pf" 2>/dev/null || echo "  patch 略過 $(basename "$pf")"
  done
fi

ARCH_FLAGS="-arch arm64 -arch x86_64 -mmacosx-version-min=$MIN"
CF="-std=gnu11 -w -O2 $ARCH_FLAGS -include $COMPAT/mac_shim.h -I $COMPAT/fakeinc -I $COMPAT -I $TEXT -I $PLAT -I $SRC -I $PREFIX/include -I $PREFIX/include/SDL2"
# 上游檔以 -fpascal-strings 編(Classic Mac "\p" 字面值,243 處);抑制上游隱式宣告/int↔ptr。
GAME_SUPP="-fpascal-strings -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-incompatible-pointer-types-discards-qualifiers"
GAME="UltimaAutocombat UltimaDngn UltimaGraphics UltimaMacIF UltimaMain UltimaMisc UltimaNew UltimaNewMap UltimaSpellCombat UltimaText"

echo "[$(ts)] 編譯上游遊戲邏輯(fat)"
for f in $GAME; do clang $CF $GAME_SUPP -c "$SRC/$f.c" -o "$OBJ/$f.o"; done
# 註:繪字漏斗 UDrawThemePascalString/UThemePascalStringWidth 僅定義於 compat/qd_text.c,
# UltimaText.c 不含其定義,故 macOS 無重複符號、免 objcopy weaken(Linux 那步是防禦性 no-op)。

echo "[$(ts)] 編譯 compat / text / 平台層(fat)"
for f in $COMPAT/qd_geometry $COMPAT/qd_port $COMPAT/qd_draw $COMPAT/qd_text $COMPAT/cf_bridge $TEXT/strings; do
  clang $CF -c "$f.c" -o "$OBJ/$(basename "$f").o"
done
for p in "$PLAT"/*.c; do clang $CF -c "$p" -o "$OBJ/$(basename "${p%.c}").o"; done

echo "[$(ts)] 連結 u3(fat)"
clang $ARCH_FLAGS $OBJ/*.o -o "$OBJ/u3" \
  -L"$PREFIX/lib" -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm
lipo -info "$OBJ/u3"

# ---------------------------------------------------------------------------
# 3) 組 Ultima3-CHT.app
# ---------------------------------------------------------------------------
echo "[$(ts)] 組 .app"
APP="$DIST/Ultima3-CHT.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$APP/Contents/Frameworks"
cp "$OBJ/u3" "$APP/Contents/MacOS/u3"

# 遊戲資料(完整自含)+ Noto Sans CJK 繁中字型(mac 無 fonts-noto-cjk,線上取)。
cp -R "$REPO/assets" "$APP/Contents/Resources/assets"
mkdir -p "$APP/Contents/Resources/assets/fonts"
echo "[$(ts)] 取 Noto Sans CJK TC 字型"
FONT_OUT="$APP/Contents/Resources/assets/fonts/U3Font.ttc"
curl -fsSL -A "Mozilla/5.0" -o "$FONT_OUT" \
  "https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf" \
  || curl -fsSL -A "Mozilla/5.0" -o "$FONT_OUT" \
  "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf"
test -s "$FONT_OUT"

# launch wrapper:雙擊 .app 時 cwd 非 Resources,先 cd 過去(binary 以 cwd 找 assets/)。
# 之後 binary 的 U3_FONT fallback 會用 assets/fonts/U3Font.ttc(相對 cwd)→ 免設環境變數。
cat > "$APP/Contents/MacOS/launch" <<'LAUNCH'
#!/bin/bash
# $0 雙擊時為絕對、終端 ./launch 為相對;cd 前先解成絕對路徑,否則 dirname 會指錯。
HERE="$(cd "$(dirname "$0")" && pwd)"     # Contents/MacOS
cd "$HERE/../Resources"                    # cwd = Resources(含 assets/)
export U3_LANG="${U3_LANG:-zh-Hant}"       # 字形切換仍可用 U3_TILESET / 覆寫 U3_FONT
exec "$HERE/u3" "$@"
LAUNCH
chmod +x "$APP/Contents/MacOS/launch" "$APP/Contents/MacOS/u3"

cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Ultima3-CHT</string>
  <key>CFBundleDisplayName</key><string>Ultima III 中文版</string>
  <key>CFBundleExecutable</key><string>launch</string>
  <key>CFBundleIdentifier</key><string>tw.u3cht.ultima3</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST

# ---------------------------------------------------------------------------
# 4) dylibbundler:把自編 SDL dylib 收進 .app/Contents/Frameworks
# ---------------------------------------------------------------------------
echo "[$(ts)] dylibbundler"
dylibbundler -od -b \
  -x "$APP/Contents/MacOS/u3" \
  -d "$APP/Contents/Frameworks/" \
  -p "@executable_path/../Frameworks/" \
  -s "$PREFIX/lib" </dev/null

# ---------------------------------------------------------------------------
# 5) 防呆斷言 → ad-hoc 簽 → 打包
# ---------------------------------------------------------------------------
echo "[$(ts)] 驗證雙弧 + 無 SDL3 shim"
for b in "$APP/Contents/MacOS/u3" "$APP"/Contents/Frameworks/libSDL2-2.0.0.dylib; do
  info="$(lipo -info "$b")"; echo "  $info"
  echo "$info" | grep -q arm64 && echo "$info" | grep -q x86_64 || { echo "✗ $b 非雙弧"; exit 1; }
done
otool -L "$APP/Contents/MacOS/u3" | grep -qi SDL3 && { echo "✗ 竟連到 SDL3 shim"; exit 1; } || true
ls "$APP/Contents/Frameworks/" | grep -q "libSDL2_ttf" || { echo "✗ Frameworks 缺 SDL2_ttf"; exit 1; }

echo "[$(ts)] ad-hoc 簽章"
codesign --force --deep --sign - "$APP" || true

echo "[$(ts)] 打包 zip + dmg"
cat > "$DIST/安裝說明.txt" <<'TXT'
Ultima III: Exodus 繁體中文版(macOS / Universal arm64+x86_64)

1. 解壓得到 Ultima3-CHT.app(已含完整遊戲資料與中文字型,解壓即玩)。
2. 首次開啟被 Gatekeeper 擋(「無法驗證開發者」)時,二選一:
   • 對 App 按右鍵 →「打開」→ 再按一次「打開」。
   • 終端機執行:  xattr -dr com.apple.quarantine Ultima3-CHT.app
3. 字塊風格可用環境變數 U3_TILESET 切換(如 Apple2 / C64 / VGA)。
TXT

( cd "$DIST" && ditto -c -k --sequesterRsrc --keepParent Ultima3-CHT.app Ultima3-CHT-macos-universal.zip \
  && zip -j Ultima3-CHT-macos-universal.zip 安裝說明.txt )
hdiutil create -volname "Ultima3-CHT" -srcfolder "$APP" -ov -format UDZO \
  "$DIST/Ultima3-CHT-macos-universal.dmg" || echo "  (dmg 產生失敗,zip 仍可用)"

echo "[$(ts)] 完成:"
ls -lh "$DIST"/Ultima3-CHT-macos-universal.* 2>/dev/null | awk '{print "  "$5, $9}'
