// ============================================================
// engine.hpp — CPlusPort koruma motoru arayuzleri
// ============================================================
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "vm_opcodes.h"

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

// ---------------- PE ----------------
struct Section {
    char name[9];
    u32 va, vsz, rawoff, rawsz, chars;
};

struct PEFile {
    std::vector<u8> buf;
    bool is64 = false;
    u16 machine = 0;
    u32 imagebase = 0, entry_rva = 0;
    u64 ib64 = 0;   // tam 64-bit ImageBase (PE32+ icin dogru deger, log/rapor)
    u32 secalign = 0x1000, filealign = 0x200, sizeofimage = 0, sizeofheaders = 0;
    u32 symtab_off = 0; u32 symtab_n = 0; u32 strtab_off = 0;
    u32 e_lfanew = 0;
    std::vector<Section> secs;

    bool load(const std::string& path, std::string& err);
    bool save(const std::string& path);
    u32  rva2raw(u32 rva, bool* ok = nullptr) const;
    Section* secbyrva(u32 rva);
    Section* secbyname(const char* n);
    const Section* secbyname_c(const char* n) const;
    bool add_section(const char* name, const std::vector<u8>& data, u32 chars,
                     u32& out_rva);
    void fix_checksum();
    // BUGFIX #13: sanallastirilan fonksiyon bolgelerine dusen eski
    // .reloc girdilerini siler (ASLR'da loader stub baytlarini bozuyordu)
    bool filter_relocs(const std::vector<std::pair<u32,u32>>& ranges,
                       u32& removed, std::string& note);
    // export tablosu
    void exports(std::vector<std::pair<std::string, u32>>& out);
    // COFF sembol tablosu (isim -> rva), fonksiyon olanlar
    void coff_symbols(std::vector<std::pair<std::string, u32>>& out);
};

// ---------------- Disassembler ----------------
enum XOp {
    XP_UNKNOWN = 0, XP_NOP, XP_INT3, XP_PUSH_R, XP_POP_R, XP_MOV_MR, XP_MOV_RM,
    XP_MOV_MI, XP_MOV_RI, XP_MOV_RI64, XP_LEA, XP_ALU, XP_SHIFT, XP_IMUL_RRI,
    XP_IMUL_RR, XP_INCDEC_R, XP_INCDEC_M, XP_MOVZX, XP_MOVSX, XP_JMP, XP_JCC,
    XP_CALL, XP_RET, XP_RETI16, XP_SETCC, XP_CWDE, XP_CDQ, XP_IDIV, XP_TEST_RM, XP_PUSH_I, XP_LEAVE, XP_CMOV,
    XP_NEG, XP_NOT,
    XP_CALLI, XP_JMPI    // 0.5.0: dolayli cagri/atlayis (FF15/FF25) — IAT korumasi tarayicisi icin
};

struct MemOp {
    bool present = false;
    int  base = -1, idx = -1, scale = 0;
    long long disp = 0;
    bool riprel = false;      // x64 rip-relative
    bool absimage = false;    // x86 mutlak, image araliginda -> base+rva'ya cevrilir
    u64  absval = 0;
};

struct Insn {
    XOp op = XP_UNKNOWN;
    u32 pc = 0, len = 0;
    int sz = 4;               // 1,2,4,8
    int r1 = -1, r2 = -1;
    u64 imm = 0; int immsz = 0;
    int cc = 0, alu = 0, shi = 0, ud = 0;
    MemOp mem;
    u64 target = 0;           // jmp/jcc/call hedefi (VA)
    std::string text;         // okunur gosterim (log icin)
};

Insn disasm(const u8* p, int maxlen, bool is64, u32 pc, u64 imagebase, u32 imagesize);

// ---------------- Lifter ----------------
struct VFn {
    u32 rva = 0, size = 0;
    std::string name;
    bool ok = false;
    std::string why;
    std::vector<u8> plain;          // duz bayt-kod (sifreleme son adimda)
    std::vector<std::pair<u32, u32>> fix;   // (bc offset, hedef bc-ic)
    std::vector<std::string> notes;
    std::vector<u32> str_refs;      // .rdata string baslangic RVA'lari
    u32 retimm = 0xFFFFFFFF;        // x86 stdcall ret imm16
};

// lifter: fonksiyonun makine kodunu VM bayt-koduna cevirir.
// junk_level: 0=kapali, 2=mutation junk; junk_seed: deterministik uretim
// pad_terminates: 0.4.1 — true: ret sonrasi dolgu (nop/int3) govdeyi bitirir
// (tarayici fonksiyonlari; komsuya tasma korumasi). false: MUTANT girdi —
// mutasyon junk'i dolgu benzeri bayt uretebilir; yuruyus kesin boyuta kadar
// devam eder, bilinmeyen opcode sessiz kisalma YERINE sesli hata verir.
VFn lift_function(const u8* code, u32 rva, u32 size, bool is64,
                  u64 imagebase, u32 imagesize, bool allow_call,
                  u32 rdata_rva, u32 rdata_size, int junk_level, u32 junk_seed,
                  bool pad_terminates = true,
                  bool region_mode = false);   // 0.5.0: SDK/manual bolge — ret zorunlu degil

// ---------------- Mutasyon cekirdegi (0.3.0) ----------------
// VMProtect "Mutation" tipinin yerli uygulaması: fonksiyon makine
// kodunda kalir; talimat ikamesi + canlı-register analizi + JMP kapıları
// ile yeniden uretilir. Tum ikame/junk formlari aynı anda lifter-guvenli
// tutulur (Ultra = mutasyon + sanallastirma aynı akistan beslenir).
struct MutStats {
    u32 subst = 0, junk = 0, gates = 0;
    u32 orig_bytes = 0, new_bytes = 0;
    u32 nblocks = 0, ninsn = 0;
    u32 dead_spots = 0;    // junk sikistirilan olumlu nokta
};

struct MutFn {
    bool ok = false;
    std::string why;
    std::vector<u8> code;      // mutasyonlu native kod (base_rva tabanli)
    MutStats st;
    std::vector<std::string> notes;
    bool region_tail = false;  // 0.5.0: govde ret yok — sona region_end jmp eklendi
};

// strength: 1=hafif 2=normal 3=agir (ikame/junk/kapi olasiliklari)
// base_rva: uretilen kodun yerleştirileceği RVA (dis cagri/riprel görecleri
// buna göre hesaplanir; M modunda iki geçişte çağrılır: ölçüm + gerçek)
MutFn mutate_function(const u8* code, u32 rva, u32 size, bool is64,
                      u64 imagebase, u32 imagesize, bool allow_call,
                      int strength, u32 seed, u32 base_rva,
                      bool region_mode = false);  // 0.5.0: bolge — sona region_end'e jmp ekle

// ---------------- Stub ureteci ----------------
// x64: 15 reg push + ctx + call vm_entry(RDI) + geri
// x86: pushad + ctx + push ctxptr + call vm_entry(cdecl) + popad + ret[imm16]
// 0.5.0: xrt_rva/cfg_rva != 0 ise stub, vm_entry cagrisindan once xrt_entry
// cagrisi (bir-kez kilidi cfg->state) + zehir yolu (ud2) icerir.
std::vector<u8> build_stub_x64(u32 stub_rva, u32 vm_entry_rva, u64 key,
                               u32 bc_rva, u32 bc_len,
                               u32 xrt_entry_rva = 0, u32 cfg_rva = 0,
                               u32 cont_rva = 0);   // 0.5.0: region sonu (ret yerine jmp)
std::vector<u8> build_stub_x86(u32 stub_rva, u32 vm_entry_rva, u64 key,
                               u32 bc_rva, u32 bc_len, u32 retimm,
                               u32 xrt_entry_rva = 0, u32 cfg_rva = 0,
                               u32 cont_rva = 0);   // 0.5.0: region sonu (ret yerine jmp)

// 0.5.0: M-tipi (mutasyon) govde onu runtime kapisini (bir-kez xrt kontrolu).
// jmp bu kapiye atlar; kapi zehirse ud2, degilse govdeye atlar.
std::vector<u8> build_mgate_x86(u32 gate_rva, u32 xrt_entry_rva, u32 cfg_rva,
                                u32 body_rva);
std::vector<u8> build_mgate_x64(u32 gate_rva, u32 xrt_entry_rva, u32 cfg_rva,
                                u32 body_rva);

// 0.5.0: gecikmeli import thunk'i — IAT yuvasini hash ile cozumler,
// sonucu cache'hler ve hedefe atlar. Cozumlenemezse orijinal IAT degeri
// (loader'in yazdigi) okunur (fallback_abs).
std::vector<u8> build_thunk_x86(u32 thunk_rva, u32 resolve_rva, u32 cache_rva,
                                u32 dll_hash, u32 name_hash, u32 iat_rva);
std::vector<u8> build_thunk_x64(u32 thunk_rva, u32 resolve_rva, u32 cache_rva,
                                u32 dll_hash, u32 name_hash, u32 iat_rva);

// ---------------- Orkestrasyon ----------------
struct ManFn {               // 0.4.1: MANUEL fonksiyon (UI Add Function Base/End)
    u32 rva = 0, size = 0;   // Base ve End-Base (kesin boyut — tarayici tahmini degil)
    int  ty = 0;             // 0=V 1=M 2=U
    std::string name;        // log/dokumante icin (bos olabilir)
};

struct Options {
    int  level = 1;            // LEGACY 0.2.0: 1=Virt 2=UltraVirt+VMjunk (ftypes yoksa U fallback)
    int  default_type = 0;     // 0.3.0: 0=Virtualization 1=Mutation 2=Ultra (ftypes disi varsayilan)
    std::map<std::string,int> ftypes;  // 0.3.0: fonksiyon -> tip (anahtar opt.funcs formatıyla aynı)
    int  mut_strength = 2;     // 0.3.0: mutasyon gucu 1..3
    bool antidebug = false;
    bool strenc = false;
    bool memprot = false;
    bool vmenc = true;
    // --- 0.5.0 runtime katmani ---
    bool rt_selfcrc = false;   // motor kodu (.vmp0 vm+xrt blob) CRC
    bool rt_textcrc = false;   // .text bellek CRC (reloc-farkindalikli)
    bool rt_filecrc = false;   // diskteki dosya CRC (CheckSum+.vmp0 atla)
    bool rt_msg = true;        // hata: MessageBoxA goster
    bool rt_exit = true;       // hata: ExitProcess(1)
    bool iat_prot = false;     // import korumasi: cagri noktalarini thunk'a cevir
    bool sdk_scan = true;      // SDK isaretlerini otomatik kesfet
    bool sdk_only = false;     // 0.5.0: yalniz SDK/manual bolgeleri koru (--all/devredisi)
    std::vector<std::string> funcs;   // isim ya da rva(hex) listesi; bos -> hepsi uygun olanlar
    bool all = false;
    std::vector<ManFn> manual;        // 0.4.1: kesin {rva,boyut,tip} listesi — tarayici adayina bakilmaksizin derlenir
};

struct FnInfo {
    u32 rva = 0, size = 0;
    std::string name;
    bool selected = false;
    bool virtualized = false;
    bool exported = false;
    std::string note;
};

// koruma ana isi; log satirlarini log vektorune birikirir
int protect(const std::string& in, const std::string& out, const Options& opt,
            std::vector<std::string>& log);

// yardimcilar
std::string hexs(const u8* p, int n);
std::string hex32(u32 v);
const char* regname(bool is64, int enc);

extern const unsigned char VM_BLOB64[]; extern const int VM_BLOB64_LEN;
extern const unsigned char VM_BLOBX86[]; extern const int VM_BLOBX86_LEN;
extern const u32 VM_BLOB64_ENTRY_OFF;   // blob icinde vm_entry ofseti
extern const u32 VM_BLOBX86_ENTRY_OFF;
// 0.5.0: XProtect Runtime blob'u (xrt_core.c)
extern const unsigned char XRT_BLOB64[]; extern const int XRT_BLOB64_LEN;
extern const unsigned char XRT_BLOBX86[]; extern const int XRT_BLOBX86_LEN;
extern const u32 XRT_BLOB64_ENTRY_OFF;
extern const u32 XRT_BLOB64_RESOLVE_OFF;
extern const u32 XRT_BLOBX86_ENTRY_OFF;
extern const u32 XRT_BLOBX86_RESOLVE_OFF;
