#!/usr/bin/env python3
# find_damage_rva.py — test exe'de damage() fonksiyonun GERKEK RVA/boyunu bul
# x64: .pdata RUNTIME_FUNCTION girdisi (kesin). x86: marker'dan geriye
# prolog taramasi (55 89 E5 / 55 8B EC).
import sys, struct

def main(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    mach = struct.unpack_from('<H', d, pe+4)[0]
    nsec = struct.unpack_from('<H', d, pe+6)[0]
    optsz = struct.unpack_from('<H', d, pe+20)[0]
    magic = struct.unpack_from('<H', d, pe+24)[0]
    is64 = (magic == 0x20B)
    secs = []
    for s in range(nsec):
        o = pe+24+optsz+s*40
        nm  = d[o:o+8].rstrip(b'\x00').decode(errors='replace')
        va  = struct.unpack_from('<I', d, o+12)[0]
        vsz = struct.unpack_from('<I', d, o+8)[0]
        raw = struct.unpack_from('<I', d, o+20)[0]
        rsz = struct.unpack_from('<I', d, o+16)[0]
        secs.append((nm, va, vsz, raw, rsz))
    def raw2rva(i):
        for nm, va, vsz, raw, rsz in secs:
            if raw <= i < raw+rsz:
                return va + (i - raw)
        return None
    mk = bytes([0x90,0x90,0xB8,0x5A,0x6B,0x7C,0x11])
    mi = d.find(mk)
    if mi < 0:
        print("0 0 MARKER_YOK"); return
    mrva = raw2rva(mi)
    if is64:
        # .pdata tara: Begin <= mrva < End
        for nm, va, vsz, raw, rsz in secs:
            if nm == '.pdata':
                for k in range(0, rsz, 12):
                    b, e, u = struct.unpack_from('<III', d, raw+k)
                    if b and b <= mrva < e:
                        print("%X %X" % (b, e-b)); return
        print("0 0 PDATA_YOK"); return
    else:
        # geriye prolog tara (en fazla 48 bayt)
        for back in range(0, 48):
            p = mi - back
            if p < 0: break
            if d[p] == 0x55 and d[p+1:p+3] in (b'\x89\xe5', b'\x8b\xec'):
                rva = raw2rva(p)
                if rva:
                    print("%X %X" % (rva, 0x100)); return
        print("0 0 PROLOG_YOK"); return

if __name__ == '__main__':
    main(sys.argv[1])
