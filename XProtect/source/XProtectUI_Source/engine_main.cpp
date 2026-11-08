// ============================================================
// engine_main.cpp — koruma orkestrasyonu
// Akis: PE yukle -> fonksiyonlari bul (export+COFF+PDB/MAP) ->
//       sec -> TIP SEVKI (0.3.0: V=sanallastirma, M=mutasyon,
//       U=mutasyon+sanallastirma) -> STRDEC/state planla ->
//       sifrele -> .vmp0 kur (blob+bc+stub+mut+state) -> .text yamala ->
//       secenekler (antidebug/strenc/memprot) -> checksum -> kaydet
// ============================================================
#include "engine.hpp"
#include "engine_sym.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <map>
#include <string>
#include <vector>

struct RStr { u32 rva, len; };

// Fonksiyonun GERCEK sonunu tara: ret + dolgu (int3/nop) -> dur.
// COFF sembolu olmayan son fonksiyonlarda 0xCC firtinasi onlenir.
static u32 est_func_size(const u8* p, u32 max, bool is64, u64 ib, u32 isz) {
    u32 pc = 0; bool sawret = false;
    while (pc < max) {
        Insn I = disasm(p + pc, (int)(max - pc), is64, pc, ib, isz);
        if (I.op == XP_UNKNOWN || I.len == 0 || I.op == XP_INT3)
            return sawret ? pc : max;
        if (I.op == XP_NOP && sawret) return pc;
        if (I.op == XP_NOP && !sawret) { pc += I.len; continue; }
        pc += I.len;
        if (I.op == XP_RET || I.op == XP_RETI16) sawret = true;
    }
    return pc;
}

// .pdata (x64): RUNTIME_FUNCTION tablosundan fonksiyonun KESIN sonunu bul.
// Donus: fonksiyon sonu RVA'si (bulunamazsa 0).
static u32 pdata_func_end(const PEFile& pe, u32 rva) {
    const Section* pd = pe.secbyname_c(".pdata");
    if (!pd || pd->vsz < 12) return 0;
    bool ok; u32 raw = pe.rva2raw(pd->va, &ok);
    if (!ok) return 0;
    u32 n = pd->vsz / 12;
    // RUNTIME_FUNCTION dizisi BeginAddress'e gore siralidir -> ikili arama
    u32 lo = 0, hi = n;
    while (lo < hi) {
        u32 mid = (lo + hi) / 2;
        u32 o = raw + mid * 12;
        if (o + 12 > pe.buf.size()) return 0;
        u32 beg; memcpy(&beg, &pe.buf[o], 4);
        if (beg == rva) {
            u32 end; memcpy(&end, &pe.buf[o + 4], 4);
            return (end > beg) ? end : 0;
        }
        if (beg < rva) lo = mid + 1; else hi = mid;
    }
    return 0;
}

static void scan_strings(const PEFile& pe, std::vector<RStr>& out) {
    const Section* rd = pe.secbyname_c(".rdata");
    if (!rd) return;
    bool ok; u32 raw = pe.rva2raw(rd->va, &ok);
    if (!ok) return;
    u32 run = 0;
    for (u32 i = 0; i < rd->vsz && raw + i < pe.buf.size(); i++) {
        u8 c = pe.buf[raw + i];
        if (c >= 0x20 && c < 0x7F) { run++; continue; }
        if (run >= 4 && c == 0) out.push_back({ rd->va + i - run, run });
        run = 0;
    }
}

int protect(const std::string& in, const std::string& out, const Options& opt,
            std::vector<std::string>& log) {
    auto L = [&](const std::string& s) { log.push_back(s); };
    char b[300];

    PEFile pe; std::string err;
    if (!pe.load(in, err)) { L("[HATA] " + err); return 1; }
    snprintf(b, sizeof b, "[PE] %s | Machine=0x%04X | %s | ImageBase=0x%llX | GirisRVA=0x%X",
             in.c_str(), pe.machine, pe.is64 ? "PE32+(x64)" : "PE32(x86)",
             (unsigned long long)pe.ib64, pe.entry_rva);
    L(b);
    for (auto& s : pe.secs) {
        snprintf(b, sizeof b, "  sec %-8s RVA=0x%06X VSz=0x%06X Raw=0x%06X Chars=0x%08X",
                 s.name, s.va, s.vsz, s.rawoff, s.chars);
        L(b);
    }

    // --- fonksiyon kesfi: export + COFF sembolleri ---
    std::map<u32, std::string> fnames;
    std::vector<std::pair<std::string, u32>> ex, cs;
    pe.exports(ex);
    pe.coff_symbols(cs);
    for (auto& e : ex) fnames[e.second] = e.first;
    for (auto& c : cs) if (!fnames.count(c.second)) fnames[c.second] = c.first;
    L("[KESIF] export=" + std::to_string(ex.size()) + " coff-fonksiyon=" + std::to_string(cs.size()));

    // --- 0.3.0: PDB / MAP fonksiyon isimleri (VMProtect davranisi) ---
    // Hedefin yaninda <ad>.pdb/.map varsa veya debug dizini gecerli bir PDB
    // gosteriyorsa isimler keşfe eklenir. HICBIRI yoksa bu blogun hicbir
    // etkisi yoktur — 0.2.0'nin kullanici-dogrulanmis cikti davranisi
    // bayt bayt korunur (Test.exe pakette pdb'siz birlikte gelir).
    size_t sym_added = 0;
    {
        SymSections ss;
        for (auto& s : pe.secs)
            ss.secs.push_back({s.va, s.vsz, s.rawsz, s.rawoff, s.chars, std::string(s.name)});
        auto slurp_n = [](const std::string& p, std::vector<u8>& out) -> bool {
            FILE* f = fopen(p.c_str(), "rb");
            if (!f) return false;
            fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
            if (n <= 0) { fclose(f); return false; }
            out.resize((size_t)n);
            size_t rd = fread(out.data(), 1, (size_t)n, f);
            fclose(f);
            return rd == (size_t)n;
        };
        // hedef klasoru + ad (dar yol)
        std::string dir = ".", base;
        {
            size_t q = in.find_last_of("\\/");
            if (q != std::string::npos) { dir = in.substr(0, q); base = in.substr(q + 1); }
            else base = in;
            size_t d = base.find_last_of('.');
            if (d != std::string::npos && d > 0) base.resize(d);
        }
        std::vector<std::string> cands;
        cands.push_back(dir + "\\" + base + ".pdb");
        std::vector<DbgPdbRef> dbg_names;
        pe_debug_pdb_names(pe.buf.data(), pe.buf.size(), ss, dbg_names);
        const su8* want_guid = NULL; su32 want_age = 0;
        if (!dbg_names.empty() && dbg_names[0].has_guid) {
            want_guid = dbg_names[0].guid; want_age = dbg_names[0].age;
        }
        for (auto& r : dbg_names) {
            const std::string& n = r.name;
            bool abs_ = (n.size() > 1 && (n[1] == ':' || n[0] == '\\' || n[0] == '/'));
            if (abs_) cands.push_back(n);
            size_t q = n.find_last_of("\\/");
            cands.push_back(dir + "\\" + (q != std::string::npos ? n.substr(q + 1) : n));
        }
        // dedupe
        for (size_t i = 0; i < cands.size(); i++)
            for (size_t j = i + 1; j < cands.size(); )
                if (cands[j] == cands[i]) cands.erase(cands.begin() + j); else j++;
        std::vector<SymEntry> sfns; std::string serr;
        for (auto& c : cands) {
            std::vector<u8> pd;
            if (!slurp_n(c, pd)) continue;
            if (want_guid && !pdb_matches(pd.data(), pd.size(), want_guid, want_age))
                continue;   // bayat/eslesmeyen PDB — ele
            if (pdb_parse(pd.data(), pd.size(), ss, sfns, serr)) break;
            sfns.clear();
        }
        // MAP
        {
            std::vector<u8> md;
            if (slurp_n(dir + "\\" + base + ".map", md)) {
                std::vector<SymEntry> mfns;
                if (map_parse((const char*)md.data(), md.size(), ss, pe.imagebase, mfns, serr))
                    for (auto& e : mfns) sfns.push_back(e);
            }
        }
        for (auto& e : sfns) {
            if (fnames.count(e.rva)) continue;
            fnames[e.rva] = e.name;
            sym_added++;
        }
        if (sym_added)
            L("[KESIF] +PDB/MAP=" + std::to_string(sym_added) + " (pdb/map yaninda/hata-ayiklama dizininde)");
    }

    std::vector<FnInfo> fns;
    {
        Section* text0 = pe.secbyname(".text");
        u32 textend = text0 ? text0->va + text0->vsz : 0;
        std::vector<u32> rv;
        for (auto& kv : fnames) rv.push_back(kv.first);
        std::sort(rv.begin(), rv.end());
        for (size_t i = 0; i < rv.size(); i++) {
            u32 r = rv[i];
            if (!text0 || r < text0->va || r >= textend) continue;
            u32 next_bound = (i + 1 < rv.size() ? rv[i + 1] : textend);
            u32 sz = next_bound - r;
            if (sz > 0x8000) sz = 0x8000;
            // x64: .pdata'daki RUNTIME_FUNCTION KESIN siniri verir —
            // (BU, onceki surumdeki "est+16 tasma -> sonraki fonksiyonu
            //  int3 ile ezip acilista crash" hatasini kokten engeller)
            if (pe.is64) {
                u32 pend = pdata_func_end(pe, r);
                if (pend > r && pend <= next_bound) sz = pend - r;
            }
            // sembol araligi zenginse guven; genis aralikta gercek kod sonunu tara
            {
                bool okraw; u32 raw = pe.rva2raw(r, &okraw);
                if (okraw && raw + sz <= pe.buf.size() && sz > 16) {
                    u32 est = est_func_size(&pe.buf[raw], sz, pe.is64, pe.imagebase, pe.sizeofimage);
                    if (est + 64 < sz) sz = est + 16;   // tarayici daha erken bitirdi
                }
            }
            // KESIN GUVENLIK: sonraki sembolun icine asla tasma (x86 + pdata'siz durumlar)
            if (r + sz > next_bound) sz = next_bound - r;
            if (sz < 8) continue;
            FnInfo f; f.rva = r; f.size = sz; f.name = fnames[r];
            f.exported = std::any_of(ex.begin(), ex.end(), [&](auto& e) { return e.second == r; });
            fns.push_back(f);
        }
    }
    snprintf(b, sizeof b, "[FONKSIYON] %zu aday (.text)", fns.size());
    L(b);

    // --- 0.4.1: MANUEL fonksiyonlar (UI Manual Address Base/End) ---
    // Kullanici Base/End ile kesin {rva, boyut, tip} verir. 0.4.0'a kadar bu
    // liste motora hic ulasmadigindan (yalnizca tarayici adaylari eslestirilirdi)
    // ULTRA (M+V) secilse bile fonksiyona DOKUNULMIYORDU (.vmp0 yalnizca VM blob
    // tasiyordu). Artik aday-aramadan bagimsiz derlenir; ayni RVA'daki tarayici
    // adayi kesin boyut lehine cikarilir.
    for (auto& m : opt.manual) {
        Section* ms = NULL;
        for (auto& s : pe.secs)
            if ((s.chars & 0x20000000) && m.rva >= s.va && m.rva < s.va + s.vsz) { ms = &s; break; }
        if (!ms) { L("[MANUEL] RED: 0x" + hex32(m.rva) + " yurutulebilir bolumde degil"); continue; }
        u32 sz = m.size;
        if (!sz || sz > 0x100000) { L("[MANUEL] RED: gecersiz boyut 0x" + hex32(sz)); continue; }
        if (m.rva + sz > ms->va + ms->vsz) {
            sz = (ms->va + ms->vsz) - m.rva;
            L("[MANUEL] uyari: End bolum sonunu asti -> 0x" + hex32(sz) + " kirpildi");
        }
        bool replaced = false;
        for (size_t i = 0; i < fns.size(); ) {
            if (fns[i].rva == m.rva) { fns.erase(fns.begin() + i); replaced = true; }
            else i++;
        }
        FnInfo f; f.rva = m.rva; f.size = sz;
        f.name = m.name.empty() ? ("MANUAL_0x" + hex32(m.rva)) : m.name;
        f.selected = true;
        fns.push_back(f);
        snprintf(b, sizeof b, "[MANUEL] %s @RVA=0x%06X boyut=0x%X tip=%s%s",
                 f.name.c_str(), f.rva, sz,
                 m.ty == 0 ? "V" : (m.ty == 1 ? "M" : "U"),
                 replaced ? " (tarayici adayi degisti)" : "");
        L(b);
    }

    // --- secim ---
    std::set<u32> man_rva; std::map<u32,int> man_ty;   // 0.4.1: manuel fonksiyonlar
    for (auto& m : opt.manual) { man_rva.insert(m.rva); man_ty[m.rva] = m.ty; }
    for (auto& f : fns) {
        if (man_rva.count(f.rva)) continue;   // manuel: kesin secili (asla ezilmez)
        if (opt.manual.empty() && (opt.all || opt.funcs.empty())) { f.selected = (f.size >= 24 && f.size <= 8192); continue; }
        for (auto& want : opt.funcs) {
            if (want == f.name) { f.selected = true; break; }
            u32 rv = (u32)strtoul(want.c_str(), nullptr, 16);
            if (rv && rv == f.rva) { f.selected = true; break; }
        }
    }

    // --- 0.3.0: TIP SEVKI (V/M/U) — ftypes anahtarlari opt.funcs
    // ile ayni formattadir (isim ya da buyuk-harf RVA-hex) ---
    auto ftype_of = [&](const FnInfo& f) -> int {
        auto itm = man_ty.find(f.rva);          // 0.4.1: manuel tip onceligi
        if (itm != man_ty.end()) return itm->second;
        auto it = opt.ftypes.find(f.name);
        if (it != opt.ftypes.end()) return it->second;
        char rb[24]; snprintf(rb, sizeof rb, "%X", f.rva);
        it = opt.ftypes.find(rb);
        if (it != opt.ftypes.end()) return it->second;
        return opt.default_type;
    };

    Section* text = pe.secbyname(".text");
    Section* rdata = pe.secbyname(".rdata");
    u32 rdata_rva = rdata ? rdata->va : 0;
    u32 rdata_sz  = rdata ? rdata->vsz : 0;
    std::vector<RStr> strings;
    if (opt.strenc) {
        scan_strings(pe, strings);
        snprintf(b, sizeof b, "[STRENC] .rdata'da %zu string tespit", strings.size());
        L(b);
    }

    // --- lift / mutasyon ---
    struct Compiled {
        FnInfo* f; VFn v; std::vector<u8> plain, enc;
        u64 key = 0; u32 bc_rva = 0, stub_rva = 0;
        bool tramp = false;
        int  ty = 0;                          // 0=V 1=M 2=U
        MutFn mut;                           // M: olcum gecisi; U: kaynak mutasyon
        u32  mut_rva = 0;                    // M: .vmp0 icindeki yerlesim
        std::vector<u32> state_sites;         // plain icinde state rva u32 pozisyonlari
        std::vector<u32> state_strva;         // sirasiyla string rva'lari
    };
    std::vector<Compiled> comps;
    int nV = 0, nM = 0, nU = 0, nUf = 0;
    for (auto& f : fns) {
        if (!f.selected) continue;
        bool okraw; u32 raw = pe.rva2raw(f.rva, &okraw);
        if (!okraw || raw + f.size > pe.buf.size()) { f.note = "ham veri yok"; continue; }
        int ty = ftype_of(f);
        if (ty == 1) {
            // --- MUTASYON (native): makine kodu kalir, yeniden uretilir ---
            MutFn m = mutate_function(&pe.buf[raw], f.rva, f.size, pe.is64,
                                      pe.imagebase, pe.sizeofimage,
                                      true /* 0.4.1: x86 dis-cagri rel32 fixup zaten destekli */,
                                      opt.mut_strength,
                                      f.rva ^ 0x9E3779B9u, 0 /*olcum gecisi*/);
            if (!m.ok) { f.note = "RED: mutasyon " + m.why; continue; }
            Compiled c; c.f = &f; c.ty = 1; c.mut = m;
            comps.push_back(c);
            nM++;
            continue;
        }
        // --- V / U: lifter kaynagi (U'da mutasyon on gecisi) ---
        std::vector<u8> mbuf;
        const u8* lcode = &pe.buf[raw];
        u32 lsize = f.size;
        int jl;
        MutFn mkeep;                            // U: istatistik/kayit tut
        if (ty == 2) {
            MutFn m = mutate_function(&pe.buf[raw], f.rva, f.size, pe.is64,
                                      pe.imagebase, pe.sizeofimage,
                                      true /* 0.4.1: x86 dis-cagri rel32 fixup zaten destekli */,
                                      opt.mut_strength,
                                      f.rva ^ 0x9E3779B9u, f.rva /*base=fonksiyon RVA*/);
            if (m.ok) {
                mbuf = m.code; lcode = mbuf.data(); lsize = (u32)mbuf.size();
                mkeep = m;
                nU++;
            } else {
                // mutasyon mumkun degil: 0.2.0 U davranisina dus (VM junk)
                L("[ULTRA] on-mutasyon RED: " + m.why + " -> saf V ye dusuluyor");
                ty = 0; nUf++;
            }
            jl = 2;
        } else {
            // V: acik tip 0 => saf sanallastirma (junk yok).
            // ftypes yoksa 0.2.0 CLI uyumu: level>=2 -> VM junk
            jl = opt.ftypes.empty() ? (opt.level >= 2 ? 2 : 0) : 0;
            nV++;
        }
        VFn v = lift_function(lcode, f.rva, lsize, pe.is64,
                              pe.imagebase, pe.sizeofimage,
                              true /* 0.4.1: x86 natcall eklendi — call iceren fonksiyonlar da sanallastirilir */,
                              rdata_rva, rdata_sz,
                              jl, f.rva ^ 0x9E3779B9u,
                              mbuf.empty() /* 0.4.1: mutant girdide dolgu akisi kesmez */);
        if (!v.ok) { f.note = "RED: " + v.why; continue; }
        Compiled c; c.f = &f; c.ty = ty; c.v = v;
        if (ty == 2) c.mut = mkeep;
        comps.push_back(c);
    }
    snprintf(b, sizeof b, "[LIFT] %zu/%zu fonksiyon | V=%d M=%d U=%d (U-mutdus=%d)",
             comps.size(), fns.size(), nV, nM, nU, nUf);
    L(b);
    for (auto& f : fns) {
        if (f.selected && !f.virtualized && !f.note.empty() && f.note.rfind("RED", 0) == 0)
            L("[RED] " + f.name + " @0x" + hex32(f.rva) + ": " + f.note);
    }

    // --- plain olustur: STRDEC + ANTIDBG + govde ---
    const u8* blob; int blob_len;
    if (pe.is64) { blob = VM_BLOB64; blob_len = VM_BLOB64_LEN; }
    else         { blob = VM_BLOBX86; blob_len = VM_BLOBX86_LEN; }

    u32 newva_guess = 0;
    {
        u32 lastva = 0, lastvsz = 0;
        for (auto& s : pe.secs) if (s.va + s.vsz > lastva + lastvsz) { lastva = s.va; lastvsz = s.vsz; }
        newva_guess = (lastva + lastvsz + pe.secalign - 1) & ~(pe.secalign - 1);
    }

    for (auto& c : comps) {
        if (c.ty == 1) continue;              // M: bayt-kod yok
        std::vector<u8> plain;
        if (opt.strenc) {
            std::vector<RStr> used;
            for (u32 ref : c.v.str_refs)
                for (auto& s : strings)
                    if (ref >= s.rva && ref < s.rva + s.len)
                        if (std::find_if(used.begin(), used.end(), [&](RStr& r) { return r.rva == s.rva; }) == used.end())
                            used.push_back(s);
            for (auto& s : used) {
                plain.push_back(OP_STRDEC);
                for (int k = 0; k < 4; k++) plain.push_back((u8)(s.rva >> (8 * k)));
                c.state_sites.push_back((u32)plain.size());
                for (int k = 0; k < 4; k++) plain.push_back(0);   // state rva placeholder
                c.state_strva.push_back(s.rva);
                u32 k2 = (s.rva * 2654435761u) | 1u;
                c.v.notes.push_back("  [STRDEC] str=0x" + hex32(s.rva) + " len=" + std::to_string(s.len) + " key=0x" + hex32(k2));
            }
        }
        if (opt.antidebug) plain.push_back(OP_ANTIDBG);
        plain.insert(plain.end(), c.v.plain.begin(), c.v.plain.end());
        c.plain = plain;
    }

    // --- section icinde RVAlari planla ---
    u32 cur = (u32)blob_len;
    for (auto& c : comps) {
        if (c.ty == 1) continue;              // M: bc yerlesimi yok
        cur = (cur + 7) & ~7u;
        c.bc_rva = newva_guess + cur;
        cur += (u32)c.plain.size();
    }
    u32 vent_guess = newva_guess + (pe.is64 ? VM_BLOB64_ENTRY_OFF : VM_BLOBX86_ENTRY_OFF);
    for (auto& c : comps) {
        if (c.ty == 1) continue;              // M: stub yok (native govde)
        cur = (cur + 15) & ~15u;
        c.stub_rva = newva_guess + cur;
        std::vector<u8> st;
        if (pe.is64) st = build_stub_x64(c.stub_rva, vent_guess, c.key, c.bc_rva, (u32)c.plain.size());
        else         st = build_stub_x86(c.stub_rva, vent_guess, c.key, c.bc_rva, (u32)c.plain.size(), c.v.retimm);
        c.tramp = st.size() > c.f->size;
        cur += (u32)st.size();
    }
    // --- mutasyon bloklari (tip M): olcum gecisi boyutu kadar yer ---
    for (auto& c : comps) if (c.ty == 1) {
        cur = (cur + 15) & ~15u;
        c.mut_rva = newva_guess + cur;
        cur += (u32)c.mut.code.size();
    }
    u32 state_off = cur;
    u32 nstate = 0;
    for (auto& c : comps) nstate += (u32)c.state_sites.size();

    // --- state RVA'larini plain'e yaz ---
    u32 state_idx = 0;
    for (auto& c : comps) {
        for (size_t i = 0; i < c.state_sites.size(); i++) {
            u32 stva = newva_guess + state_off + state_idx;
            u32 at = c.state_sites[i];
            for (int k = 0; k < 4; k++) c.plain[at + k] = (u8)(stva >> (8 * k));
            state_idx++;
        }
    }

    // --- sifrele ---
    for (auto& c : comps) {
        if (c.ty == 1) continue;              // M: sifreleme yok (native kod)
        u64 key = 0;
        if (opt.vmenc) {
            key = ((u64)(u32)(c.f->rva * 2654435761u + 0x9E3779B9u) << 32)
                | (u32)((u32)c.plain.size() * 0x2545F491u + 1u) | 1ull;
        }
        c.key = key;
        c.enc.resize(c.plain.size());
        for (size_t i = 0; i < c.plain.size(); i++)
            c.enc[i] = c.plain[i] ^ VM_KS(key, i);   /* pozisyon-bazli */
    }

    // --- section goruntusunu kur ---
    std::vector<u8> secimg;
    secimg.insert(secimg.end(), blob, blob + blob_len);
    while (secimg.size() < (size_t)state_off - (nstate ? 0 : 0)) secimg.push_back(0);
    secimg.resize(state_off + nstate, 0);
    for (auto& c : comps) {
        if (c.ty == 1) continue;              // M: bc/stub kopyasi yok
        u32 o = c.bc_rva - newva_guess;
        memcpy(&secimg[o], c.enc.data(), c.enc.size());
        u32 so = c.stub_rva - newva_guess;
        u32 vent = vent_guess;
        std::vector<u8> st;
        if (pe.is64) st = build_stub_x64(c.stub_rva, vent, c.key, c.bc_rva, (u32)c.plain.size());
        else         st = build_stub_x86(c.stub_rva, vent, c.key, c.bc_rva, (u32)c.plain.size(), c.v.retimm);
        memcpy(&secimg[so], st.data(), st.size());
    }
    // mutasyon bloklari: GERCEK base_rva ile yeniden uret (deterministik LCG
    // ayni seed'i verir; boyutlar bayt bayt eslesmeli)
    for (auto& c : comps) if (c.ty == 1) {
        bool okraw2; u32 raw2 = pe.rva2raw(c.f->rva, &okraw2);
        if (!okraw2) { L("[HATA] mutasyon yeniden uretim raw yok"); return 1; }
        // 0.4.3 FIX (kullanici x4: Mutation ciktisi V ile ayni cikiyordu):
        // ikinci geciste allow_call YANLISLIKLA pe.is64 yollaniyordu — x86'da
        // (is64=false) yeniden-uretim "call desteklenmiyor" diyerek dusuyor,
        // olcum gecisi ise true ile govde uretip tutarsizlik tetikliyor ->
        // [HATA] + rc=1 -> CIKTI DOSYASI HIC YAZILMIYOR. GUI eski V ciktisini
        // diskte birakip kullanici onu "mutation" saniyor (uc dosyadan ikisi
        // bayt-bayt ayni). Olcum gecisiyle AYNI parametre: allow_call=true
        // (0.4.1 x86 rel32 dis-cagri fixup'i destekliyor).
        MutFn m2 = mutate_function(&pe.buf[raw2], c.f->rva, c.f->size, pe.is64,
                                   pe.imagebase, pe.sizeofimage, true,
                                   opt.mut_strength, c.f->rva ^ 0x9E3779B9u,
                                   c.mut_rva /* GERCEK base */);
        if (!m2.ok || m2.code.size() != c.mut.code.size()) {
            { FILE* f1 = fopen("/tmp/mutpass1.bin","wb"); if(f1){fwrite(c.mut.code.data(),1,c.mut.code.size(),f1);fclose(f1);} FILE* f2 = fopen("/tmp/mutpass2.bin","wb"); if(f2){fwrite(m2.code.data(),1,m2.code.size(),f2);fclose(f2);} L("[HATA] mutasyon tutarsiz: " + c.f->name + " neden=" + m2.why + " " + std::to_string(m2.code.size()) + " vs " + std::to_string(c.mut.code.size())); return 1; }
        }
        u32 o = c.mut_rva - newva_guess;
        memcpy(&secimg[o], m2.code.data(), m2.code.size());
    }
    // state baytlari 0 (VM ilk calismada 1 yapar)

    u32 vmp_rva = 0;
    u32 chars = 0xE0000060u;   // CNT_CODE|MEM_EXEC|MEM_READ|MEM_WRITE
    if (!pe.add_section(".vmp0", secimg, chars, vmp_rva)) { L("[HATA] section eklenemedi"); return 1; }
    snprintf(b, sizeof b, "[SECTION] .vmp0 RVA=0x%06X boyut=0x%06X | VM blob %dB | vm_entry RVA=0x%06X",
             vmp_rva, (u32)secimg.size(), blob_len, vmp_rva + (pe.is64 ? VM_BLOB64_ENTRY_OFF : VM_BLOBX86_ENTRY_OFF));
    L(b);
    if (vmp_rva != newva_guess) L("[UYARI] section RVA beklentiyle farkli!");

    // --- .text yamalama ---
    for (auto& c : comps) {
        FnInfo& f = *c.f;
        bool okraw; u32 raw = pe.rva2raw(f.rva, &okraw);
        if (!okraw) { f.note += " | yama raw yok"; continue; }
        if (c.ty == 1) {
            // MUTASYON: giris kapisi E9 -> .vmp0'taki mutasyonlu govde.
            // Govde native x86/x64 kodu olarak calisir (VM YOK, tam hiz).
            i32 rel = (i32)(c.mut_rva - (f.rva + 5));
            u8 j[5] = { 0xE9, (u8)(u32)rel, (u8)((u32)rel >> 8), (u8)((u32)rel >> 16), (u8)((u32)rel >> 24) };
            memcpy(&pe.buf[raw], j, 5);
            for (u32 i = 5; i < f.size; i++) pe.buf[raw + i] = 0xCC;
            f.virtualized = true;
            snprintf(b, sizeof b, "[MUT] %s @RVA=0x%06X boyut=0x%X | ikame=%u junk=%u kapi=%u (blok=%u) | %uB->%uB (x%.2f) | govde=0x%06X",
                     f.name.c_str(), f.rva, f.size,
                     c.mut.st.subst, c.mut.st.junk, c.mut.st.gates, c.mut.st.nblocks,
                     c.mut.st.orig_bytes, c.mut.st.new_bytes,
                     c.mut.st.orig_bytes ? (double)c.mut.st.new_bytes / (double)c.mut.st.orig_bytes : 0.0,
                     c.mut_rva);
            L(b);
            for (auto& n : c.mut.notes) L(n);
            continue;
        }
        if (c.tramp) {
            i32 rel = (i32)(c.stub_rva - (f.rva + 5));
            u8 j[5] = { 0xE9, (u8)(u32)rel, (u8)((u32)rel >> 8), (u8)((u32)rel >> 16), (u8)((u32)rel >> 24) };
            memcpy(&pe.buf[raw], j, 5);
            for (u32 i = 5; i < f.size; i++) pe.buf[raw + i] = 0xCC;
        } else {
            // YERINDE stub: stub_rva = fonksiyonun kendi RVA'si olacak sekilde uret
            u32 vent = vmp_rva + (pe.is64 ? VM_BLOB64_ENTRY_OFF : VM_BLOBX86_ENTRY_OFF);
            std::vector<u8> st;
            if (pe.is64) st = build_stub_x64(f.rva, vent, c.key, c.bc_rva, (u32)c.plain.size());
            else         st = build_stub_x86(f.rva, vent, c.key, c.bc_rva, (u32)c.plain.size(), c.v.retimm);
            memcpy(&pe.buf[raw], st.data(), st.size());
            for (u32 i = (u32)st.size(); i < f.size; i++) pe.buf[raw + i] = 0xCC;
        }
        f.virtualized = true;
        snprintf(b, sizeof b, "[YAMA] %s @RVA=0x%06X boyut=0x%X | bc=%uB key=0x%llX | stubRVA=0x%06X retimm=%u %s",
                 f.name.c_str(), f.rva, f.size, (u32)c.enc.size(),
                 (unsigned long long)c.key, c.stub_rva, c.v.retimm,
                 c.tramp ? "(trampoline)" : "(yerinde)");
        L(b);
        if (c.ty == 2 && c.mut.ok) {
            snprintf(b, sizeof b, "  [ULTRA] on-mutasyon: ikame=%u junk=%u kapi=%u | %uB->%uB (x%.2f)",
                     c.mut.st.subst, c.mut.st.junk, c.mut.st.gates,
                     c.mut.st.orig_bytes, c.mut.st.new_bytes,
                     c.mut.st.orig_bytes ? (double)c.mut.st.new_bytes / (double)c.mut.st.orig_bytes : 0.0);
            L(b);
        }
        for (auto& n : c.v.notes) L(n);
    }

    if (nM && pe.is64)
        L("[MUT] Not: x64 .pdata unwind kayitlari eski govde araligina isaret eder "
          "(mutasyonlu fonksiyonlarda istisna isleme kapali — VMProtect davranisi)");

    // --- BUGFIX #13: ezilen bolgelerin eski reloc girdilerini temizle ---
    // Yerinde stub + trampoline .text yamasi fonksiyonun TUM govdesini
    // eziyor (stub + int3). Orijinal koda ait HIGHLOW girdileri bu
    // baytlarin uzerinde kalirsa gercek Windows ASLR (DYNAMICBASE) loader
    // tarafindan +delta ile yamalanir -> stub immediateleri (VM anahtari!)
    // ve E9 rel32 bozulur -> ilk VM cagrisinda cokme. Wine preferred-base
    // yukledigi icin (delta=0) bu hata wine'da gorunmezdi.
    {
        std::vector<std::pair<u32,u32>> ranges;
        for (auto& c : comps)
            if (c.f->virtualized) ranges.push_back({ c.f->rva, c.f->size });
        u32 removed = 0; std::string rnote;
        if (pe.filter_relocs(ranges, removed, rnote)) L("[RELOC] " + rnote);
        else L("[HATA] reloc filtreleme: " + rnote);
    }

    // --- secenek bayraklari ---
    if ((opt.memprot && text) || (opt.strenc && rdata)) {
        u32 fh = pe.e_lfanew + 4;
        u16 optsize = 0; memcpy(&optsize, &pe.buf[fh + 16], 2);
        u32 sh = fh + 20 + optsize;
        for (u32 i = 0; i < pe.secs.size(); i++) {
            u32 o = sh + i * 40;
            char nm[9]; memcpy(nm, &pe.buf[o], 8); nm[8] = 0;
            if (opt.memprot && strcmp(nm, ".text") == 0) {
                u32 ch = text->chars & ~0x80000000u;   // WRITE kaldir -> EXECUTE_READ
                memcpy(&pe.buf[o + 36], &ch, 4);
                snprintf(b, sizeof b, "[MEMPROT] .text Chars=0x%08X (yazilamaz)", ch);
                L(b);
            }
            if (opt.strenc && strcmp(nm, ".rdata") == 0) {
                u32 ch = rdata->chars | 0x80000000u;   // cozum icin yazma
                memcpy(&pe.buf[o + 36], &ch, 4);
                snprintf(b, sizeof b, "[STRENC] .rdata Chars=0x%08X (yazilabilir)", ch);
                L(b);
            }
        }
    }

    pe.fix_checksum();
    if (!pe.save(out)) { L("[HATA] kaydedilemedi: " + out); return 1; }
    snprintf(b, sizeof b, "[KAYIT] %s (%zu bayt)", out.c_str(), pe.buf.size());
    L(b);
    L("[OZET] tipler: V=" + std::to_string(nV) + " M=" + std::to_string(nM) + " U=" + std::to_string(nU)
      + (nUf ? " (U->V dusus=" + std::to_string(nUf) + ")" : "")
      + " | antidebug=" + (opt.antidebug ? "ACIK" : "kapali")
      + " | strenc=" + (opt.strenc ? "ACIK" : "kapali")
      + " | memprot=" + (opt.memprot ? "ACIK" : "kapali")
      + " | vmenc=" + (opt.vmenc ? "ACIK" : "KAPALI")
      + " | mutasyon-gucu=" + std::to_string(opt.mut_strength));
    return 0;
}
