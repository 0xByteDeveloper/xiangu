// ============================================================
// vm_opcodes.h — VM bayt-kod formatinin TEK kaynagi.
// Hem CPlusPort engine (encoder) hem vm_core.c (decoder) kullanir.
// Her byte rolling-LCG anahtari ile sifrelenir:
//   cipher = b ^ (key & 0xFF);  key = key*A + C
// ============================================================
#ifndef VM_OPCODES_H
#define VM_OPCODES_H

// Pozisyon-bazli keystream: sifre byte'i salt (anahtar, pozisyon) ciftinin
// fonksiyonudur. JMP/JCC ile akis icinde sicransa bile encoder/decoder
// uyumu bozulmaz (rolling-key tasarim hatasini onler).
#define VM_KS(key, pos) ((unsigned char)( \
    ((((unsigned long long)(key)) ^ ((unsigned long long)(pos)) * 0x9E3779B97F4A7C15ULL) \
      * 0x2545F4914F6CDD1DULL + 0x9E3779B97F4A7C15ULL) >> 56))

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
#define XCFG_SIZE_E     0x48

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
#define VM_DELTA64  0x600   /* 0x78+0x330+8(ret)+0x240(C-frame) -> align16 */
#define VM_DELTA86  0x420   /* 0x20+0x268+4+4(ret)+0x180(C-frame) -> align16 */

#endif
