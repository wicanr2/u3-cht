#!/bin/bash
# render_music.sh — 把上游 QuickTime Music (.mov) 轉成可播放的 ogg。
#
# 上游 Resources/Music/Song_*.mov 是 QuickTime Music ('musi' 音符序列,類 MIDI),
# 標準工具 (ffmpeg/SDL_mixer) 無法直接解碼。流程:
#   1) tools/qtma2midi.py  : 解析 'musi' 音符事件 → 標準 MIDI
#   2) fluidsynth + GM soundfont : MIDI → wav
#   3) ffmpeg              : wav → ogg (assets/Music/Song_*.ogg)
#
# 需要:fluidsynth、ffmpeg、一個 GM soundfont (.sf2)。本機渲染用的 sf2 不入庫,
# 只提交產出的 ogg (引擎與資料分離)。已提交的 ogg 用 Roland SC-55 渲染。
#
# 可重現作法 (不依賴本機特定 sf2):用 throwaway 容器 + apt 取自由授權 GM soundfont:
#   docker run --rm -v "$PWD/..":/work ubuntu:24.04 bash -c '
#     apt-get update && apt-get install -y fluidsynth timgm6mb-soundfont ffmpeg python3
#     cd /work/u3-cht && bash tools/render_music.sh /usr/share/sounds/sf2/TimGM6mb.sf2'
# (timgm6mb-soundfont 為 Ubuntu/Debian 內建自由授權 GM 音色;或用 SC-55 等其他 GM sf2,
#  參考 https://github.com/bradhowes/SoundFonts)
#
# 用法:bash tools/render_music.sh <soundfont.sf2>
set -eu
SF="${1:?用法: render_music.sh <soundfont.sf2>}"
SRC="$(dirname "$0")/../../ultima3/Resources/Music"
OUT="$(dirname "$0")/../assets/Music"
TMP="$(mktemp -d)"
mkdir -p "$OUT"
for n in 1 2 3 4 5 6 7 8 A B; do
  mov="$SRC/Song_$n.mov"
  [ -e "$mov" ] || continue
  python3 "$(dirname "$0")/qtma2midi.py" "$mov" "$TMP/Song_$n.mid"
  fluidsynth -ni -g 0.8 -r 44100 -F "$TMP/Song_$n.wav" "$SF" "$TMP/Song_$n.mid" >/dev/null 2>&1
  ffmpeg -y -i "$TMP/Song_$n.wav" -c:a libvorbis -q:a 3 "$OUT/Song_$n.ogg" >/dev/null 2>&1
  echo "Song_$n.ogg $(du -h "$OUT/Song_$n.ogg" | cut -f1)"
done
rm -rf "$TMP"
echo "完成 → $OUT"
