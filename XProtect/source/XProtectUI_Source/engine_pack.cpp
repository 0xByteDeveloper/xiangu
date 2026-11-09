// ============================================================
// engine_pack.cpp — 0.6.0 cpPack: LZMA paketleme + strip fixups/debug
// Referans: VMProtect core/pefile.cc Save(cpPack) + runtime/loader.cc
// PACKER_INFO akisi (third-party/lzma LZMA SDK portu ile birebir).
//
// Duzen:
//   .vmp1 = [XPK1 baslik][kayitlar][props 5B][sikistirilmis akislar][stub]
//   Kayit: {src_rva, dst_rva, dst_size} — src: .vmp1 icinde akis RVA'si,
//   dst: sikistirilan bolum RVA'si, dst_size: orijinal rawsz.
//   Runtime: EP -> stub -> xrt_unpack (xrt_core.c) -> OEP.
//   Paketlenen: dizin tasimayan bolumler (import/TLS/export/reloc/debug/
//   loadcfg/IAT/CLR tasiyanlar + .vmp* muaf). ASLR temizlenir, dosya
//   compaction ile yigilir (gercek boyut kazanci).
// ============================================================
#include "engine.hpp"
#include <cstdlib>
#include <cstring>

extern "C" {
#include "lzma/LzmaEnc.h"
#include "lzma/LzmaDecode.h"   // LZMA_PROPERTIES_SIZE
}

// ---- LZMA SDK bellek saglayicilari (motor tarafi — malloc serbest) ----
static void* SzAllocP(void*, size_t n) { return malloc(n); }
static void  SzFreeP(void*, void* p) { free(p); }
static ISzAlloc g_EngineAlloc = { SzAllocP, SzFreeP };

static u32 pow2ceil(u32 v) {
    u32 p = 1u << 16;                       // en az 64K sozluk
    while (p < v && p < (1u << 26)) p <<= 1;
    return p;
}

// LZMA ile sikistir (birebir VMProtect altyapisi: LZMA1 + end-mark).
// dict: cagiran taraf belirler (tum akislar AYNI props ile sikistirilir —
// referanstaki tek-LzmaDecodeProperties davranisi).
bool lzma_compress(const u8* in, u32 inlen, u32 dict,
                   std::vector<u8>& out, u8 props[5]) {
    out.clear();
    if (!in || !inlen) return false;
    CLzmaEncProps p; LzmaEncProps_Init(&p);
    p.level = 9;
    p.dictSize = dict;
    p.writeEndMark = 1;                     // runtime dst_size tamamlama kontrolu
    LzmaEncProps_Normalize(&p);
    size_t cap = (size_t)inlen + inlen / 8 + 4096;
    std::vector<u8> dest(cap);
    SizeT destLen = (SizeT)dest.size();
    SizeT propsSize = LZMA_PROPERTIES_SIZE;
    SRes r = LzmaEncode(dest.data(), &destLen, in, (SizeT)inlen, &p,
                        props, &propsSize, 1, 0, &g_EngineAlloc, &g_EngineAlloc);
    if (r != 0 || propsSize != LZMA_PROPERTIES_SIZE) return false;
    out.assign(dest.data(), dest.data() + destLen);
    return true;
}

// ---------------- EP redirect stub uretecleri ----------------
// x64 (RIP-goreli cfg — ASLR-guvenli), 23 bayt:
//   NOT: xrt blob'u Linux gcc ile derlenir -> SysV ABI; ilk argiman RDI.
//   48 8D 3D rel32   lea rdi,[rip+cfg]
//   E8 rel32         call xrt_unpack
//   85 C0            test eax,eax
//   75 05            jne poison
//   E9 rel32         jmp oep
//   0F 0B            ud2 (poison)
std::vector<u8> build_packstub_x64(u32 stub_rva, u32 unpack_rva, u32 cfg_rva, u32 oep_rva) {
    std::vector<u8> c;
    auto w32 = [&](u32 v) { c.push_back((u8)v); c.push_back((u8)(v >> 8)); c.push_back((u8)(v >> 16)); c.push_back((u8)(v >> 24)); };
    c.push_back(0x48); c.push_back(0x8D); c.push_back(0x3D);
    w32(cfg_rva - (stub_rva + 7));
    c.push_back(0xE8); w32(unpack_rva - (stub_rva + 12));
    c.push_back(0x85); c.push_back(0xC0);
    c.push_back(0x75); c.push_back(0x05);
    c.push_back(0xE9); w32(oep_rva - (stub_rva + 21));
    c.push_back(0x0F); c.push_back(0x0B);
    return c;
}

// x86 (call $+5 / pop eax hilesi — ASLR-guvenli), 36 bayt:
//   E8 00000000      call $+5
//   58               pop eax            ; eax = stub+5
//   2D 05000000      sub eax,5          ; eax = stub
//   05 imm32         add eax, cfg-stub  ; eax = cfg VA
//   50               push eax
//   E8 rel32         call xrt_unpack
//   83 C4 04         add esp,4
//   85 C0            test eax,eax
//   75 05            jne poison
//   E9 rel32         jmp oep
//   0F 0B            ud2
std::vector<u8> build_packstub_x86(u32 stub_rva, u32 unpack_rva, u32 cfg_rva, u32 oep_rva) {
    std::vector<u8> c;
    auto w32 = [&](u32 v) { c.push_back((u8)v); c.push_back((u8)(v >> 8)); c.push_back((u8)(v >> 16)); c.push_back((u8)(v >> 24)); };
    c.push_back(0xE8); w32(0);
    c.push_back(0x58);
    c.push_back(0x2D); w32(5);
    c.push_back(0x05); w32(cfg_rva - stub_rva);
    c.push_back(0x50);
    c.push_back(0xE8); w32(unpack_rva - (stub_rva + 22));
    c.push_back(0x83); c.push_back(0xC4); c.push_back(0x04);
    c.push_back(0x85); c.push_back(0xC0);
    c.push_back(0x75); c.push_back(0x05);
    c.push_back(0xE9); w32(oep_rva - (stub_rva + 34));
    c.push_back(0x0F); c.push_back(0x0B);
    return c;
}

// ---------------- yardimci PE yazicilari ----------------
static u32 grd32(const std::vector<u8>& b, size_t o) { return (u32)b[o] | ((u32)b[o+1] << 8) | ((u32)b[o+2] << 16) | ((u32)b[o+3] << 24); }
static u16 grd16(const std::vector<u8>& b, size_t o) { return (u16)(b[o] | (b[o+1] << 8)); }
static void gwr32(std::vector<u8>& b, size_t o, u32 v) { b[o] = (u8)v; b[o+1] = (u8)(v >> 8); b[o+2] = (u8)(v >> 16); b[o+3] = (u8)(v >> 24); }
static void gwr16(std::vector<u8>& b, size_t o, u16 v) { b[o] = (u8)v; b[o+1] = (u8)(v >> 8); }

static u32 pk_opt(PEFile& pe) { return pe.e_lfanew + 4 + 20; }
static u32 pk_dd(PEFile& pe, int idx) { return pk_opt(pe) + (pe.is64 ? 112 : 96) + 8 * (u32)idx; }
static u32 pk_align(u32 v, u32 a) { return (v + a - 1) & ~(a - 1); }

// ---------------- cpStripFixups ----------------
// .reloc dizinini kaldirir + ASLR bayraklarini temizler (VMProtect:
// fixups derleme-zamaninda kesinlesir; yukleyici delta uygulamaz).
bool strip_fixups(PEFile& pe, std::vector<std::string>& log) {
    char b[256];
    u32 dd = pk_dd(pe, 5);   // directory 5 = BASERELOC
    u32 rva = grd32(pe.buf, dd), sz = grd32(pe.buf, dd + 4);
    if (rva && sz) {
        for (auto& s : pe.secs) {
            if (rva >= s.va && rva < s.va + s.vsz) {
                bool ok; u32 raw = pe.rva2raw(rva, &ok);
                if (ok && raw + sz <= pe.buf.size()) memset(&pe.buf[raw], 0, sz);
                snprintf(b, sizeof b, "[STRIPFIX] .reloc dizini silindi (%u bayt)", sz);
                log.push_back(b);
                break;
            }
        }
        gwr32(pe.buf, dd, 0); gwr32(pe.buf, dd + 4, 0);
    }
    u16 dc = grd16(pe.buf, pk_opt(pe) + 70);
    if (dc & 0x0060) {
        dc &= (u16)~0x0060;
        gwr16(pe.buf, pk_opt(pe) + 70, dc);
        log.push_back("[STRIPFIX] DYNAMIC_BASE/HIGH_ENTROPY_VA temizlendi (ASLR kapali)");
    }
    return true;
}

// ---------------- cpStripDebugInfo ----------------
bool strip_debug(PEFile& pe, std::vector<std::string>& log) {
    char b[256];
    u32 dd = pk_dd(pe, 6);   // directory 6 = DEBUG
    u32 rva = grd32(pe.buf, dd), sz = grd32(pe.buf, dd + 4);
    if (rva && sz) {
        bool ok; u32 raw = pe.rva2raw(rva, &ok);
        if (ok && raw + sz <= pe.buf.size()) memset(&pe.buf[raw], 0, sz);
        snprintf(b, sizeof b, "[STRIPDBG] debug dizini silindi (%u bayt)", sz);
        log.push_back(b);
        gwr32(pe.buf, dd, 0); gwr32(pe.buf, dd + 4, 0);
    }
    u32 fh = pe.e_lfanew + 4;
    if (pe.symtab_n && pe.symtab_off) {
        snprintf(b, sizeof b, "[STRIPDBG] COFF sembol tablosu cikarildi (%u kayit)", pe.symtab_n);
        log.push_back(b);
        gwr32(pe.buf, fh + 8, 0);
        gwr32(pe.buf, fh + 12, 0);
        pe.symtab_off = 0; pe.symtab_n = 0; pe.strtab_off = 0;
    }
    return true;
}

// ---------------- cpPack: bolum paketleme + dosya sikistirma ----------------
// donus: 0 = tamam (pack_rva cfg'ye yazilir), >0 = red (err'de neden)
int do_pack(PEFile& pe, u32 cfg_rva, u32 unpack_rva,
            std::vector<std::string>& log, u32& pack_rva_out, std::string& err) {
    char b[320];
    pack_rva_out = 0;
    u32 opto = pk_opt(pe);
    const u32 size_before = (u32)pe.buf.size();

    // --- on-kosullar (durusluk: red edilirse NET neden) ---
    u16 fchars = grd16(pe.buf, pe.e_lfanew + 4 + 18);   // FileHeader.Characteristics
    if (fchars & 0x2000) { err = "DLL dosyalari 0.6.0'da paketlenmez (reloc/tam-loader 0.9.0'da)"; return 1; }
    // TLS: callback VARSA red (EP oncesi calisir, .text sikisissa crash).
    // Callback listesi bos/sifirsa paketleme serbest (metin2 davranisi);
    // .tls bolumu her durumda paketlenmez (asagida aday filtresi).
    {
        u32 tlrva = grd32(pe.buf, pk_dd(pe, 9));
        u32 tlsz  = grd32(pe.buf, pk_dd(pe, 9) + 4);
        if (tlrva && tlsz) {
            bool ok; u32 traw = pe.rva2raw(tlrva, &ok);
            if (ok && traw + tlsz <= pe.buf.size()) {
                u32 cboff = pe.is64 ? 24 : 12;   // AddressOfCallBacks alani
                // x64: 8 bayt VA; x86: 4 bayt VA (ImageBase ile RVA'ya cevir)
                u64 cbva = pe.is64
                    ? ((u64)grd32(pe.buf, traw + cboff) | ((u64)grd32(pe.buf, traw + cboff + 4) << 32))
                    : (u64)grd32(pe.buf, traw + cboff);
                u64 ib = pe.ib64 ? pe.ib64 : (u64)pe.imagebase;
                if (cbva > ib && cbva - ib < 0x10000000u) {
                    u32 cbrva = (u32)(cbva - ib);
                    u32 cbraw = 0;
                    bool okc; cbraw = pe.rva2raw(cbrva, &okc);
                    if (okc) {
                        u32 entsz = pe.is64 ? 8 : 4;
                        // en fazla 64 kayit tara; hepsi 0 ise callback yok sayilir
                        bool has_cb = false;
                        for (u32 k = 0; k < 64; k++) {
                            u32 o = cbraw + k * entsz;
                            if (o + entsz > pe.buf.size()) break;
                            u64 v = pe.is64 ? (u64)grd32(pe.buf, o) | ((u64)grd32(pe.buf, o + 4) << 32)
                                            : (u64)grd32(pe.buf, o);
                            if (v) { has_cb = true; break; }
                            if (!v && k > 0) break;   // sonlandirici 0
                        }
                        if (has_cb) { err = "TLS callback'leri EP'den once calisir (paketlenmis .text crash olur); tam-loader 0.9.0'da desteklenecek"; return 1; }
                    }
                }
            }
        }
    }
    if (grd32(pe.buf, pk_dd(pe, 14) + 4)) { err = ".NET (CLR) dosyalari paketlenmez"; return 1; }
    if (grd32(pe.buf, pk_dd(pe, 4) + 4)) { err = "imzali (Authenticode) dosya paketlenmez"; return 1; }
    u32 oep = grd32(pe.buf, opto + 16);
    if (!oep) { err = "giris noktasi (EP) 0 — paketlenemez"; return 1; }

    // --- dizin sahibi bolumleri koru ---
    // 0 EXPORT,1 IMPORT,2 RESOURCE,3 EXCEPTION,5 RELOC,6 DEBUG,7 ARCH,
    // 9 TLS,10 LOADCFG,13 IAT,14 CLR,15 DELAY
    const int dirs[] = { 0,1,2,3,5,6,7,9,10,13,14,15 };
    std::vector<int> skip(pe.secs.size(), 0);
    for (int di : dirs) {
        u32 rva = grd32(pe.buf, pk_dd(pe, di));
        u32 sz  = grd32(pe.buf, pk_dd(pe, di) + 4);
        if (!rva || !sz) continue;
        for (size_t i = 0; i < pe.secs.size(); i++)
            if (rva >= pe.secs[i].va && rva < pe.secs[i].va + pe.secs[i].vsz) skip[i] = 1;
    }

    // --- ORTAK props: en buyuk bolume gore sozluk (referanstaki tek-
    //     LzmaDecodeProperties davranisi) ---
    u32 maxsec = 0;
    for (size_t i = 0; i < pe.secs.size(); i++) {
        Section& s = pe.secs[i];
        if (skip[i]) continue;
        if (strncmp(s.name, ".vmp", 4) == 0) continue;
        if (strncmp(s.name, ".tls", 4) == 0) continue;   // TLS sablonu hic paketlenmez
        if (!s.rawsz || !s.vsz) continue;
        if (s.rawsz > maxsec) maxsec = s.rawsz;
    }
    if (!maxsec) { err = "paketlenecek bolum yok (tum bolumler dizin tasiyor/muaf)"; return 1; }
    u32 dict = pow2ceil(maxsec);

    // --- adaylari sikistir ---
    struct Reg { u32 dst_rva, dst_size, src_off; std::vector<u8> stream; };
    std::vector<Reg> regs;
    u8 props[5]; bool props_ok = false;
    for (size_t i = 0; i < pe.secs.size(); i++) {
        Section& s = pe.secs[i];
        if (skip[i]) continue;
        if (strncmp(s.name, ".vmp", 4) == 0) continue;
        if (strncmp(s.name, ".tls", 4) == 0) continue;   // TLS sablonu hic paketlenmez
        if (!s.rawsz || !s.vsz) continue;
        bool ok; u32 raw = pe.rva2raw(s.va, &ok);
        if (!ok || (u64)raw + s.rawsz > pe.buf.size()) continue;
        Reg r; r.dst_rva = s.va; r.dst_size = s.rawsz; r.src_off = 0;
        if (!lzma_compress(&pe.buf[raw], s.rawsz, dict, r.stream, props)) {
            snprintf(b, sizeof b, "[PACK] %s sikistirilamadi — muaf tutuldu", s.name);
            log.push_back(b);
            continue;
        }
        props_ok = true;
        regs.push_back(std::move(r));
        snprintf(b, sizeof b, "[PACK] %s RVA=0x%08X %u B -> LZMA %zu B (%%%.1f)", s.name, s.va, s.rawsz, regs.back().stream.size(), 100.0 * regs.back().stream.size() / s.rawsz);
        log.push_back(b);
    }
    if (regs.empty() || !props_ok) { err = "hicbir bolum sikistirilamadi"; return 1; }

    // --- .vmp1 verisini kur ---
    u32 cur = pk_align(16 + (u32)regs.size() * 12 + 5, 16);
    std::vector<u8> data;
    auto put32 = [&](u32 v) { data.push_back((u8)v); data.push_back((u8)(v>>8)); data.push_back((u8)(v>>16)); data.push_back((u8)(v>>24)); };
    put32(0x31504B58u);           // 'XPK1'
    put32((u32)regs.size());
    put32(0);                     // props_rva (RVA'lari bolum RVA'si bilinince yazariz)
    put32(0);                     // rezerve
    for (auto& r : regs) { put32(0); put32(r.dst_rva); put32(r.dst_size); }  // src placeholder
    data.resize(pk_align((u32)data.size(), 16), 0);
    u32 props_off = (u32)data.size();
    data.insert(data.end(), props, props + 5);
    data.resize(pk_align((u32)data.size(), 16), 0);
    for (auto& r : regs) {
        data.resize(pk_align((u32)data.size(), 16), 0);
        r.src_off = (u32)data.size();
        data.insert(data.end(), r.stream.begin(), r.stream.end());
    }
    data.resize(pk_align((u32)data.size(), 16), 0);
    u32 stub_off = (u32)data.size();
    data.resize(stub_off + 48, 0);            // stub icin sabit 48B yer (sonra yazilir)

    u32 vmp1_rva = 0;
    if (!pe.add_section(".vmp1", data, 0xE00000E0u /*RWX*/, vmp1_rva)) {
        err = ".vmp1 bolumu eklenemedi"; return 1;
    }
    // RVA'lari cfg/records icine yaz
    {
        bool ok; u32 praw = pe.rva2raw(vmp1_rva, &ok);
        if (!ok) { err = ".vmp1 raw erisilemiyor"; return 1; }
        gwr32(pe.buf, praw + 8, vmp1_rva + props_off);
        for (size_t i = 0; i < regs.size(); i++)
            gwr32(pe.buf, praw + 16 + (u32)i * 12, vmp1_rva + regs[i].src_off);
    }
    // stub'i gercek RVA'lerle uret ve yerlestir
    u32 stub_rva = vmp1_rva + stub_off;
    {
        std::vector<u8> st = pe.is64 ? build_packstub_x64(stub_rva, unpack_rva, cfg_rva, oep)
                                     : build_packstub_x86(stub_rva, unpack_rva, cfg_rva, oep);
        st.resize(48, 0xCC);
        bool ok; u32 sraw = pe.rva2raw(stub_rva, &ok);
        if (!ok) { err = "stub raw erisilemiyor"; return 1; }
        memcpy(&pe.buf[sraw], st.data(), st.size());
    }
    // EP -> stub
    gwr32(pe.buf, opto + 16, stub_rva);
    snprintf(b, sizeof b, "[PACK] EP 0x%X -> unpack stub 0x%X (.vmp1) | OEP 0x%X stub'e gomuldu", oep, stub_rva, oep);
    log.push_back(b);

    // --- ASLR temizle (fixups artik yukleyicide uygulanmaz) ---
    {
        u16 dc = grd16(pe.buf, opto + 70);
        dc &= (u16)~0x0060;
        gwr16(pe.buf, opto + 70, dc);
        log.push_back("[PACK] DYNAMIC_BASE/HIGH_ENTROPY_VA temizlendi (sabit taban)");
    }

    // --- COFF sembolleri (compaction sonrasi ofset gecersiz kalir) ---
    if (pe.symtab_n && pe.symtab_off) {
        u32 fh2 = pe.e_lfanew + 4;
        gwr32(pe.buf, fh2 + 8, 0); gwr32(pe.buf, fh2 + 12, 0);
        pe.symtab_off = 0; pe.symtab_n = 0; pe.strtab_off = 0;
        log.push_back("[PACK] COFF sembol tablosu cikarildi");
    }

    // --- bolum rawsz'lerini sikismis boyuta cek + DOSYA COMPACTION ---
    {
        u32 fh = pe.e_lfanew + 4;
        u16 optsize = grd16(pe.buf, fh + 16);
        u32 sh = fh + 20 + optsize;
        for (auto& r : regs) {
            for (size_t i = 0; i < pe.secs.size(); i++) {
                if (pe.secs[i].va == r.dst_rva) {
                    u32 newraw = pk_align((u32)r.stream.size(), pe.filealign);
                    gwr32(pe.buf, sh + (u32)i * 40 + 16, newraw);
                    pe.secs[i].rawsz = newraw;
                }
            }
        }
        u32 ns = (u32)pe.secs.size();
        u32 headers_end = sh + ns * 40;
        u32 cursor = pk_align(headers_end, pe.filealign);
        std::vector<u32> newoff(ns, 0);
        for (u32 i = 0; i < ns; i++) {
            if (!pe.secs[i].rawsz) continue;
            cursor = pk_align(cursor, pe.filealign);
            newoff[i] = cursor;
            cursor += pe.secs[i].rawsz;
        }
        std::vector<u8> nb((size_t)cursor, 0);
        memcpy(&nb[0], &pe.buf[0], headers_end);
        for (u32 i = 0; i < ns; i++) {
            if (!newoff[i]) continue;
            Section& s = pe.secs[i];
            // 0.6.0 KRITIK: paketlenen bolumlere ORIJINAL baytlar degil,
            // SIKISTIRILMIS akis yazilir (rawsz zaten sikismis boyuta cekildi).
            bool is_packed = false;
            for (auto& r : regs) {
                if (r.dst_rva == s.va) {
                    memcpy(&nb[newoff[i]], r.stream.data(), r.stream.size());
                    is_packed = true;
                    break;
                }
            }
            if (!is_packed && (u64)s.rawoff + s.rawsz <= pe.buf.size())
                memcpy(&nb[newoff[i]], &pe.buf[s.rawoff], s.rawsz);
            gwr32(nb, sh + i * 40 + 20, newoff[i]);
            s.rawoff = newoff[i];
        }
        pe.buf.swap(nb);
        u32 newsoh = pk_align(headers_end, pe.filealign);
        gwr32(pe.buf, opto + 60, newsoh);
        pe.sizeofheaders = newsoh;
        snprintf(b, sizeof b, "[PACK] dosya compaction: %u -> %u bayt", size_before, (u32)pe.buf.size());
        log.push_back(b);
    }
    pack_rva_out = vmp1_rva;
    err.clear();
    return 0;
}
