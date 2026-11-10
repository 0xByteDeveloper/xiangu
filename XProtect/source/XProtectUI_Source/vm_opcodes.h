// ============================================================
// vm_opcodes.h — VM bayt-kod formatinin TEK kaynagi.
// Hem CPlusPort engine (encoder) hem vm_core.c (decoder) kullanir.
// Her byte rolling-LCG anahtari ile sifrelenir:
//   cipher = b ^ (key & 0xFF);  key = key*A + C
// ============================================================
#ifndef VM_OPCODES_H
#define VM_OPCODES_H

// 0.8.0 crypt_command — IKI-KATMANLI konum keystream'i (VMProtect
// crypt_command'a es-karsilik): katman-1 ince (byte pozisyon), katman-2
// kaba (8-byte periyot, roll edilmis anahtar). Ikisi de SALT (key,pos)
// fonksiyonu oldugu icin JMP/JCC siralamasindan bagimsiz encoder/decoder
// uyumu korunur; tek-bayt tamper artik 8-byte blok degradesini de degistirir.
// NOT: eski tek-katman VM_KS kullanan tum ciktilar bu surumle yeniden
// sifrelenir (encoder + decoder ayni makrodan turetir).
#define VM_ROLL(k) ((((unsigned long long)(k)) * 0x100000001B3ULL) \
    ^ (((unsigned long long)(k)) >> 23) ^ 0x51ED270B4C5A3D0FULL)
#define VM_KS(key, pos) ((unsigned char)( \
    ( ((((unsigned long long)(key)) ^ ((unsigned long long)(pos)) * 0x9E3779B97F4A7C15ULL) \
        * 0x2545F4914F6CDD1DULL + 0x9E3779B97F4A7C15ULL) \
    ^ ( ((((VM_ROLL(key))) ^ ((unsigned long long)((pos) >> 3)) * 0xD1B54A32D192ED03ULL) \
        * 0x94D049BB133111EBULL) ) \
    ) >> 56))

// 0.8.0 crypt_command — CFB geribesleme katmani: her baytin akisi BIR
// ONCEKI SIFRELI bayta baglanir (cipher-feedback). encoder sira ile
// uretir (enc[i-1] elinde), decoder konumdan okur (bc[i-1] buffer'da) —
// JMP/JCC konum-degisikligi zinciri BOZMAZ (saf (key, prev, pos) fonksiyonu),
// tek-bayt tamper sonraki baytin cozumunu de degistirir (yayilim).
#define VM_KSC(key, prev, pos) VM_KS( \
    ((unsigned long long)(key)) ^ ((unsigned long long)(unsigned char)(prev)) * 0x2545F4914F6CDD1DULL, \
    (pos))

// 0.8.0 crypt_key — stub'in tasiydigi anahtar obfuscated: stub key^K2
// yazar; vm_entry giriste K2 ile geri cozer. K2 = (bc_rva, bc_len) purely
// fonksiyonu — encoder ve decoder ayni makrodan turetir; `mov rax,imm64`
// artik duz anahtar tasimaz. (splitmix64 finalizer; salt-ifade)
#define VM_KXI(rva, len) (0x5850424DULL \
    ^ ((unsigned long long)(unsigned int)(rva) << 32) \
    ^ (unsigned long long)(unsigned int)(len))
#define VM_KXA(x) ((x) ^ ((x) >> 30))
#define VM_KXB(x) ((x) * 0xBF58476D1CE4E5B9ULL)
#define VM_KXC(x) ((x) ^ ((x) >> 27))
#define VM_KXD(x) ((x) * 0x94D049BB133111EBULL)
#define VM_KXE(x) ((x) ^ ((x) >> 31))
#define VM_KEYXOR(bc_rva, bc_len) \
    (VM_KXE(VM_KXD(VM_KXC(VM_KXB(VM_KXA(VM_KXI(bc_rva, bc_len)))))))

// --- opcode numaralari ---
#define OP_NOP       0x00   // mutation junk / alignment
#define OP_MOV_RR    0x01   // sz r1 r2
#define OP_MOV_RI    0x02   // sz r1 imm32(sx)
#define OP_MOV_RM    0x03   // sz r1 mem
#define OP_MOV_MR    0x04   // sz mem r1
#define OP_MOV_MI    0x05   // sz mem imm32(sx)
#define OP_LEA_RM    0x06   // r1 mem
#define OP_ALU_RR    0x07   // alu sz r1 r2
#define OP_ALU_RI    0x08   // alu sz r1 imm32
#define OP_ALU_RM    0x09   // alu sz r1 mem
#define OP_ALU_MR    0x0A   // alu sz mem r1
#define OP_ALU_MI    0x0B   // alu sz mem imm32
#define OP_SHI_RI    0x0C   // shi sz r1 imm8
#define OP_SHI_CL    0x0D   // shi sz r1
#define OP_IMUL_RRI  0x0E   // sz r1 r2 imm32
#define OP_IMUL_RR   0x0F   // sz r1 r2
#define OP_INCDEC_R  0x10   // ud sz r1
#define OP_INCDEC_M  0x11   // ud sz mem
#define OP_MOVZX     0x12   // dsz ssz r1 form src
#define OP_MOVSX     0x13   // dsz ssz r1 form src
#define OP_PUSH_R    0x14   // r1
#define OP_PUSH_I    0x15   // imm32(sx)
#define OP_POP_R     0x16   // r1
#define OP_JMP       0x17   // rel32 (bytecode icinde)
#define OP_JCC       0x18   // cc rel32
#define OP_NCALL     0x19   // u32: target = base + rva
#define OP_RET       0x1A
#define OP_SETCC     0x1B   // cc r1
#define OP_CQO       0x1C   // RAX->RDX (x64) / EAX->EDX (x86)
#define OP_IDIV_R    0x1D   // sz r1
#define OP_ANTIDBG   0x1E
#define OP_STRDEC    0x1F   // u32 strRva u32 stateRva u32 key u32 len
#define OP_MOV_RI64  0x20   // r1 imm64
#define OP_CMOV      0x21   // cc sz r1 form (0: r2 | 1: mem)
#define OP_SHI_RM_RI 0x22   // shi sz mem imm8   (bellek shift)
#define OP_SHI_RM_CL 0x23   // shi sz mem        (bellek shift %cl)
#define OP_IMUL_RMRI 0x24   // sz r1 mem imm32   (imul r,[m],imm)
#define OP_NEG_R     0x25   // sz r1             (0 - r; CF=x!=0,ZF/SF/OF)
#define OP_NOT_R     0x26   // sz r1             (~r; bayrak etkisi yok)
#define OP_NEG_M     0x27   // sz mem
#define OP_NOT_M     0x28   // sz mem
#define OP_BSWAP_R   0x2A   // sz r1
#define OP_XCHG_RR   0x2B   // sz r1 r2        (bayrak etkisi yok)
// ROL/ROR: OP_SHI_RI/CL'nin shi=3/4 degeriyle (yeni op gerekmez)

// --- 0.8.0 polimorfik isleyiciler: opcode permütasyonu ---
// π = anahtar-turetilmis (Fisher-Yates; tohum = fonksiyon RVA'si).
// Encoder (engine_lift eop) gercel->raw yazar (vm_perm_enc); interpreter
// (vm_run) raw->gercek cozumler (vm_perm_dec). Her iki taraf AYNI
// fonksiyondan turer -> tablo tasimaya gerek yok (stub yalniz tohumu
// ctx->scratch[3]'e yazar). Tohum 0 = kimlik degil, gecerli permütasyon
// (determinizm iki tarafta da ayni fonksiyondan).
#define VM_OPN 0x2C   // opcode uzayi (0x00..0x2B)
static void vm_perm_tab(unsigned int seed, unsigned char* t, int dec) {
    unsigned char p[VM_OPN]; int i, j;
    for (i = 0; i < VM_OPN; i++) p[i] = (unsigned char)i;
    {
        unsigned long long z = ((unsigned long long)seed ^ 0x5850424D9E3779B9ULL) | 1ULL;
        for (i = VM_OPN - 1; i > 0; i--) {
            z ^= z >> 30; z *= 0xBF58476D1CE4E5B9ULL;
            z ^= z >> 27; z *= 0x94D049BB133111EBULL; z ^= z >> 31;
            /* x86 freestanding: 64-bit modulo YOK (umoddi3) — yuksek 32B */
            j = (int)(((unsigned int)(z >> 32)) % (unsigned int)(i + 1));
            { unsigned char tmp = p[i]; p[i] = p[j]; p[j] = tmp; }
        }
    }
    if (dec) { for (i = 0; i < VM_OPN; i++) t[i] = p[i]; }          // raw->gercek
    else     { for (i = 0; i < VM_OPN; i++) t[p[i]] = (unsigned char)i; } // gercek->raw
}

// --- ALU alt kodlari ---
#define ALU_ADD 0
#define ALU_SUB 1
#define ALU_AND 2
#define ALU_OR  3
#define ALU_XOR 4
#define ALU_CMP 5
#define ALU_TEST 6

// --- shift alt kodlari ---
#define SHI_SHL 0
#define SHI_SHR 1
#define SHI_SAR 2
#define SHI_ROL 3   // 0.8.0: donusumler (RCL/RCR desteklenmez -> RED)
#define SHI_ROR 4

// --- condition kodlari (x86 Jcc sirasi! JP/NP dahil) ---
#define CC_O  0
#define CC_NO 1
#define CC_B  2
#define CC_AE 3
#define CC_E  4
#define CC_NE 5
#define CC_BE 6
#define CC_A  7
#define CC_S  8
#define CC_NS 9
#define CC_P  10
#define CC_NP 11
#define CC_L  12
#define CC_GE 13
#define CC_LE 14
#define CC_G  15

// --- bayrak bitleri (VMContext.flags) ---
#define FCF 1u
#define FZF 2u
#define FSF 4u
#define FOF 8u

// --- register enc: 0..15 = x64 GPR sirasi ---
// rax rcx rdx rbx rsp rbp rsi rdi r8..r15
// 16..19 = ah ch dh bh (yuksek bayt); 8-bit yazmada kullanilir
#define REG_RAX 0
#define REG_RCX 1
#define REG_RDX 2
#define REG_RBX 3
#define REG_RSP 4
#define REG_RBP 5
#define REG_RSI 6
#define REG_RDI 7
#define REG_R8  8

// --- mem operand form'lari ---
#define MEMF_DISP 0   // addr = base + disp32(rva)   [rip-rel / abs32 donusturulmus]
#define MEMF_SIB  1   // base_reg, idx_reg, scale(0..3), disp32
#define MEMF_SIB_IMG 2  // MEMF_SIB gibi; disp32 IMAGE-RVA'dir -> c->base eklenir
                        // (x86 mutlak adresleme: [reg + VA] -> [reg + (VA-IB)])
#define SIB_NOBASE 0xFF

// --- VMContext offsetleri (engine stub'i ile BIREBIR ayni olmali) ---
#define VMOFF_BC        0x00
#define VMOFF_BCLEN     0x08
#define VMOFF_KEY       0x10
#define VMOFF_BASE      0x18
#define VMOFF_REGS      0x20      // 16 x 8 = 0x80
#define VMOFF_FLAGS     0xA0
#define VMOFF_PUSHAREA  0xA8
#define VMOFF_INITRSP   0xB0
#define VMOFF_EXTRAPOP  0xB8
#define VMOFF_DONE      0xC0
#define VMOFF_SCRATCH   0xC8      // 8 x 8
#define VMSIZE          0xE8      // toplam (align 0x10)

// --- XRT cfg sözleşme boyutu (0.6.1) ---
// xrt_core.c'deki XCFG_SIZE (0x48) ile BIREBIR ayni olmali. 0.6.0'da
// XCFG 0x40->0x48 büyütüldü ama engine rezervi 0x40'ta kalmisti: cfg'nin
// son 8B'si (pack_rva + reserved) ilk bytecode'un ilk 8 baytini eziyor,
// VM çöp opcode okuyor ve fonksiyon SESSIZCE no-op'a düşüyordu
// (kullanıcı kanıtı: metin2 ULTRA+loaderCRC = damage yok, crash yok).
#define XCFG_SIZE_E     0x60   // 0.8.0: +16B (unw 0x44/0x48, sectcrypt 0x4C/0x50/0x58 — xr_cipher.h)

// --- stub'in yigin rezervi (fonksiyonun kendi push/literal alani) ---
#define VM_CTX_SZ   0xE8
// Stub yigin duzeni (olcumlu):
//   x64: 15 push(0x78) + sub 0x330 (ctx 0xE8 + VM C cerceve marji) + call
//   x86: pushad(0x20) + sub 0x268 + push ctx(4) + call
// VM C cerceveleri olculecek kadar asagi inmez (x64 ~0x120, x86 ~0x90;
// 2x marj). Fonksiyonun mantiksal yazimlari C cercevelerinin ALTINA
// DELTA ile tasinir -> kesisme imkansiz.
#define VM_STUB_SUB64 0x330
#define VM_STUB_SUB86 0x268
#define VM_DELTA64  0xC00   /* 0.8.2: C-cagri derinligi buyudu (ANTIDBG API cagrilari) — emu-yigin C-copunun ALTINA tasinir */
#define VM_DELTA86  0xC00   /* 0.8.2: x86 da ayni derinlik sorunu — emu-yigin C-copunun altinda */

#endif
