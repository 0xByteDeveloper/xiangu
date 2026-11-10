#!/usr/bin/env python3
# static_ad_check.py — 0.8.1 statik anti-debug kaniti
# Korunan exe'den stub'i bul (seed=C7 84 24 E0 00 00 00 <rva>), bytecode'i
# CFB ile coz, ilk op'un itab uzerinden ANTIDBG (0x1E)'ye dustugunu dogrula.
import struct, sys

M1 = 0x9E3779B97F4A7C15
M2 = 0x2545F4914F6CDD1D
M3 = 0xD1B54A32D192ED03
M4 = 0x94D049BB133111EB
M5 = 0x100000001B3
R1 = 0x51ED270B4C5A3D0F
MASK = (1 << 64) - 1

def rol_key(k):
    return ((k * M5) & MASK) ^ (k >> 23) ^ R1

def vm_ks(key, pos):
    a = (((key ^ ((pos * M1) & MASK)) * M2) & MASK) + M1 & MASK
    b = ((rol_key(key)) ^ (((pos >> 3) * M3) & MASK)) * M4 & MASK
    return ((a ^ b) >> 56) & 0xFF

def vm_ksc(key, prev, pos):
    return vm_ks(key ^ ((prev * M2) & MASK), pos)

def vm_keyxor(rva, ln):
    x = (0x5850424D ^ ((rva << 32) & MASK) ^ ln) & MASK
    x ^= x >> 30
    x = (x * 0xBF58476D1CE4E5B9) & MASK
    x ^= x >> 27
    x = (x * M4) & MASK
    x ^= x >> 31
    return x

VM_OPN = 0x2C  # opcode uzayi (vm_opcodes.h)

def vm_perm_tab(seed, dec):
    p = list(range(VM_OPN))
    z = ((seed ^ 0x5850424D9E3779B9) & MASK) | 1
    for i in range(VM_OPN - 1, 0, -1):
        z ^= z >> 30; z = (z * 0xBF58476D1CE4E5B9) & MASK
        z ^= z >> 27; z = (z * M4) & MASK; z ^= z >> 31
        j = (z >> 32) % (i + 1)
        p[i], p[j] = p[j], p[i]
    t = [0] * 256
    if dec:
        for i in range(VM_OPN): t[i] = p[i]        # raw -> gercel
    else:
        for i in range(VM_OPN): t[p[i]] = i        # gercel -> raw
    return t

def main(path):
    d = open(path, 'rb').read()
    pe_off = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe_off + 6)[0]
    optsz = struct.unpack_from('<H', d, pe_off + 20)[0]
    secs = []
    for i in range(nsec):
        o = pe_off + 24 + optsz + i * 40
        nm = d[o:o + 8].rstrip(b'\x00').decode()
        vsz, va, rsz, raw = struct.unpack_from('<IIII', d, o + 8)
        secs.append((nm, va, vsz, rsz, raw))
    vmp0 = [s for s in secs if s[0] == '.vmp0'][0]
    _, va, vsz, rsz, raw = vmp0
    sec = d[raw:raw + rsz]

    # stub'ta seed yazimi: C7 84 24 E0 00 00 00 <seed LE>
    pat = b'\xC7\x84\x24\xE0\x00\x00\x00'
    # stub yerlesimleri (build_stub_x64/x86 birebir sayim):
    # x64: 15 push=23B ... seed C7 84 24 E0.. yazimi stub+114; key(8B)@59,
    #      bc_rva(4B)@92, bc_len(8B)@101
    # x86: pushad=1 ... seed yazimi stub+83; key_lo(4B)@34, key_hi(4B)@43,
    #      bc_rva(4B)@67, bc_len(4B)@75
    machine = struct.unpack_from('<H', d, pe_off + 4)[0]
    idx = sec.find(pat)
    if idx < 0:
        print('HATA: stub seed yazimi bulunamadi'); return 1
    seed = struct.unpack_from('<I', sec, idx + 7)[0]
    if machine == 0x8664:
        stub_off = idx - 114
        key_obf = struct.unpack_from('<Q', sec, stub_off + 59)[0]
        bc_rva = struct.unpack_from('<I', sec, stub_off + 92)[0]
        bc_len = struct.unpack_from('<Q', sec, stub_off + 101)[0]
    else:
        stub_off = idx - 83
        lo = struct.unpack_from('<I', sec, stub_off + 34)[0]
        hi = struct.unpack_from('<I', sec, stub_off + 43)[0]
        key_obf = (hi << 32) | lo
        bc_rva = struct.unpack_from('<I', sec, stub_off + 67)[0]
        bc_len = struct.unpack_from('<I', sec, stub_off + 75)[0]
    print(f'seed=0x{seed:X} bc_rva=0x{bc_rva:X} bc_len={bc_len}')

    # bc .vmp0 icinde mi? (baska bolumde ise raw hesabi ayri)
    bsec = None
    for s in secs:
        if s[1] <= bc_rva < s[1] + max(s[2], s[3]):
            bsec = s; break
    if bsec is None:
        print('HATA: bc bolumu bulunamadi'); return 1
    boff = bsec[4] + (bc_rva - bsec[1])
    bc = d[boff:boff + bc_len]

    key = key_obf ^ vm_keyxor(bc_rva, bc_len)
    plain = bytearray(bc_len)
    prev = 0
    for i in range(bc_len):
        plain[i] = bc[i] ^ vm_ksc(key, prev, i)
        prev = bc[i]

    itab = vm_perm_tab(seed, 1)
    op0 = itab[plain[0]]
    print(f'bc[0]=0x{plain[0]:02X} -> itab -> 0x{op0:02X}')
    if op0 == 0x1E:
        print('SONUC: ANTIDBG (0x1E) DOGRU COZULUYOR ✓')
        return 0
    else:
        names = {0x1C: 'CQO (zararsiz — 0.8.0 BUG\'U)', 0x1F: 'STRDEC'}
        print(f'SONUC: YANLIS OP 0x{op0:02X} {names.get(op0, "")} ✗')
        return 2

if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
