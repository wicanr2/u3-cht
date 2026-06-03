#!/bin/bash
# package_appimage.sh — 在 Ubuntu 22.04 容器內把遊戲打包成自含 AppImage。
# 為相容性 (glibc 2.35) 用 22.04 建置,可在 22.04+ 執行。含完整遊戲資料 + 字型。
#
# 用法 (host):
#   docker run --rm -e HOSTUID=$(id -u) -e HOSTGID=$(id -g) \
#     -v /home/anr2/u3-cht:/work ubuntu:22.04 bash /work/u3-cht/tools/package_appimage.sh
set -eu
export DEBIAN_FRONTEND=noninteractive
REPO=/work/u3-cht
DIST=$REPO/dist
APP=/tmp/AppDir

echo "[ai] apt 安裝建置依賴 ..."
apt-get update -qq >/dev/null
apt-get install -y -qq build-essential clang git wget file python3-pil \
    libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev \
    fonts-noto-cjk >/dev/null

echo "[ai] 建置遊戲 (Ubuntu 22.04) ..."
bash $REPO/tools/build_game.sh >/tmp/aibuild.log 2>&1 || { echo "建置失敗:"; tail -25 /tmp/aibuild.log; exit 1; }
echo "[ai] 建置 OK ($(file $REPO/build/u3 | grep -o 'ELF.*stripped\|ELF.*x86-64' | head -1))"

# 重繪繁中 RaceClassInfo.gif (角色建立的種族/職業說明圖)。原為英文上游資產且被
# gitignore;此處由產生器重建,確保打包進 AppImage 的是中文版。失敗不致命 (退回現有檔)。
echo "[ai] 重繪中文 RaceClassInfo.gif ..."
python3 $REPO/tools/make_raceclass_gif.py >/tmp/aigif.log 2>&1 && echo "[ai] GIF OK" || { echo "[ai] ⚠ GIF 產生失敗 (沿用現有檔):"; tail -5 /tmp/aigif.log; }

# ---- 組 AppDir ----
rm -rf "$APP"; mkdir -p "$APP/usr/bin" "$APP/usr/lib" "$APP/data"
cp "$REPO/build/u3" "$APP/usr/bin/u3"

# 遞迴收集 SDL/媒體相依 .so (排除 glibc/驅動/X11/GL — 由宿主提供)
copy_deps() {
  ldd "$1" 2>/dev/null | awk '/=> \//{print $3}' | while read -r lib; do
    b=$(basename "$lib")
    # 只排除 glibc 家族與 GPU/GL/driver (必須由宿主提供,綁定硬體驅動);
    # 其餘 (X11/xcb/Xinerama/asound/dbus/udev/wayland 等) 全打包以求自含。
    case "$b" in
      libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|ld-linux*|\
      libgcc_s.so.*|libstdc++.so.*|\
      libGL.so.*|libGLX.so.*|libGLdispatch.so.*|libEGL.so.*|libOpenGL.so.*|libGLU.so.*|\
      libdrm.so.*|libgbm.so.*|libglapi.so.*) continue;;
    esac
    [ -e "$APP/usr/lib/$b" ] && continue
    cp -L "$lib" "$APP/usr/lib/$b" && copy_deps "$lib"
  done
}
copy_deps "$APP/usr/bin/u3"
echo "[ai] 打包 .so:$(ls "$APP/usr/lib" | wc -l) 個"

# 遊戲資料 (完整,自含) + 字型
cp -r "$REPO/assets" "$APP/data/assets"
mkdir -p "$APP/data/assets/fonts"
cp /usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc "$APP/data/assets/fonts/U3Font.ttc"
echo "[ai] 資料:$(du -sh "$APP/data" | cut -f1)"

# AppRun:設 cwd 到 data、字型、lib 路徑
cat > "$APP/AppRun" <<'RUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export U3_FONT="${U3_FONT:-$HERE/data/assets/fonts/U3Font.ttc}"
export U3_LANG="${U3_LANG:-zh-Hant}"
cd "$HERE/data" || exit 1
exec "$HERE/usr/bin/u3" "$@"
RUN
chmod +x "$APP/AppRun"

# .desktop + 圖示
cat > "$APP/u3cht.desktop" <<'DESK'
[Desktop Entry]
Type=Application
Name=Ultima III 中文版
Comment=Ultima III: Exodus 繁體中文化
Exec=u3
Icon=u3cht
Categories=Game;
Terminal=false
DESK
# 圖示 (PNG) — 用既有美術轉一張 256x256;無轉檔器時直接放 PNG
if [ -e "$REPO/assets/UltimaLogo.png" ]; then cp "$REPO/assets/UltimaLogo.png" "$APP/u3cht.png"; else
  printf 'P' > /dev/null; fi

# ---- appimagetool ----
echo "[ai] 取得 appimagetool ..."
AT=/tmp/appimagetool
wget -q "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" -O "$AT" || \
wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" -O "$AT"
chmod +x "$AT"
mkdir -p "$DIST"
ARCH=x86_64 "$AT" --appimage-extract-and-run "$APP" "$DIST/Ultima3-CHT-x86_64.AppImage" >/tmp/at.log 2>&1 || {
  echo "appimagetool 失敗:"; tail -20 /tmp/at.log; exit 1; }

# chown 回宿主使用者 (build 與 git apply 改過的上游檔)
chown -R "${HOSTUID:-1000}:${HOSTGID:-1000}" "$DIST" "$REPO/build" /work/ultima3/Sources 2>/dev/null || true
echo "[ai] 完成:$(ls -lh "$DIST"/*.AppImage | awk '{print $5, $9}')"
