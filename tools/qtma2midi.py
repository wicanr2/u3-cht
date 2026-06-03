#!/usr/bin/env python3
"""把 LairWare Ultima III 的 QuickTime Music (.mov, 'musi'/QTMA) 轉成標準 MIDI。

QTMA tune 事件 (32-bit big-endian word),由 (w>>29)&7 分類:
  0 Rest    : 推進目前時間,duration = w & 0x1FFFFF (timescale 單位)
  1 Note    : part=(w>>24)&0x1F, pitch=((w>>18)&0x3F)+32, vel=(w>>11)&0x7F,
              duration = w & 0x7FF;於目前時間排程 note-on,+duration note-off
  2 Control : 控制器 (移植期略過)
  3 Marker  : 標記 (略過)
其餘 (一般/擴充事件) 於本批曲目未出現,保險起見以單字略過。
mdhd timescale = 600 單位/秒 → MIDI division=600, tempo=1,000,000us/四分音符
(1 tick = 1/600 秒),duration 單位即 MIDI tick。

用法:python3 tools/qtma2midi.py <in.mov> <out.mid>
"""
import struct, sys

def find_mdat(d):
    o = 0
    while o + 8 <= len(d):
        sz, typ = struct.unpack(">I", d[o:o+4])[0], d[o+4:o+8]
        if sz == 0: sz = len(d) - o
        if sz < 8: break
        if typ == b"mdat":
            return d[o+8:o+sz]
        o += sz
    return b""

def parse_events(mdat):
    """回傳 (note_events, total_time)。note_events: (start, dur, chan, pitch, vel)。"""
    t = 0
    notes = []
    for i in range(0, len(mdat) - 3, 4):
        w = struct.unpack(">I", mdat[i:i+4])[0]
        et = (w >> 29) & 7
        if et == 0:                      # Rest
            t += w & 0x1FFFFF
        elif et == 1:                    # Note
            part  = (w >> 24) & 0x1F
            pitch = ((w >> 18) & 0x3F) + 32
            vel   = (w >> 11) & 0x7F
            dur   = w & 0x7FF
            if 0 < pitch < 128 and dur > 0:
                notes.append((t, dur, part & 0x0F, pitch, vel or 64))
        # Control(2)/Marker(3)/其他:略過 (單字)
    return notes, t

def write_midi(notes, path, division=600):
    # 組 note-on/off 事件 (絕對時間),排序後寫成 delta-time
    ev = []
    for start, dur, chan, pitch, vel in notes:
        ev.append((start, 0x90 | chan, pitch, vel))      # note on
        ev.append((start + dur, 0x80 | chan, pitch, 0))  # note off
    ev.sort(key=lambda e: (e[0], e[1] & 0x80))           # 同刻先 off 再 on? 此處 on(0x90)>off(0x80) 排序: off 先
    track = bytearray()
    # tempo: 1,000,000 us/quarter (配 division=600 → 1 tick=1/600s)
    track += b"\x00\xFF\x51\x03" + struct.pack(">I", 1000000)[1:]
    last = 0
    def vlq(n):
        b = [n & 0x7F]; n >>= 7
        while n: b.append((n & 0x7F) | 0x80); n >>= 7
        return bytes(reversed(b))
    for tm, status, d1, d2 in ev:
        track += vlq(tm - last) + bytes([status, d1, d2])
        last = tm
    track += b"\x00\xFF\x2F\x00"                          # end of track
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, division))
        f.write(b"MTrk" + struct.pack(">I", len(track)) + track)

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    mdat = find_mdat(open(src, "rb").read())
    notes, total = parse_events(mdat)
    write_midi(notes, dst)
    print(f"{src}: {len(notes)} 音符, 總長 {total/600:.1f}s → {dst}")
