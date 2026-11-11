#!/usr/bin/env python3
# unic_drx_x86.py — 0.8.5 K3 (DRx/GetThreadContext) x86 Unicorn matrisi.
# Gercek vmx86.bin + SAHTE Dunya: PEB + sahte kernel32 (LDR listesi +
# export tablosu + GetThreadContext taklidi). Deterministik kanit:
#   drx  -> sahte GTC DR0=0x004D7610 dondurur -> POISON (K3 yakalar)
#   drx0 -> sahte GTC DR0=0 dondurur          -> TEMIZ (yanlis-pozitif YOK)
#   drxf -> sahte GTC BASARISIZ (0) dondurur  -> TEMIZ (skip guvenli)
#   clean/beingdbg/ntglobal/kapicc/tf -> 0.8.4 matrisi ayni beklenir
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from unicorn import *
from unicorn.x86_const import *

M1 = 0x9E3779B97F4A7C15
M2 = 0x2545F4914F6CDD1D
M3 = 0xD1B54A32D192ED03
M4 = 0x94D049BB133111EB
M5 = 0x100000001B3
R1 = 0x51ED270B4C5A3D0F
M64 = (1 << 64) - 1
VM_OPN = 0x2C
OP_ANTIDBG = 0x1E
OP_RET = 0x1A

BLOB = 0x00100000
IMG = 0x00400000
STK = 0x7F000000
STKSZ = 0x200000
PEB = 0x7E100000
LDR = 0x7E200000
K32 = 0x7E300000
CTX = 0x03100000
SCRATCH = 0x03110000
SENTINEL = 0x031F0000
FN_RVA = 0xD7610

def fnv_a(s):  # ANSI FNV-1a
    h = 2166136261
    for ch in s.encode():
        h ^= ch; h = (h * 16777619) & 0xFFFFFFFF
    return h

def fnv_w(s):  # UNICODE FNV-1a (kucuk-harf, wchar-bazli — xa_fnv_w ayni)
    h = 2166136261
    b = s.lower().encode('utf-16-le')
    for i in range(0, len(b), 2):
        ch = b[i] | (b[i + 1] << 8)
        h ^= ch; h = (h * 16777619) & 0xFFFFFFFF
    return h

XA_K32 = fnv_w('kernel32.dll')
XA_GTC = fnv_a('GetThreadContext')

def rol_key(k):
    return ((k * M5) & M64) ^ (k >> 23) ^ R1

def vm_ks(key, pos):
    a = (((key ^ ((pos * M1) & M64)) * M2) & M64) + M1 & M64
    b = ((rol_key(key)) ^ (((pos >> 3) * M3) & M64)) * M4 & M64
    return ((a ^ b) >> 56) & 0xFF

def vm_ksc(key, prev, pos):
    return vm_ks(key ^ ((prev * M2) & M64), pos)

def vm_keyxor(rva, ln):
    x = (0x5850424D ^ ((rva & 0xFFFFFFFF) << 32) ^ (ln & 0xFFFFFFFF)) & M64
    x ^= x >> 30; x = (x * 0xBF58476D1CE4E5B9) & M64
    x ^= x >> 27; x = (x * M4) & M64; x ^= x >> 31
    return x

def perm_p(seed):
    p = list(range(VM_OPN))
    z = ((seed ^ 0x5850424D9E3779B9) & M64) | 1
    for i in range(VM_OPN - 1, 0, -1):
        z ^= z >> 30; z = (z * 0xBF58476D1CE4E5B9) & M64
        z ^= z >> 27; z = (z * M4) & M64; z ^= z >> 31
        j = (z >> 32) % (i + 1)
        p[i], p[j] = p[j], p[i]
    return p

def find_offsets(blob):
    fs_sites = []
    i = 0
    pat = b'\x64\xA1\x30\x00\x00\x00'
    while True:
        i = blob.find(pat, i)
        if i < 0: break
        fs_sites.append(i)
        i += 1
    k2_site = -1
    for s in fs_sites:
        j = blob.find(b'\x9C\x58', s, s + 0x80)
        if j >= 0:
            k2_site = j
            break
    return fs_sites, k2_site

def build_fake_kernel32(mu, sc):
    # --- sahte kernel32 PE: MZ + PE + export(GetThreadContext) ---
    mu.mem_write(K32, b'MZ')                        # e_lfanew 0x3C
    mu.mem_write(K32 + 0x3C, struct.pack('<I', 0x80))
    mu.mem_write(K32 + 0x80, b'PE\0\0')
    mu.mem_write(K32 + 0x98, struct.pack('<H', 0x10B))  # OptionalHeader magic (PE32)
    # DataDirectory[0] = OptionalHeader + 96 -> export dir RVA
    exp_rva = 0x200
    mu.mem_write(K32 + 0x98 + 96, struct.pack('<I', exp_rva))
    mu.mem_write(K32 + 0x98 + 100, struct.pack('<I', 0x100))
    # ExportDirectory @ K32+0x200: +24 nnames, +28 AddressOfFunctions,
    # +32 AddressOfNames, +36 AddressOfNameOrdinals
    names_rva = 0x240
    funcs_rva = 0x260
    ords_rva = 0x280
    mu.mem_write(K32 + exp_rva + 24, struct.pack('<I', 1))    # NumberOfNames
    mu.mem_write(K32 + exp_rva + 28, struct.pack('<I', funcs_rva))
    mu.mem_write(K32 + exp_rva + 32, struct.pack('<I', names_rva))
    mu.mem_write(K32 + exp_rva + 36, struct.pack('<I', ords_rva))
    name = b'GetThreadContext\0'
    mu.mem_write(K32 + names_rva, struct.pack('<I', 0x2A0))   # name RVA
    mu.mem_write(K32 + 0x2A0, name)
    mu.mem_write(K32 + ords_rva, struct.pack('<H', 0))        # ordinal 0
    mu.mem_write(K32 + funcs_rva, struct.pack('<I', 0x2C0))   # fn RVA
    mu.mem_write(K32 + 0x2C0, b'\xC3')                        # ret (hook ezer)
    # --- PEB LDR listesi (x86 InMemoryOrderModuleList @ Ldr+0x14) ---
    mu.mem_write(PEB + 0x0C, struct.pack('<I', LDR))
    ent = LDR + 0x40                                          # modul entry
    mu.mem_write(LDR + 0x14, struct.pack('<I', ent + 0x08))   # head.Flink
    mu.mem_write(ent + 0x08, struct.pack('<I', LDR + 0x14))   # Flink -> head
    mu.mem_write(ent + 0x18, struct.pack('<I', K32))          # DllBase
    bdn = ent + 0x50
    mu.mem_write(ent + 0x2C, struct.pack('<H', 24))           # BaseDllName.Length
    mu.mem_write(ent + 0x30, struct.pack('<I', bdn))          # .Buffer
    mu.mem_write(bdn, 'kernel32.dll'.encode('utf-16-le'))
    return K32 + 0x2C0

def run_scenario(blob, sc, off_vm_entry, verbose=False):
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    for a, sz in [(BLOB, 0x40000), (IMG, 0x600000), (STK, STKSZ),
                  (PEB, 0x1000), (LDR, 0x1000), (K32, 0x1000),
                  (CTX, 0x1000), (SCRATCH, 0x1000), (SENTINEL, 0x1000)]:
        mu.mem_map(a, sz)
    mu.mem_write(BLOB, blob)
    fs_sites, k2_site = find_offsets(blob)
    assert fs_sites, 'fs:0x30 sitesi yok'
    assert k2_site >= 0, 'K2 pushfl yok'

    mu.mem_write(PEB + 2, struct.pack('<B', sc.get('beingdbg', 0)))
    mu.mem_write(PEB + 0x68, struct.pack('<I', sc.get('ntglobal', 0)))
    mu.mem_write(IMG + FN_RVA, b'\xCC' if sc.get('kapicc') else b'\xE9')

    gtc_addr = build_fake_kernel32(mu, sc)

    # sentetik bytecode: [ANTIDBG][RET]
    p = perm_p(FN_RVA)
    bc_rva = 0x545BC8
    bc_len = 8
    plain = bytes([p.index(OP_ANTIDBG), p.index(OP_RET), 0x90, 0x90, 0x90, 0x90, 0x90, 0x90])
    enc = bytearray()
    prev = 0
    for i, b in enumerate(plain):
        c = b ^ vm_ksc(0, prev, i)
        enc.append(c)
        prev = c
    mu.mem_write(IMG + bc_rva, bytes(enc))
    key = vm_keyxor(bc_rva, bc_len)

    mu.mem_write(CTX + 0x00, struct.pack('<I', IMG + bc_rva))
    mu.mem_write(CTX + 0x08, struct.pack('<I', bc_len))
    mu.mem_write(CTX + 0x10, struct.pack('<I', key & 0xFFFFFFFF))
    mu.mem_write(CTX + 0x14, struct.pack('<I', (key >> 32) & 0xFFFFFFFF))
    mu.mem_write(CTX + 0x18, struct.pack('<I', IMG))
    mu.mem_write(CTX + 0xA8, struct.pack('<I', SCRATCH))            # push_area
    mu.mem_write(SCRATCH + 28, struct.pack('<I', 0x11223344))       # push_area[7]=EAX
    mu.mem_write(CTX + 0xB0, struct.pack('<I', STK + STKSZ - 0x8000))  # init_rsp
    mu.mem_write(CTX + 0xE0, struct.pack('<I', FN_RVA))             # scratch[3]=seed

    def hook_code(mu, addr, size, ud):
        if addr in (BLOB + o for o in fs_sites):
            mu.reg_write(UC_X86_REG_EAX, PEB)
            mu.reg_write(UC_X86_REG_EIP, addr + 6)
        elif addr == BLOB + k2_site and sc.get('tf', 0):
            efl = mu.reg_read(UC_X86_REG_EFLAGS)
            mu.reg_write(UC_X86_REG_EFLAGS, efl | 0x100)
            mu.reg_write(UC_X86_REG_EIP, addr + 1)
            mu.reg_write(UC_X86_REG_ESP, mu.reg_read(UC_X86_REG_ESP) - 4)
            mu.mem_write(mu.reg_read(UC_X86_REG_ESP), struct.pack('<I', efl | 0x100))
            mu.reg_write(UC_X86_REG_EFLAGS, (efl | 0x100) & ~0x100)
        elif addr == gtc_addr:
            # sahte GetThreadContext(stdcall h, pctx): DR'ler yazar, eax=1
            esp = mu.reg_read(UC_X86_REG_ESP)
            ctxp = struct.unpack('<I', mu.mem_read(esp + 8, 4))[0]
            dr0 = sc.get('gtc_dr0', 0)
            ok = sc.get('gtc_ok', 1)
            if ctxp:
                mu.mem_write(ctxp + 0x00, struct.pack('<I', 0x00010010))
                for i2 in range(4):
                    mu.mem_write(ctxp + 0x04 + 4 * i2,
                                 struct.pack('<I', dr0 if i2 == 0 else 0))
            mu.reg_write(UC_X86_REG_EAX, 1 if ok else 0)
            ret_eip = struct.unpack('<I', mu.mem_read(esp, 4))[0]
            mu.reg_write(UC_X86_REG_ESP, esp + 12)     # ret 8 = eip(+4) + 2 arg(+8)
            mu.reg_write(UC_X86_REG_EIP, ret_eip)

    mu.hook_add(UC_HOOK_CODE, hook_code)
    if verbose:
        def mem_inv(mu, access, address, size, value, ud):
            print('  !! INVALID acc=%d addr=0x%08X eip=0x%08X' % (
                access, address, mu.reg_read(UC_X86_REG_EIP)))
            return False
        mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED
                    | UC_HOOK_MEM_FETCH_UNMAPPED, mem_inv)

    esp0 = STK + STKSZ - 0xC000
    mu.mem_write(esp0, struct.pack('<I', SENTINEL))
    mu.mem_write(esp0 + 4, struct.pack('<I', CTX))
    mu.reg_write(UC_X86_REG_ESP, esp0)
    try:
        mu.emu_start(BLOB + off_vm_entry, SENTINEL, timeout=10_000_000, count=1_000_000)
        err = 'OK'
    except UcError as e:
        err = 'UCERR %s eip=0x%X' % (e, mu.reg_read(UC_X86_REG_EIP))
    done = struct.unpack('<I', mu.mem_read(CTX + 0xC0, 4))[0]
    eax = struct.unpack('<I', mu.mem_read(CTX + 0x20, 4))[0]
    return done, eax, err

def main():
    blob_path = sys.argv[1] if len(sys.argv) > 1 else \
        '/home/z/my-project/scripts/xp085/XProtect_0.8.4/source/XProtectUI_Source/vmx86.bin'
    blob = open(blob_path, 'rb').read()
    off_vm_entry = int(sys.argv[2], 0) if len(sys.argv) > 2 else 0xE80
    cases = [
        ('clean',    dict(),                              ('OK', 0x11223344)),
        ('beingdbg', dict(beingdbg=1),                    ('OK', 0xDEADBEEF)),
        ('ntglobal', dict(ntglobal=0x70),                 ('OK', 0xDEADBEEF)),
        ('kapicc',   dict(kapicc=1),                      ('OK', 0xDEADBEEF)),
        ('tf',       dict(tf=1),                          ('OK', 0xDEADBEEF)),
        ('drx',      dict(gtc_dr0=0x004D7610),            ('OK', 0xDEADBEEF)),
        ('drx0',     dict(gtc_dr0=0),                     ('OK', 0x11223344)),
        ('drxfail',  dict(gtc_dr0=0x004D7610, gtc_ok=0),  ('OK', 0x11223344)),
    ]
    ok = 0
    for name, sc, (want_err, want_eax) in cases:
        done, eax, err = run_scenario(blob, sc, off_vm_entry)
        good = (err == want_err and eax == want_eax and done == 1)
        print('%-9s done=%d eax=0x%08X %s -> %s' % (
            name, done, eax, err, 'TAMAM' if good else 'HATA'))
        ok += good
    print('SONUC: %d/%d' % (ok, len(cases)))
    return 0 if ok == len(cases) else 1

if __name__ == '__main__':
    sys.exit(main())
