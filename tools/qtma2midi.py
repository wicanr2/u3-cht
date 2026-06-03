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

def parse_tune_header(d):
    """從 stsd 'musi' 取各 part 的 GM 樂器號 (ToneDescription 末端的
    instrumentNumber/gmNumber)。ToneDescription: synthType(4)+synthName(Str31=32)
    +instrumentName(Str31=32)+instNum(4)+gmNum(4);gmNum 在 synthName 起點 +68。
    回傳 part 順序的 gmNumber 清單 (1-indexed GM;鼓組為 0x4000+)。"""
    si, end = d.find(b"musi"), d.find(b"mdat")
    if si < 0:
        return []
    region = d[si:end if end > si else si + 4000]
    progs, i = [], 0
    while i < len(region) - 72:
        ln = region[i]
        if 1 <= ln <= 31 and all(32 <= region[i + 1 + k] < 127 for k in range(ln)):
            instnum = struct.unpack(">I", region[i + 64:i + 68])[0]
            gmnum = struct.unpack(">I", region[i + 68:i + 72])[0]
            if instnum == gmnum and (1 <= gmnum <= 128 or 0x4000 <= gmnum <= 0x4080) \
               and 1 <= region[i + 32] <= 31:
                progs.append(gmnum)
                i += 68
                continue
        i += 1
    return progs

def build_part_channel(progs):
    """part → (midi_channel, program或None)。鼓組 (gm>=0x4000) 用 ch9;其餘依序
    分配 channel (跳過 9),program = gm-1 (轉 0-indexed)。"""
    pmap, nextch = {}, 0
    for part, gm in enumerate(progs):
        if gm >= 0x4000:
            pmap[part] = (9, None)
        else:
            ch = nextch
            nextch += 1
            if ch == 9:
                ch = nextch
                nextch += 1
            pmap[part] = (ch & 0x0F, (gm - 1) & 0x7F)
    return pmap

def chan_of(pmap, part):
    return pmap[part][0] if part in pmap else (part & 0x0F)

def parse_events(mdat, pmap):
    """回傳 (note_events, ctrl_events, total_time)。
    note: (start, dur, chan, pitch, vel);ctrl: (start, chan, cc, val)。
    QTMA 控制器號與 MIDI CC 相同 (7=音量, 10=聲相, 91=殘響, 93=合唱)。"""
    t = 0
    notes, ctrls = [], []
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
                notes.append((t, dur, chan_of(pmap, part), pitch, vel or 64))
        elif et == 2:                    # Control change
            part = (w >> 24) & 0x1F
            cc   = (w >> 16) & 0xFF
            val  = (w >> 8) & 0xFF
            if cc < 120:
                ctrls.append((t, chan_of(pmap, part), cc, min(val, 127)))
        # Marker(3)/General(4-7):略過 (其內容控制事件會以 top3=2 各自解析)
    return notes, ctrls, t

def write_midi(notes, ctrls, pmap, path, division=600):
    # 組 program-change (t=0) + note-on/off + control-change 事件,排序後寫 delta-time
    ev = []
    for part, (chan, prog) in pmap.items():
        if prog is not None:
            ev.append((0, 0xC0 | chan, prog, -1))        # program change (2-byte)
    for start, chan, cc, val in ctrls:
        ev.append((start, 0xB0 | chan, cc, val))         # control change
    for start, dur, chan, pitch, vel in notes:
        ev.append((start, 0x90 | chan, pitch, vel))      # note on
        ev.append((start + dur, 0x80 | chan, pitch, 0))  # note off
    # 同刻優先序:program(0xC0)/control(0xB0) < note-off(0x80) < note-on(0x90)
    def pri(st):
        h = st & 0xF0
        return 0 if h in (0xC0, 0xB0) else (1 if h == 0x80 else 2)
    ev.sort(key=lambda e: (e[0], pri(e[1])))
    track = bytearray()
    # tempo: 1,000,000 us/quarter (配 division=600 → 1 tick=1/600s)
    track += b"\x00\xFF\x51\x03" + struct.pack(">I", 1000000)[1:]
    last = 0
    def vlq(n):
        b = [n & 0x7F]; n >>= 7
        while n: b.append((n & 0x7F) | 0x80); n >>= 7
        return bytes(reversed(b))
    for tm, status, d1, d2 in ev:
        if (status & 0xF0) == 0xC0:                       # program change: 2-byte
            track += vlq(tm - last) + bytes([status, d1])
        else:
            track += vlq(tm - last) + bytes([status, d1, d2])
        last = tm
    track += b"\x00\xFF\x2F\x00"                          # end of track
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, division))
        f.write(b"MTrk" + struct.pack(">I", len(track)) + track)

if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    data = open(src, "rb").read()
    mdat = find_mdat(data)
    pmap = build_part_channel(parse_tune_header(data))
    notes, ctrls, total = parse_events(mdat, pmap)
    write_midi(notes, ctrls, pmap, dst)
    insts = ",".join(f"p{p}=ch{c}{'(鼓)' if pr is None else '/gm'+str(pr+1)}" for p, (c, pr) in sorted(pmap.items()))
    print(f"{src}: {len(notes)} 音符, {len(ctrls)} 控制, 總長 {total/600:.1f}s, 樂器[{insts}] → {dst}")
