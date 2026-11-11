#!/usr/bin/env python3
# bc_census_100.py — 1.0.0-UO bytecode yapisal census (encoder emit-sirasi
# birebir model: engine_lift.cpp akislari). Korumali PE'nin VM bytecode'unu
# CFB+perm ile cozer, op-uzunluklariyla UCTAN UCA yurur.
# Kullanim: bc_census_100.py <korumali.exe>
import struct, sys

def vm_roll(k):
    k &= 0xFFFFFFFFFFFFFFFF
    return ((k * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF) ^ (k >> 23) ^ 0x51ED270B4C5A3D0F

def vm_ks(key, pos):
    t1 = (((key ^ (pos * 0x9E3779B97F4A7C15)) * 0x2545F4914F6CDD1D) + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    t2 = ((vm_roll(key) ^ ((pos >> 3) * 0xD1B54A32D192ED03)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return (t1 ^ t2) >> 56 & 0xFF

def vm_ksc(key, prev, pos):
    k = (key ^ (prev * 0x2545F4914F6CDD1D)) & 0xFFFFFFFFFFFFFFFF
    return vm_ks(k, pos)

def vm_keyxor(bc_rva, bc_len):
    x = (0x5850424D ^ ((bc_rva & 0xFFFFFFFF) << 32) ^ (bc_len & 0xFFFFFFFF)) & 0xFFFFFFFFFFFFFFFF
    x ^= x >> 30; x = (x * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    x ^= x >> 27; x = (x * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    x ^= x >> 31
    return x

VM_OPN = 0x3E
def vm_perm_tab(seed, dec):
    p = list(range(VM_OPN))
    z = (seed ^ 0x5850424D9E3779B9) | 1
    for i in range(VM_OPN - 1, 0, -1):
        z ^= z >> 30; z = (z * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
        z ^= z >> 27; z = (z * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
        z ^= z >> 31
        j = (z >> 32) % (i + 1)
        p[i], p[j] = p[j], p[i]
    t = [0]*VM_OPN
    if dec:
        for i in range(VM_OPN): t[i] = p[i]
    else:
        for i in range(VM_OPN): t[p[i]] = i
    return t

# --- akis parser: emit sirasi engine_lift.cpp birebir ---
def u32(b, p): return int.from_bytes(b[p:p+4], 'little')

def memlen(b, p):
    f = b[p]
    if f == 0: return 5        # DISP: form + disp32
    if f in (1, 2): return 8   # SIB/SIB_IMG: form+base+idx+scale+disp32 (vm_core mem_addr birebir)
    if f == 3:                 # SEG: form+seg+altform[+disp|SIB]
        af = b[p+2]
        if af in (0, 4): return 3 + 4
        if af == 1: return 3 + 7   # SIB: base+idx+scale+disp32
        return -1
    if f == 4: return 5        # DISP_SEG: form + disp32
    return -1

def oplen(op, b, p, n):
    """op akista p konumunda (op bayti p'te); toplam uzunluk ya da -1"""
    q = p + 1
    def need(k):
        return q + k <= n
    if op == 0x00: return 1                                    # NOP
    if op in (0x01,): return 4                                 # MOV_RR sz,r1,r2
    if op == 0x02: return 7                                    # MOV_RI sz,r1,imm32
    if op == 0x03:                                             # MOV_RM sz,r1,mem
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml
    if op == 0x04:                                             # MOV_MR sz,mem,r1
        if not need(1): return -1
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml + 1
    if op == 0x05:                                             # MOV_MI sz,mem,imm32
        if not need(1): return -1
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml + 4
    if op == 0x06:                                             # LEA_RM r1,mem
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml
    if op == 0x07: return 5                                    # ALU_RR alu,sz,r1,r2
    if op == 0x08: return 8                                    # ALU_RI alu,sz,r1,imm32
    if op == 0x09:                                             # ALU_RM alu,sz,r1,mem
        if not need(3): return -1
        ml = memlen(b, q+3); return -1 if ml < 0 else 4 + ml
    if op == 0x0A:                                             # ALU_MR alu,sz,mem,r1
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml + 1
    if op == 0x0B:                                             # ALU_MI alu,sz,mem,imm32
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml + 4
    if op == 0x0C: return 4                                    # SHI_RI shi,sz,imm8
    if op == 0x0D: return 3                                    # SHI_CL shi,sz
    if op == 0x0E: return 8                                    # IMUL_RRI sz,r1,r2,imm32
    if op == 0x0F: return 4                                    # IMUL_RR sz,r1,r2
    if op == 0x10: return 4                                    # INCDEC_R ud,sz,r1
    if op == 0x11:                                             # INCDEC_M ud,sz,mem
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml
    if op in (0x12, 0x13):                                     # MOVZX/MOVSX sz,ssz,r1,form,[mem|src]
        if not need(4): return -1
        frm = b[q+3]
        if frm == 0: return 6
        ml = memlen(b, q+4); return -1 if ml < 0 else 5 + ml
    if op == 0x14: return 2                                    # PUSH_R r1
    if op == 0x15: return 5                                    # PUSH_I imm32
    if op == 0x16: return 2                                    # POP_R r1
    if op == 0x17: return 5                                    # JMP rel32
    if op == 0x18: return 6                                    # JCC cc,rel32
    if op == 0x19: return 5                                    # NCALL u32
    if op == 0x1A: return 1                                    # RET
    if op == 0x1B: return 3                                    # SETCC cc,r1
    if op == 0x1C: return 1                                    # CQO
    if op == 0x1D: return 3                                    # IDIV_R sz,r1
    if op == 0x1E: return 1                                    # ANTIDBG
    if op == 0x1F: return 17                                   # STRDEC 4xu32
    if op == 0x20: return 10                                   # MOV_RI64 r1,imm64
    if op == 0x21:                                             # CMOV cc,sz,r1,form,[mem|src]
        if not need(4): return -1
        frm = b[q+3]
        if frm == 0: return 6
        ml = memlen(b, q+4); return -1 if ml < 0 else 5 + ml
    if op == 0x22:                                             # SHI_RM_RI shi,sz,mem,imm8
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml + 1
    if op == 0x23:                                             # SHI_RM_CL shi,sz,mem
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml
    if op == 0x24:                                             # IMUL_RMRI sz,r1,mem,imm32
        if not need(2): return -1
        ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml + 4
    if op == 0x25: return 3                                    # NEG_R sz,r1
    if op == 0x26: return 3                                    # NOT_R sz,r1
    if op == 0x27:                                             # NEG_M sz,mem
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml
    if op == 0x28:                                             # NOT_M sz,mem
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml
    if op == 0x2A: return 3                                    # BSWAP_R sz,r1
    if op == 0x2B: return 4                                    # XCHG_RR sz,r1,r2
    if op in (0x2C, 0x2D, 0x2E, 0x2F): return 1                # PUSHF/POPF/SAHF/LAHF
    if op == 0x30: return 2                                    # SETFLAG ud
    if op in (0x31, 0x32):                                     # MUL_R/DIV_R sz,form[,mem]
        if not need(2): return -1
        if b[q+1] == 1:
            ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml
        return 3
    if op == 0x33:                                             # BT_X ud,sz,form[,mem],srcform[,src]
        if not need(3): return -1
        if b[q+2] == 1:
            ml = memlen(b, q+3); ln = 4 + ml
        else:
            ln = 5
        if not need(ln - p): return -1
        return ln + 2                                          # +srcform+src
    if op in (0x34, 0x35):                                     # SHLD/SHRD sz,r1,r2,nmode[,n]
        if not need(4): return -1
        return 6 if b[q+3] == 0 else 5
    if op == 0x36: return 6                                    # LOOP cc,rel32
    if op == 0x37: return 5                                    # JECXZ rel32
    if op == 0x38:                                             # XADD sz,form[,mem],r2
        if not need(2): return -1
        if b[q+1] == 1:
            ml = memlen(b, q+2); return -1 if ml < 0 else 3 + ml + 1
        return 4
    if op == 0x39: return 4                                    # STR ud,sz,rep
    if op == 0x3A:                                             # IMUL_RM sz,mem
        ml = memlen(b, q+1); return -1 if ml < 0 else 2 + ml
    if op in (0x3B, 0x3C): return 2                            # NOR_ST/NAND_ST sz
    if op == 0x3D: return 3                                    # POP_RP sz,r1
    return -1

NAMES = {0x00:'NOP',0x01:'MOV_RR',0x02:'MOV_RI',0x03:'MOV_RM',0x04:'MOV_MR',0x05:'MOV_MI',
         0x06:'LEA',0x07:'ALU_RR',0x08:'ALU_RI',0x09:'ALU_RM',0x0A:'ALU_MR',0x0B:'ALU_MI',
         0x0C:'SHI_RI',0x0D:'SHI_CL',0x0E:'IMUL_RRI',0x0F:'IMUL_RR',0x10:'INCDEC_R',
         0x11:'INCDEC_M',0x12:'MOVZX',0x13:'MOVSX',0x14:'PUSH_R',0x15:'PUSH_I',0x16:'POP_R',
         0x17:'JMP',0x18:'JCC',0x19:'NCALL',0x1A:'RET',0x1B:'SETCC',0x1C:'CQO',0x1D:'IDIV',
         0x1E:'ANTIDBG',0x1F:'STRDEC',0x20:'MOV_RI64',0x21:'CMOV',0x25:'NEG_R',0x26:'NOT_R',
         0x2A:'BSWAP',0x2B:'XCHG',0x30:'SETFLAG',0x31:'MUL',0x32:'DIV',0x33:'BT',0x34:'SHLD',
         0x35:'SHRD',0x36:'LOOP',0x37:'JECXZ',0x38:'XADD',0x39:'STR',0x3A:'IMUL_RM',
         0x3B:'NOR_ST',0x3C:'NAND_ST',0x3D:'POP_RP'}

def main(path):
    d = open(path, 'rb').read()
    pe_off = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe_off + 6)[0]
    optsz = struct.unpack_from('<H', d, pe_off + 20)[0]
    secs = []
    for i in range(nsec):
        o = pe_off + 24 + optsz + i * 40
        nm = d[o:o+8].rstrip(b'\x00').decode()
        vsz, va, rsz, raw = struct.unpack_from('<IIII', d, o+8)
        secs.append((nm, va, vsz, rsz, raw))
    vmp0 = [s for s in secs if s[0] == '.vmp0'][0]
    _, va, vsz, rsz, raw = vmp0
    sec = d[raw:raw+rsz]
    pat = b'\xC7\x84\x24\xE0\x00\x00\x00'
    idx = sec.find(pat)
    if idx < 0:
        print('HATA: stub seed yazimi yok'); return 1
    seed = struct.unpack_from('<I', sec, idx+7)[0]
    machine = struct.unpack_from('<H', d, pe_off+4)[0]
    if machine == 0x8664:
        stub_off = idx - 114
        key_obf = struct.unpack_from('<Q', sec, stub_off+59)[0]
        bc_rva = struct.unpack_from('<I', sec, stub_off+92)[0]
        bc_len = struct.unpack_from('<Q', sec, stub_off+101)[0]
    else:
        stub_off = idx - 83
        lo = struct.unpack_from('<I', sec, stub_off+34)[0]
        hi = struct.unpack_from('<I', sec, stub_off+43)[0]
        key_obf = (hi << 32) | lo
        bc_rva = struct.unpack_from('<I', sec, stub_off+67)[0]
        bc_len = struct.unpack_from('<I', sec, stub_off+75)[0]
    bsec = None
    for s in secs:
        if s[1] <= bc_rva < s[1] + max(s[2], s[3]): bsec = s; break
    if bsec is None:
        print('HATA: bc bolumu yok'); return 1
    boff = bsec[4] + (bc_rva - bsec[1])
    bc = d[boff:boff+bc_len]
    key = key_obf ^ vm_keyxor(bc_rva, bc_len)
    plain = bytearray(bc_len)
    prev = 0
    for i in range(bc_len):
        plain[i] = bc[i] ^ vm_ksc(key, prev, i)
        prev = bc[i]
    itab = vm_perm_tab(seed, 1)
    print(f'seed=0x{seed:X} bc_rva=0x{bc_rva:X} bc_len={bc_len} makine={"x64" if machine==0x8664 else "x86"}')

    pc = 0; cnt = {}; ops = 0; last = -1; bad = None
    while pc < bc_len:
        raw_op = plain[pc]
        if raw_op >= VM_OPN: bad = (pc, 'op-uzay-disi', raw_op); break
        op = itab[raw_op]
        ln = oplen(op, plain, pc, bc_len)
        if ln <= 0 or pc + ln > bc_len:
            bad = (pc, f'op={NAMES.get(op, hex(op))} uzunluk-tasmasi/parse', ln); break
        cnt[op] = cnt.get(op, 0) + 1
        last = op; ops += 1
        pc += ln
    if bad:
        print(f'HATA: pc={bad[0]} {bad[1]} ({bad[2]}) — yuruyus {ops} op sonra bozuldu')
        return 2
    gate = cnt.get(0x3B, 0) + cnt.get(0x3C, 0)
    print(f'yuruyus: {ops} op, tam bc_len={bc_len} bitti, son-op={NAMES.get(last, hex(last))}')
    print(f'kapi sayimi: NOR_ST={cnt.get(0x3B,0)} NAND_ST={cnt.get(0x3C,0)} POP_RP={cnt.get(0x3D,0)} toplam-kapi={gate}')
    top = sorted(cnt.items(), key=lambda kv: -kv[1])[:12]
    print('ust-ops:', ', '.join(f'{NAMES.get(o, hex(o))}={c}' for o, c in top))
    if last == 0x1A and gate > 0:
        print('SONUC: YAPI-SAGLAM (uctan-uca yuruyus) + KAPI AKTIF ✓')
        return 0
    if last == 0x1A:
        print('SONUC: yapi-saglam ama kapi yok (fonksiyonda and/or/xor reg-formu yokmus)')
        return 0
    print('SONUC: son-op RET degil ✗')
    return 3

if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
