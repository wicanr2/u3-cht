#!/usr/bin/env python3
"""解析 Classic Mac 資源 fork (MainResources.rsrc),抽出資源。

用法:
  python3 tools/extract_rsrc.py list                 # 列出所有 type/ID/size
  python3 tools/extract_rsrc.py dump <TYPE> <outdir>  # 抽出某 type 的所有資源為 <TYPE>_<id>.bin
"""
import struct, sys, os

RSRC = os.path.join(os.path.dirname(__file__), "..", "..",
                    "ultima3", "Resources", "English.lproj", "MainResources.rsrc")


def parse(path):
    with open(path, "rb") as f:
        blob = f.read()
    dataOff, mapOff, dataLen, mapLen = struct.unpack(">IIII", blob[:16])
    m = mapOff
    # map: 16 reserved + 4 nextMap + 2 refnum + 2 attrs + 2 typeListOff + 2 nameListOff
    typeListOff = struct.unpack(">H", blob[m + 24:m + 26])[0]
    nameListOff = struct.unpack(">H", blob[m + 26:m + 28])[0]
    tlist = m + typeListOff
    numTypes = struct.unpack(">h", blob[tlist:tlist + 2])[0] + 1
    out = []
    for i in range(numTypes):
        e = tlist + 2 + i * 8
        rtype = blob[e:e + 4].decode("mac_roman")
        count = struct.unpack(">h", blob[e + 4:e + 6])[0] + 1
        refOff = struct.unpack(">H", blob[e + 6:e + 8])[0]
        rl = tlist + refOff
        for j in range(count):
            r = rl + j * 12
            rid = struct.unpack(">h", blob[r:r + 2])[0]
            dataOffset = struct.unpack(">I", b"\x00" + blob[r + 5:r + 8])[0]
            dpos = dataOff + dataOffset
            dlen = struct.unpack(">I", blob[dpos:dpos + 4])[0]
            data = blob[dpos + 4:dpos + 4 + dlen]
            out.append((rtype, rid, data))
    return out


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "list"
    res = parse(RSRC)
    if cmd == "list":
        from collections import Counter
        c = Counter(r[0] for r in res)
        for t, n in sorted(c.items()):
            print(f"type {t!r:8s} x{n}")
        print("---")
        for rtype, rid, data in res:
            print(f"  {rtype!r:8s} id={rid:6d} {len(data):6d} bytes")
    elif cmd == "dump":
        want, outdir = sys.argv[2], sys.argv[3]
        os.makedirs(outdir, exist_ok=True)
        n = 0
        for rtype, rid, data in res:
            if rtype == want:
                with open(os.path.join(outdir, f"{want.strip()}_{rid}.bin"), "wb") as f:
                    f.write(data)
                n += 1
        print(f"抽出 {n} 個 {want!r} 資源 → {outdir}")
