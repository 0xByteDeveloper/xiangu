// ============================================================
// xprotect_ui.cpp — XProtect UI (Win32 API)
// universalObfusPublicSrc GUI'sinin (Qt) birebir Win32 muadili.
// Menuler/diyaloglar/ozellik seti VMProtect kaynak agacindaki
// VMProtect/mainwindow.cc, function_dialog.cc, settings_dialog.cc,
// watermark_dialog.cc, license_dialog.cc, about_dialog.cc,
// progress_dialog.cc yapisina gore kuruldu (etiketler en.lng).
//
// XProtect 0.2.0 — Saf Win32 (Qt YOK), x64 + x86.
// Modlar:
//   XProtectUI.exe              -> normal GUI
//   XProtectUI.exe --selftest   -> pencere kur + 1.2 sn pompala + EXIT 0
// ============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "engine.hpp"
#include "engine_sym.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// ---------------- surum ----------------
#define XPROTECT_EDITION  L"XProtect Ultimate"
#define XPROTECT_VERSION  L"0.3.0"
#define XPROTECT_BUILD    L"2026-11-07"

// ---------------- dil tablosu (en.lng birebir + TR) ----------------
enum LS {
    LS_FILE, LS_EDIT, LS_PROJECT, LS_TOOLS, LS_HELP,
    LS_OPEN, LS_SAVE, LS_SAVEAS, LS_SAVELIC, LS_CLOSE, LS_EXIT,
    LS_UNDO, LS_REDO, LS_CUT, LS_COPY, LS_PASTE, LS_DELETE, LS_SEARCH, LS_GOTO,
    LS_ADD, LS_ADDFUNC, LS_ADDFOLDER, LS_ADDLIC, LS_ADDFILE,
    LS_IMPORT, LS_IMPLIC, LS_IMPPROJ, LS_EXPORTKEY,
    LS_COMPILE, LS_EXECUTE, LS_WATERMARKS, LS_SETTINGS,
    LS_CONTENTS, LS_HOMEPAGE, LS_ABOUT,
    LS_NAME, LS_VALUE, LS_GENERATE, LS_CANCEL, LS_OK, LS_DETAILS,
    LS_VIRT, LS_MUT, LS_ULTRA, LS_COMPTYPE, LS_LOCKKEY, LS_OPTIONS,
    LS_FUNCS, LS_ADDRESS, LS_LANGUAGE, LS_AUTOSAVE,
    LS_WELCOME, LS_CUSTNAME, LS_EMAIL, LS_DATE, LS_ORDER, LS_COMMENTS,
    LS_SERIALCONT, LS_HWID, LS_EXPIRE, LS_TIMELIMIT, LS_MAXBUILD, LS_USERDATA,
    LS_PARAMS, LS_TEMPLATES, LS_SAVETPL, LS_EXCLUDED, LS_BLOCKED, LS_ADDWM,
    LS_EXTADDR, LS_BREAKADDR, LS_SHOWPROT,
    LS_EXEC_ORIG, LS_EXEC_PROT, LS_EXEC_PARAMS,
    LS_TYPE, LS_SIZE, LS_EXPORT, LS_PROTECTION, LS_UNREG, LS_PURCHASE,
    LS_OPEN_FILE, LS_OPEN_PROJ, LS_NOTPORTED, LS_COMPILING, LS_DONE,
    LS_READY, LS_ERROR, LS_QUESTION, LS_DELETE_TITLE,
    LS_LIC_PROPS, LS_CORE_PROPS, LS_FUNC_PROPS, LS_ALLFILES, LS_PROJECT_FILES,
    LS_SELECT_TARGET, LS_TARGET, LS_FUNC_ADDED, LS_ALREADY_ADDED, LS_NO_TARGET,
    LS_CDFILTER, LS_WM_PROPS,
    // 0.3.0 ekleri — sona eklenir (tablo sirasi korunur)
    LS_SOURCE, LS_DISASM, LS_TPL_LOAD, LS_EXEC_SEL, LS_SYMINFO,
    LS_COUNT
};

static const wchar_t* g_lang_en[LS_COUNT] = {
    L"File", L"Edit", L"Project", L"Tools", L"Help",
    L"Open", L"Save Project", L"Save Project As", L"Save Licenses As", L"Close", L"Exit",
    L"Undo", L"Redo", L"Cut", L"Copy", L"Paste", L"Delete", L"Search", L"Go To Address",
    L"Add", L"Add Function", L"Add Folder", L"Add License", L"Add File",
    L"Import", L"Import License", L"Import Licenses from a Project File", L"Export Key Pair",
    L"Compile", L"Execute", L"Watermarks", L"Settings",
    L"Contents", L"Home Page", L"About",
    L"Name", L"Value", L"Generate", L"Cancel", L"OK", L"Details",
    L"Virtualization", L"Mutation", L"Ultra", L"Compilation Type", L"Lock To Serial Number", L"Options",
    L"Functions", L"Address", L"Language", L"AutoSave Project After Compilation",
    L"Welcome to", L"Customer Name", L"Email", L"Date", L"Order #", L"Comments",
    L"Serial Number Contents", L"Hardware ID", L"Expiration Date", L"Running Time Limit",
    L"Max Build Date", L"User Data",
    L"Parameters", L"Templates", L"Save Template As", L"Excluded From Compilation", L"Blocked",
    L"Add Watermark",
    L"External Address", L"End of Function", L"Show protected functions only",
    L"Execute Original", L"Execute Protected", L"Parameters...",
    L"Type", L"Size", L"Export", L"Protection", L"Unregistered Version", L"Purchase a License",
    L"Open File", L"Open Project", L"This feature is not ported yet (engine roadmap).",
    L"Compiling...", L"Done",
    L"Ready", L"Error", L"Question", L"Delete",
    L"License Properties", L"Core Properties", L"Function Properties", L"All Files", L"Project Files",
    L"Select Target File", L"Target", L"Function added.", L"This address is already used by function.",
    L"No target file. Open a file first.", L"Executable files\0*.exe;*.dll\0All Files\0*.*\0",
    L"Watermark Properties",
    L"Source", L"Disassembly", L"Load Template", L"Execute selected", L"Symbol info"
};
static const wchar_t* g_lang_tr[LS_COUNT] = {
    L"Dosya", L"Düzen", L"Proje", L"Araçlar", L"Yardım",
    L"Aç", L"Projeyi Kaydet", L"Projeyi Farklı Kaydet", L"Lisansları Farklı Kaydet", L"Kapat", L"Çıkış",
    L"Geri Al", L"Yinele", L"Kes", L"Kopyala", L"Aktar", L"Sil", L"Ara", L"Adrese Git",
    L"Ekle", L"Fonksiyon Ekle", L"Klasör Ekle", L"Lisans Ekle", L"Dosya Ekle",
    L"İçe Aktar", L"Lisans İçe Aktar", L"Lisansları Proje Dosyasından İçe Aktar", L"Anahtar Çifti Dışa Aktar",
    L"Derle", L"Çalıştır", L"Filigranlar", L"Ayarlar",
    L"İçindekiler", L"Ana Sayfa", L"Hakkında",
    L"Ad", L"Değer", L"Üret", L"İptal", L"Tamam", L"Ayrıntılar",
    L"Sanallaştırma", L"Mutasyon", L"Ultra", L"Derleme Türü", L"Seri Numarasına Kilitle", L"Seçenekler",
    L"Fonksiyonlar", L"Adres", L"Dil", L"Derlemeden Sonra Projeyi Otomatik Kaydet",
    L"Hoş geldiniz", L"Müşteri Adı", L"E-posta", L"Tarih", L"Sipariş #", L"Açıklamalar",
    L"Seri Numarası İçeriği", L"Donanım Kimliği", L"Son Kullanma Tarihi", L"Çalışma Süresi Sınırı",
    L"Maks. Derleme Tarihi", L"Kullanıcı Verisi",
    L"Parametreler", L"Şablonlar", L"Şablonu Farklı Kaydet", L"Derlemeden Hariç", L"Engelli",
    L"Filigran Ekle",
    L"Dış Adres", L"Fonksiyon Sonu", L"Sadece korumalı fonksiyonları göster",
    L"Orijinali Çalıştır", L"Korumalıyı Çalıştır", L"Parametreler...",
    L"Tür", L"Boyut", L"Dışa Aktarım", L"Koruma", L"Kayıtsız Sürüm", L"Lisans Satın Al",
    L"Dosya Aç", L"Proje Aç", L"Bu özellik henüz port edilmedi (motor yol haritası).",
    L"Derleniyor...", L"Bitti",
    L"Hazır", L"Hata", L"Soru", L"Sil",
    L"Lisans Özellikleri", L"Çekirdek Özellikleri", L"Fonksiyon Özellikleri", L"Tüm Dosyalar", L"Proje Dosyaları",
    L"Hedef Dosya Seç", L"Hedef", L"Fonksiyon eklendi.", L"Bu adres zaten bir fonksiyon tarafından kullanılıyor.",
    L"Hedef dosya yok. Önce dosya açın.", L"Çalıştırılabilir dosyalar\0*.exe;*.dll\0Tüm Dosyalar\0*.*\0",
    L"Filigran Özellikleri",
    L"Kaynak", L"Söküm (Disassembly)", L"Şablonu Yükle", L"Seçiliyi çalıştır", L"Sembol bilgisi"
};
static bool g_tr = false;
static const wchar_t* L(int id) { return (g_tr ? g_lang_tr : g_lang_en)[id]; }

// ---------------- menu/komut ID'leri ----------------
enum {
    IDM_OPEN = 0x1000, IDM_SAVE, IDM_SAVEAS, IDM_SAVELIC, IDM_CLOSE, IDM_EXIT,
    IDM_UNDO, IDM_REDO, IDM_CUT, IDM_COPY, IDM_PASTE, IDM_DELETE,
    IDM_SEARCH, IDM_GOTO,
    IDM_ADDFUNC, IDM_ADDFOLDER, IDM_ADDLIC, IDM_ADDFILE,
    IDM_IMPLIC, IDM_IMPPROJ, IDM_EXPORTKEY,
    IDM_COMPILE, IDM_EXECUTE,
    IDM_WATERMARKS, IDM_SETTINGS,
    IDM_HELP, IDM_HOMEPAGE, IDM_ABOUT,
    IDM_EXEC_ORIG, IDM_EXEC_PROT, IDM_EXEC_PARAMS,
    IDM_TPL_SAVE, IDM_TPL_EDIT, IDM_DISASM,
    IDM_RECENT_BASE = 0x2000,
    IDM_TPL_BASE = 0x2100,
    IDC_MAIN_TOOLBAR = 101, IDC_MAIN_TREE, IDC_MAIN_FUNC, IDC_MAIN_PROP,
    IDC_MAIN_LOG, IDC_MAIN_STATUS, IDC_MAIN_TABSTRIP, IDC_MAIN_WELCOME,
    IDC_MAIN_FILTER, IDC_SPLIT_V,
    IDC_TBB_PROJECT = 300, IDC_TBB_FUNCS, IDC_TBB_DETAILS
};

// ---------------- proje modeli ----------------
struct ProjFunc {
    std::wstring name;
    unsigned long long rva = 0;
    unsigned long long size = 0;
    bool is_export = false;
    int src = 0;               // keşif kaynağı (SYM_EXPORT/COFF/PDB/MAP/STUB)
    int type = 0;            // 0=Virtualization 1=Mutation 2=Ultra
    bool excluded = false;
    bool lock_to_key = false;
    unsigned long long ext = 0;   // External Address (0=auto)
    unsigned long long brk = 0;   // End of Function  (0=auto)
};
struct ProjWatermark { std::wstring name, value; };
struct ProjLicense {
    std::wstring name, email, date, order, comments;
    bool s_name = false, s_email = false, s_hwid = false, s_expire = false,
         s_time = false, s_maxbuild = false, s_userdata = false;
    std::wstring s_expire_d, s_maxbuild_d, s_time_v, s_userdata_v;
};
struct Project {
    bool open = false, modified = false;
    std::wstring target;                 // hedef PE (tam yol)
    std::wstring path;                   // .xproj dosya yolu
    unsigned long long options = 0;      // cp* bitleri (core.h birebir)
    std::vector<ProjFunc> funcs;
    std::vector<ProjWatermark> wms;
    std::vector<ProjLicense> lics;
    std::wstring exec_params;
    int find_func(const std::wstring& n) const {
        for (size_t i = 0; i < funcs.size(); i++) if (funcs[i].name == n) return (int)i;
        return -1;
    }
};
static Project g_proj;

// cp* bitleri — universalObfusPublicSrc core.h:6-29 birebir
enum ProjectOption {
    cpDebugMode             = 0x00000002,
    cpCryptValues           = 0x00000008,
    cpIncludeWatermark      = 0x00000020,
    cpRunnerCRC             = 0x00000040,
    cpEncryptRegs           = 0x00000080,
    cpStripFixups           = 0x00008000,
    cpPack                  = 0x00000100,
    cpImportProtection      = 0x00000200,
    cpCheckDebugger         = 0x00000400,
    cpCheckVirtualMachine   = 0x00000800,
    cpMemoryProtection      = 0x00001000,
    cpResourceProtection    = 0x00010000,
    cpCheckKernelDebugger   = 0x00020000,
    cpStripDebugInfo        = 0x00040000,
    cpLoaderCRC             = 0x10000000,
    cpEncryptBytecode       = 0x80000000,
    cpVirtualFiles          = 0x08000000,
    cpInternalMemoryProtection = 0x04000000,
    cpLoader                = 0x02000000
};

// ---------------- PE cozumleyici: export + COFF + PDB + MAP + stub ----------------
// (VMProtect davranisi: pdb/map varsa fonksiyon RVA listesi onlardan;
//  yoksa stub fonksiyonlar listelenir)
struct PeSection { std::wstring name; unsigned long long va, vsz, rawsz, rawptr; unsigned long chars; };
struct PeFunc { std::wstring name; unsigned long long rva, size; bool is_export; int source; };
struct PeInfo {
    bool ok = false; bool is64 = false;
    unsigned long long imagebase = 0, ep = 0;
    unsigned long machine = 0, subsystem = 0, dllchars = 0, timedate = 0;
    std::vector<PeSection> secs;
    std::vector<PeFunc> funcs;
    std::vector<unsigned char> raw;      // tam dosya (disasm + sembol taramalari)
    std::wstring sym_note;               // sembol kaynagi ozeti (log/status icin)
};

static std::wstring pe_name8(const unsigned char* p) {
    char buf[9]; memcpy(buf, p, 8); buf[8] = 0;
    std::string s(buf);
    return std::wstring(s.begin(), s.end());
}

// PE header'dan dosya offsetine rva cevrimi icin section tablosu taramasi
static unsigned long long rva_to_fileoff(const std::vector<PeSection>& secs,
                                         unsigned long long rva) {
    for (auto& s : secs) {
        if (rva >= s.va && rva < s.va + s.rawsz && s.rawsz > 0)
            return s.rawptr + (rva - s.va);
    }
    return 0;
}

// ---------------- sembol kaynagi yukleyiciler (PDB / MAP / stub) ----------------
static std::wstring sym_dir_of(const std::wstring& p) {
    size_t q = p.find_last_of(L"\\/");
    return q == std::wstring::npos ? L"." : p.substr(0, q);
}
static std::wstring sym_base_noext(const std::wstring& p) {
    size_t q = p.find_last_of(L"\\/");
    std::wstring b = q == std::wstring::npos ? p : p.substr(q + 1);
    size_t d = b.find_last_of(L'.');
    if (d != std::wstring::npos && d > 0) b.resize(d);
    return b;
}
static std::wstring sym_fname_of(const std::wstring& p) {
    size_t q = p.find_last_of(L"\\/");
    return q == std::wstring::npos ? p : p.substr(q + 1);
}
static bool sym_slurp(const std::wstring& path, std::vector<unsigned char>& out) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD hi = 0, lo = GetFileSize(f, &hi);
    size_t n = (size_t)(((unsigned long long)hi << 32) | lo);
    out.resize(n ? n : 1);
    DWORD rd = 0;
    ReadFile(f, out.data(), (DWORD)n, &rd, NULL);
    CloseHandle(f);
    return rd == n && n > 0;
}
static std::wstring s2w_narrow(const std::string& s) {   // dar (latin1-vari) -> genis
    std::wstring w(s.size(), L' ');
    for (size_t i = 0; i < s.size(); i++) w[i] = (unsigned char)s[i];
    return w;
}
static std::wstring sym_numdec(unsigned long long v) {
    wchar_t b[32]; _snwprintf(b, 32, L"%llu", v); return b;
}

// PDB / MAP / stub keşfi — VMProtect davranisinin portu:
// pdb/map varsa fonksiyon RVA listesi onlardan; yoksa stub fonksiyonlar.
static void load_symbol_sources(PeInfo& pi, const std::wstring& target_path) {
    if (pi.raw.empty() || pi.secs.empty()) return;
    SymSections ss;
    for (auto& s : pi.secs) {
        SymSections::Sec t;
        t.va = (su32)s.va; t.vsz = (su32)s.vsz; t.rawsz = (su32)s.rawsz;
        t.rawptr = (su32)s.rawptr; t.chars = (su32)s.chars;
        std::string nm(s.name.begin(), s.name.end());
        t.name = nm;
        ss.secs.push_back(t);
    }
    std::wstring dir = sym_dir_of(target_path);
    std::wstring base = sym_base_noext(target_path);

    // --- aday PDB yollari + beklenen GUID/age (bayat PDB eleme — VMP davranisi) ---
    std::vector<std::wstring> cands;
    if (!base.empty()) cands.push_back(dir + L"\\" + base + L".pdb");
    std::vector<DbgPdbRef> dbg_refs;
    pe_debug_pdb_names(pi.raw.data(), pi.raw.size(), ss, dbg_refs);
    const su8* want_guid = NULL; su32 want_age = 0;
    if (!dbg_refs.empty() && dbg_refs[0].has_guid) {
        want_guid = dbg_refs[0].guid; want_age = dbg_refs[0].age;
    }
    for (auto& r : dbg_refs) {
        std::wstring w = s2w_narrow(r.name);
        bool abs_ = (w.size() > 1 && (w[1] == L':' || w[0] == L'\\' || w[0] == L'/'));
        if (abs_) cands.push_back(w);
        std::wstring fn = sym_fname_of(w);
        if (!fn.empty()) cands.push_back(dir + L"\\" + fn);
    }
    // dedupe
    for (size_t i = 0; i < cands.size(); i++)
        for (size_t j = i + 1; j < cands.size(); )
            if (cands[j] == cands[i]) cands.erase(cands.begin() + j); else j++;

    // --- PDB ---
    for (auto& c : cands) {
        std::vector<unsigned char> pd;
        if (!sym_slurp(c, pd)) continue;
        if (want_guid && !pdb_matches(pd.data(), pd.size(), want_guid, want_age))
            continue;   // bayat/eslesmeyen PDB — ele
        std::vector<SymEntry> fns; std::string err;
        if (!pdb_parse(pd.data(), pd.size(), ss, fns, err)) continue;
        size_t added = 0, updated = 0;
        for (auto& e : fns) {
            PeFunc* hit = NULL;
            for (auto& f : pi.funcs) if (f.rva == e.rva) { hit = &f; break; }
            if (hit) {
                if (e.size && !hit->size) { hit->size = e.size; updated++; }
            } else {
                PeFunc pf;
                pf.name = s2w_narrow(e.name);
                pf.rva = e.rva; pf.size = e.size; pf.is_export = false;
                pf.source = SYM_PDB;
                pi.funcs.push_back(pf);
                added++;
            }
        }
        if (added || updated) {
            pi.sym_note = std::wstring(L"PDB: ") + sym_fname_of(c) + L" (+" + sym_numdec(added) + L")";
        }
        break;   // ilk tutan PDB yeter
    }

    // --- MAP ---
    if (!base.empty()) {
        std::vector<unsigned char> md;
        std::wstring mpath = dir + L"\\" + base + L".map";
        if (sym_slurp(mpath, md)) {
            std::vector<SymEntry> fns; std::string err;
            if (map_parse((const char*)md.data(), md.size(), ss, (su32)pi.imagebase, fns, err)) {
                size_t added = 0;
                for (auto& e : fns) {
                    bool dup = false;
                    for (auto& f : pi.funcs) if (f.rva == e.rva) { dup = true; break; }
                    if (dup) continue;
                    PeFunc pf;
                    pf.name = s2w_narrow(e.name);
                    pf.rva = e.rva; pf.size = e.size; pf.is_export = false;
                    pf.source = SYM_MAP;
                    pi.funcs.push_back(pf);
                    added++;
                }
                if (added) pi.sym_note = std::wstring(L"MAP: ") + sym_fname_of(mpath) + L" (+" + sym_numdec(added) + L")";
            }
        }
    }

    // --- hic sembol yoksa: stub fonksiyonlar (giris + prologue taramasi) ---
    if (pi.funcs.empty()) {
        std::vector<SymEntry> fns;
        stub_scan(pi.raw.data(), pi.raw.size(), ss, pi.is64, (su32)pi.ep, fns);
        for (auto& e : fns) {
            PeFunc pf;
            pf.name = s2w_narrow(e.name);
            pf.rva = e.rva; pf.size = e.size; pf.is_export = false;
            pf.source = SYM_STUB;
            pi.funcs.push_back(pf);
        }
        if (!fns.empty()) pi.sym_note = L"STUB: " + sym_numdec(fns.size()) + L" (pdb/map yok)";
    }
}


static bool parse_pe_file(const std::wstring& path, PeInfo& pi, std::wstring& err) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) { err = L"CreateFile failed"; return false; }
    DWORD hi = 0, lo = GetFileSize(f, &hi);
    size_t n = (size_t)(((unsigned long long)hi << 32) | lo);
    std::vector<unsigned char> d(n ? n : 1);
    DWORD rd = 0;
    ReadFile(f, d.data(), (DWORD)n, &rd, NULL);
    CloseHandle(f);
    if (n < 0x200 || rd != n) { err = L"Read failed"; return false; }
    if (d[0] != 'M' || d[1] != 'Z') { err = L"MZ not found"; return false; }
    unsigned e_lfanew = *(unsigned*)(d.data() + 0x3C);
    if ((size_t)e_lfanew + 0x100 > n || d[e_lfanew] != 'P' || d[e_lfanew + 1] != 'E') { err = L"PE signature"; return false; }
    const unsigned char* coff = d.data() + e_lfanew + 4;
    unsigned short machine = *(unsigned short*)(coff + 0);
    unsigned short nsec = *(unsigned short*)(coff + 2);
    unsigned timedate = *(unsigned*)(coff + 4);
    unsigned symptr = *(unsigned*)(coff + 8);
    unsigned nsyms = *(unsigned*)(coff + 12);
    unsigned short optsz = *(unsigned short*)(coff + 16);
    const unsigned char* opt = coff + 20;
    unsigned short magic = *(unsigned short*)opt;
    if ((size_t)e_lfanew + 4 + 20 + optsz + (size_t)nsec * 40 > n) { err = L"Truncated"; return false; }
    pi.is64 = (magic == 0x20B);
    pi.machine = machine;
    pi.timedate = timedate;
    unsigned long long dd_export_rva = 0;
    if (pi.is64) {
        pi.imagebase = *(unsigned long long*)(opt + 24);
        pi.ep = *(unsigned*)(opt + 16);
        pi.subsystem = *(unsigned short*)(opt + 68);
        pi.dllchars = *(unsigned short*)(opt + 70);
        dd_export_rva = *(unsigned*)(opt + 112 + 0);       // DataDirectory[0].VirtualAddress
    } else {
        pi.imagebase = *(unsigned*)(opt + 28);
        pi.ep = *(unsigned*)(opt + 16);
        pi.subsystem = *(unsigned short*)(opt + 68);
        pi.dllchars = *(unsigned short*)(opt + 70);
        dd_export_rva = *(unsigned*)(opt + 96 + 0);
    }
    // section table
    const unsigned char* st = opt + optsz;
    for (int i = 0; i < nsec; i++) {
        const unsigned char* sh = st + i * 40;
        PeSection s;
        s.name = pe_name8(sh);
        s.vsz = *(unsigned*)(sh + 8);
        s.va = *(unsigned*)(sh + 12);
        s.rawsz = *(unsigned*)(sh + 16);
        s.rawptr = *(unsigned*)(sh + 20);
        s.chars = *(unsigned*)(sh + 36);
        pi.secs.push_back(s);
    }
    PeSection* text = NULL;
    for (auto& s : pi.secs) if (s.name == L".text") { text = &s; break; }
    if (!text) for (auto& s : pi.secs) if (s.chars & 0x20000000) { text = &s; break; }
    // COFF sembol tablosu (engine coff_symbols ile ayni yol)
    std::map<unsigned long long, std::wstring> syms;
    if (symptr && nsyms && (size_t)symptr + (size_t)nsyms * 18 < n) {
        size_t strtab = (size_t)symptr + (size_t)nsyms * 18;
        for (unsigned i = 0; i < nsyms; i++) {
            const unsigned char* sym = d.data() + symptr + (size_t)i * 18;
            short secnum = *(short*)(sym + 6);
            unsigned short aux = *(unsigned short*)(sym + 12);
            unsigned char cls = sym[16];
            std::string nm;
            if (sym[0] == 0 && sym[1] == 0 && sym[2] == 0 && sym[3] == 0) {
                unsigned off = *(unsigned*)(sym + 4);
                if (strtab + off < n) {
                    const char* sp = (const char*)d.data() + strtab + off;
                    nm = sp;
                }
            } else {
                char b[9]; memcpy(b, sym, 8); b[8] = 0; nm = b;
            }
            if (cls == 2 && secnum > 0 && (int)secnum <= (int)pi.secs.size() && !nm.empty() && nm[0] != '.') {
                unsigned val = *(unsigned*)(sym + 8);
                unsigned long long rva = pi.secs[secnum - 1].va + val;
                if (!syms.count(rva)) syms[rva] = std::wstring(nm.begin(), nm.end());
            }
            i += aux;
        }
    }
    // export tablosu
    if (dd_export_rva) {
        unsigned long long efile = rva_to_fileoff(pi.secs, dd_export_rva);
        if (efile && efile + 40 < n) {
            const unsigned char* ex = d.data() + (size_t)efile;
            unsigned nnames = *(unsigned*)(ex + 24);
            unsigned funcs_rva = *(unsigned*)(ex + 28);
            unsigned names_rva = *(unsigned*)(ex + 32);
            unsigned long long nfile = names_rva ? rva_to_fileoff(pi.secs, names_rva) : 0;
            unsigned long long ffile = funcs_rva ? rva_to_fileoff(pi.secs, funcs_rva) : 0;
            if (nfile && ffile && nnames && nfile + (size_t)nnames * 4 < n && ffile + (size_t)nnames * 4 < n) {
                for (unsigned i = 0; i < nnames; i++) {
                    unsigned nrva = *(unsigned*)(d.data() + (size_t)nfile + i * 4);
                    unsigned long long sfile = nrva ? rva_to_fileoff(pi.secs, nrva) : 0;
                    unsigned addr_rva = *(unsigned*)(d.data() + (size_t)ffile + i * 4);
                    if (sfile && sfile < n) {
                        const char* sp = (const char*)d.data() + sfile;
                        PeFunc pf; pf.name = std::wstring(sp, sp + strlen(sp));
                        pf.rva = addr_rva; pf.size = 0; pf.is_export = true; pf.source = SYM_EXPORT;
                        pi.funcs.push_back(pf);
                    }
                }
            }
        }
    }
    // COFF sembollerini ekle
    for (auto& kv : syms) {
        bool dup = false;
        for (auto& f : pi.funcs) if (f.rva == kv.first) { dup = true; break; }
        if (dup) continue;
        if (text && (kv.first < text->va || kv.first >= text->va + text->vsz)) continue;
        PeFunc pf; pf.name = kv.second; pf.rva = kv.first; pf.size = 0; pf.is_export = false;
        pf.source = SYM_COFF;
        pi.funcs.push_back(pf);
    }
    pi.raw = d;   // disasm + sembol taramalari icin tampon
    load_symbol_sources(pi, path);
    // boyutlar: siradaki sembole kadar (PDB boyutu bilenler korunur)
    std::sort(pi.funcs.begin(), pi.funcs.end(), [](const PeFunc& a, const PeFunc& b) { return a.rva < b.rva; });
    pi.funcs.erase(std::unique(pi.funcs.begin(), pi.funcs.end(), [](const PeFunc& a, const PeFunc& b) { return a.rva == b.rva; }), pi.funcs.end());
    if (text) {
        for (size_t i = 0; i < pi.funcs.size(); i++) {
            if (pi.funcs[i].size) continue;   // PDB'den biliniyor
            unsigned long long end = (i + 1 < pi.funcs.size()) ? pi.funcs[i + 1].rva : (text->va + text->vsz);
            pi.funcs[i].size = end - pi.funcs[i].rva;
        }
    }
    pi.ok = !pi.funcs.empty() || nsec > 0;
    if (!pi.ok) err = L"No symbols";
    return pi.ok;
}

// ---------------- ikonlar (harf tabanli: F/K/V/M/U/L/W) ----------------
static HIMAGELIST g_il = NULL;
static HFONT g_font = NULL, g_font_bold = NULL;

static HBITMAP letter_bmp(HDC dc, wchar_t ch, COLORREF fg, COLORREF bg) {
    HBITMAP bmp = CreateCompatibleBitmap(dc, 16, 16);
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
    HBRUSH br = CreateSolidBrush(bg);
    RECT rc = { 0, 0, 16, 16 };
    FillRect(mem, &rc, br);
    DeleteObject(br);
    HFONT of = (HFONT)SelectObject(mem, g_font_bold);
    SetTextColor(mem, fg);
    SetBkMode(mem, TRANSPARENT);
    RECT r2 = { 0, 0, 16, 16 };
    DrawTextW(mem, &ch, 1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(mem, of);
    DeleteDC(mem);
    return bmp;
}
enum { ICO_FILE = 0, ICO_FOLDER, ICO_V, ICO_M, ICO_U, ICO_LIC, ICO_WM, ICO_COUNT };

// ---------------- ileri bildirimler (parcalar arasi) ----------------
static void update_all();
static void start_compile();
static void open_target(const std::wstring&);
static void open_project_file(const std::wstring&);
static void save_project(bool);
static void do_undo();
static void do_redo();
static void layout();                 // 0.3.0: rebuild_toolbar icin
static void update_toolbar_states(); // 0.3.0: toolbar durum guncellemesi
static void update_welcome_texts();  // 0.3.0: dil degisince welcome etiketleri
static void welcome_fill();          // 0.3.0: update_all'dan dogrudan cagriliyor

static void make_imagelist(HDC dc) {
    g_il = ImageList_Create(16, 16, ILC_COLOR32, ICO_COUNT, 0);
    ImageList_Add(g_il, letter_bmp(dc, L'F', RGB(30, 30, 30), RGB(200, 200, 200)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'K', RGB(70, 45, 0), RGB(255, 210, 90)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'V', RGB(255, 255, 255), RGB(40, 160, 60)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'M', RGB(255, 255, 255), RGB(200, 60, 40)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'U', RGB(255, 255, 255), RGB(50, 90, 210)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'L', RGB(255, 255, 255), RGB(150, 60, 190)), NULL);
    ImageList_Add(g_il, letter_bmp(dc, L'W', RGB(255, 255, 255), RGB(90, 90, 220)), NULL);
}
// ============================================================
// xprotect_ui.cpp — Part B: pencere, kontroller, layout, menuler
// ============================================================

static HWND g_main = NULL, g_toolbar = NULL, g_tree = NULL, g_func = NULL,
            g_prop = NULL, g_log = NULL, g_status = NULL, g_tabstrip = NULL,
            g_welcome = NULL, g_filter = NULL, g_split = NULL;
static HIMAGELIST g_il2 = NULL;
static HMENU g_menu = NULL;
static HACCEL g_accel = NULL;
static PeInfo g_pe;
static int g_tab = 0;              // 0=project 1=functions 2=details
static int g_split_x = 400;        // sol panel genisligi
static int g_log_h = 150;
static bool g_selftest = false;
static bool g_compiling = false;
static HANDLE g_cancel_event = NULL;
static std::vector<std::wstring> g_recent;
static bool g_autosave = false;
static std::vector<std::vector<ProjFunc> > g_undo, g_redo;

static wchar_t g_ini[MAX_PATH] = L"";

// ---------------- yardimcilar ----------------
static std::wstring exe_dir() {
    wchar_t buf[MAX_PATH]; GetModuleFileNameW(NULL, buf, MAX_PATH);
    std::wstring s(buf);
    size_t p = s.find_last_of(L'\\');
    if (p != std::wstring::npos) s = s.substr(0, p);
    return s;
}
static std::wstring basename_of(const std::wstring& p) {
    size_t q = p.find_last_of(L'\\');
    return q == std::wstring::npos ? p : p.substr(q + 1);
}
static std::wstring protected_path() {
    std::wstring t = g_proj.target;
    size_t d = t.find_last_of(L'.');
    if (d == std::wstring::npos) return t + L"_vmp.exe";
    return t.substr(0, d) + L"_vmp.exe";
}
static std::wstring numhex(unsigned long long v) {
    wchar_t b[32]; _snwprintf(b, 32, L"0x%08llX", v); return b;
}
// keşif kaynağı adı (Export/COFF/PDB/MAP/Stub)
static const wchar_t* src_name(int source) {
    switch (source) {
    case SYM_EXPORT: return L"Export";
    case SYM_COFF: return L"COFF";
    case SYM_PDB: return L"PDB";
    case SYM_MAP: return L"MAP";
    case SYM_STUB: return L"Stub";
    }
    return L"-";
}
static std::wstring numdec(unsigned long long v) {
    wchar_t b[32]; _snwprintf(b, 32, L"%llu", v); return b;
}
static void ini_write(const wchar_t* sec, const wchar_t* key, const wchar_t* val) {
    WritePrivateProfileStringW(sec, key, val, g_ini);
}
static std::wstring ini_read(const wchar_t* sec, const wchar_t* key, const wchar_t* def) {
    wchar_t buf[1024]; GetPrivateProfileStringW(sec, key, def, buf, 1024, g_ini);
    return buf;
}
static void load_settings() {
    g_tr = ini_read(L"Settings", L"Language", L"en") == L"tr";
    g_autosave = ini_read(L"Settings", L"AutoSave", L"0") == L"1";
    g_recent.clear();
    for (int i = 0; i < 4; i++) {
        wchar_t k[8]; _snwprintf(k, 8, L"File%d", i + 1);
        std::wstring v = ini_read(L"Recent", k, L"");
        if (!v.empty()) g_recent.push_back(v);
    }
    g_split_x = _wtoi(ini_read(L"Layout", L"SplitX", L"400").c_str());
    if (g_split_x < 220) g_split_x = 220;
    g_log_h = _wtoi(ini_read(L"Layout", L"LogH", L"150").c_str());
    if (g_log_h < 60) g_log_h = 60;
}
static void save_settings() {
    ini_write(L"Settings", L"Language", g_tr ? L"tr" : L"en");
    ini_write(L"Settings", L"AutoSave", g_autosave ? L"1" : L"0");
    for (int i = 0; i < 4; i++) {
        wchar_t k[8]; _snwprintf(k, 8, L"File%d", i + 1);
        ini_write(L"Recent", k, (size_t)i < g_recent.size() ? g_recent[i].c_str() : L"");
    }
    wchar_t b[16];
    _snwprintf(b, 16, L"%d", g_split_x); ini_write(L"Layout", L"SplitX", b);
    _snwprintf(b, 16, L"%d", g_log_h); ini_write(L"Layout", L"LogH", b);
}
static void push_recent(const std::wstring& p) {
    for (size_t i = 0; i < g_recent.size(); i++)
        if (g_recent[i] == p) { g_recent.erase(g_recent.begin() + i); break; }
    g_recent.insert(g_recent.begin(), p);
    if (g_recent.size() > 4) g_recent.resize(4);
}

static void add_log(const std::wstring& msg) {
    if (!g_log) return;
    time_t t = time(NULL); struct tm lt; localtime_s(&lt, &t);
    wchar_t tb[24]; _snwprintf(tb, 24, L"%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
    LVITEMW it; memset(&it, 0, sizeof(it));
    it.mask = LVIF_TEXT; it.iItem = ListView_GetItemCount(g_log); it.pszText = tb;
    int idx = ListView_InsertItem(g_log, &it);
    ListView_SetItemText(g_log, idx, 1, (LPWSTR)msg.c_str());
    ListView_EnsureVisible(g_log, idx, FALSE);
}

// ---------------- pencere oyuncaklari: dugme yaratma ----------------
static HWND mk_ctl(HWND parent, const wchar_t* cls, const wchar_t* txt,
                   unsigned long style, int id, int x, int y, int w, int h) {
    HWND c = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | style,
                             x, y, w, h, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

// ---------------- tree doldurma ----------------
static void tree_fill() {
    if (!g_tree) return;
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    TreeView_DeleteAllItems(g_tree);
    TreeView_SetImageList(g_tree, g_il, TVSIL_NORMAL);
    if (!g_proj.open) { SetCursor(LoadCursor(NULL, IDC_ARROW)); return; }
    TVINSERTSTRUCTW tv; memset(&tv, 0, sizeof(tv));
    std::wstring rootname = basename_of(g_proj.target);
    tv.hInsertAfter = TVI_LAST; tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    // kok: hedef dosya (param kind=0)
    tv.hParent = NULL;
    tv.item.iImage = tv.item.iSelectedImage = ICO_FILE;
    tv.item.pszText = (LPWSTR)rootname.c_str();
    tv.item.lParam = 0;
    HTREEITEM hFile = TreeView_InsertItem(g_tree, &tv);
    // Functions klasoru (kind=1)
    tv.hParent = hFile;
    tv.item.iImage = tv.item.iSelectedImage = ICO_FOLDER;
    tv.item.pszText = (LPWSTR)L(LS_FUNCS);
    tv.item.lParam = 1;
    HTREEITEM hFuncs = TreeView_InsertItem(g_tree, &tv);
    for (size_t i = 0; i < g_proj.funcs.size(); i++) {
        if (g_proj.funcs[i].excluded) continue;
        memset(&tv, 0, sizeof(tv));
        tv.hInsertAfter = TVI_LAST; tv.hParent = hFuncs;
        tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        int ico = g_proj.funcs[i].type == 0 ? ICO_V : (g_proj.funcs[i].type == 1 ? ICO_M : ICO_U);
        tv.item.iImage = tv.item.iSelectedImage = ico;
        tv.item.pszText = (LPWSTR)g_proj.funcs[i].name.c_str();
        tv.item.lParam = (LPARAM)(100 + i);   // kind=func, idx=i
        TreeView_InsertItem(g_tree, &tv);
    }
    // Licenses klasoru (kind=3)
    tv.hParent = hFile;
    tv.item.iImage = tv.item.iSelectedImage = ICO_FOLDER;
    tv.item.pszText = (LPWSTR)L"Licenses";
    tv.item.lParam = 2;
    HTREEITEM hLics = TreeView_InsertItem(g_tree, &tv);
    for (size_t i = 0; i < g_proj.lics.size(); i++) {
        memset(&tv, 0, sizeof(tv));
        tv.hInsertAfter = TVI_LAST; tv.hParent = hLics;
        tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        tv.item.iImage = tv.item.iSelectedImage = ICO_LIC;
        std::wstring nm = g_proj.lics[i].name.empty() ? (L"License " + numdec(i + 1)) : g_proj.lics[i].name;
        tv.item.pszText = (LPWSTR)nm.c_str();
        tv.item.lParam = (LPARAM)(200 + i);   // kind=lic, idx=i
        TreeView_InsertItem(g_tree, &tv);
    }
    TreeView_Expand(g_tree, hFile, TVE_EXPAND);
    TreeView_Expand(g_tree, hFuncs, TVE_EXPAND);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// ---------------- fonksiyon listesi doldurma ----------------
static std::wstring g_filter_txt;
static void func_fill() {
    ListView_DeleteAllItems(g_func);
    if (!g_proj.open) return;
    static std::wstring tbuf[2];
    wchar_t* cols[5];
    LV_COLUMNW lc; memset(&lc, 0, sizeof(lc));
    // kolonlar bir kez kurulur (WM_INIT icinde)
    int shown = 0;
    for (size_t i = 0; i < g_pe.funcs.size(); i++) {
        const PeFunc& f = g_pe.funcs[i];
        if (!g_filter_txt.empty() && f.name.find(g_filter_txt) == std::wstring::npos)
            continue;
        int pidx = g_proj.find_func(f.name);
        wchar_t tb[3] = { 0,0,0 };
        wchar_t tc = pidx >= 0 ? (g_proj.funcs[pidx].type == 0 ? L'V' : (g_proj.funcs[pidx].type == 1 ? L'M' : L'U')) : L'-';
        tb[0] = tc;
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        it.iItem = shown;
        it.iImage = pidx >= 0 ? (g_proj.funcs[pidx].type == 0 ? ICO_V : (g_proj.funcs[pidx].type == 1 ? ICO_M : ICO_U)) : ICO_FILE;
        it.lParam = (LPARAM)i;
        it.pszText = (LPWSTR)f.name.c_str();
        int idx = ListView_InsertItem(g_func, &it);
        ListView_SetItemText(g_func, idx, 1, tb);
        static wchar_t a1[32], s1[32];
        _snwprintf(a1, 32, L"0x%08llX", f.rva);
        _snwprintf(s1, 32, L"%llu", f.size);
        ListView_SetItemText(g_func, idx, 2, a1);
        ListView_SetItemText(g_func, idx, 3, s1);
        ListView_SetItemText(g_func, idx, 4, (LPWSTR)(f.is_export ? L"Yes" : L"-"));
        ListView_SetItemText(g_func, idx, 5, (LPWSTR)src_name(f.source));
        shown++;
    }
}

// ---------------- ozellik ızgarasi (property editor) ----------------
struct PropRow {
    int kind;               // 0 group, 1 bool, 2 enum, 3 str, 4 ro
    const wchar_t* name;    // sabit isim (statik)
    std::wstring value;
    unsigned long long id;  // cp biti / func idx / lic idx
};
static std::vector<PropRow> g_rows;
static int g_prop_sel_kind = -1;    // -1 yok, 0 core, 2 func idx, 4 lic idx
static int g_prop_sel_idx = -1;

static void prop_add_row(int kind, const wchar_t* name, const std::wstring& val, unsigned long long id) {
    PropRow r; r.kind = kind; r.name = name; r.value = val; r.id = id;
    g_rows.push_back(r);
}
static void fill_prop() {
    ListView_DeleteAllItems(g_prop);
    g_rows.clear();
    static std::wstring vals[256];
    int vi = 0;
    if (g_prop_sel_kind == 0) {
        prop_add_row(0, L(LS_CORE_PROPS), L"", 0);
        prop_add_row(4, L(LS_TARGET), g_proj.target, 0);
        prop_add_row(4, L"Machine", g_pe.is64 ? L"x64" : L"x86", 0);
        prop_add_row(4, L"Image Base", numhex(g_pe.imagebase), 0);
        prop_add_row(4, L"Entry Point", numhex(g_pe.ep), 0);
        prop_add_row(4, L"Subsystem", g_pe.subsystem == 2 ? L"GUI" : (g_pe.subsystem == 3 ? L"Console" : numdec(g_pe.subsystem)), 0);
        prop_add_row(4, L"ASLR", (g_pe.dllchars & 0x40) ? L"Yes" : L"No", 0);
        prop_add_row(4, L"DEP", (g_pe.dllchars & 0x100) ? L"Yes" : L"No", 0);
        // cp* bitleri — core.h birebir
        struct { unsigned long long bit; const wchar_t* nm; } opts[] = {
            { cpDebugMode, L"Debug Mode" },
            { cpCryptValues, L"Crypt Values" },
            { cpIncludeWatermark, L"Include Watermark" },
            { cpRunnerCRC, L"Runner CRC" },
            { cpEncryptRegs, L"Encrypt Registers" },
            { cpStripFixups, L"Strip Fixups" },
            { cpPack, L"Pack the Output File" },
            { cpImportProtection, L(LS_IMPORT) },
            { cpCheckDebugger, L"Check Debugger" },
            { cpCheckVirtualMachine, L"Check Virtual Machine" },
            { cpMemoryProtection, L"Memory Protection" },
            { cpResourceProtection, L"Resource Protection" },
            { cpCheckKernelDebugger, L"Check Kernel Debugger" },
            { cpStripDebugInfo, L"Strip Debug Information" },
            { cpLoaderCRC, L"Loader CRC" },
            { cpEncryptBytecode, L"Encrypt Bytecode" },
            { cpVirtualFiles, L"Virtual Files" },
            { cpInternalMemoryProtection, L"Internal Memory Protection" },
            { cpLoader, L"Loader" },
        };
        prop_add_row(0, L(LS_PROTECTION), L"", 0);
        for (int i = 0; i < 19; i++)
            prop_add_row(1, opts[i].nm, (g_proj.options & opts[i].bit) ? L"Yes" : L"No", opts[i].bit);
    } else if (g_prop_sel_kind == 2 && g_prop_sel_idx >= 0 && (size_t)g_prop_sel_idx < g_proj.funcs.size()) {
        const ProjFunc& f = g_proj.funcs[g_prop_sel_idx];
        prop_add_row(0, L(LS_FUNC_PROPS), L"", 0);
        prop_add_row(4, L(LS_NAME), f.name, 0);
        prop_add_row(4, L(LS_ADDRESS), numhex(f.rva), 0);
        prop_add_row(4, L(LS_SIZE), numdec(f.size), 0);
        prop_add_row(4, L(LS_EXPORT), f.is_export ? L"Yes" : L"No", 0);
        std::wstring ct = f.type == 0 ? L(LS_VIRT) : (f.type == 1 ? L(LS_MUT) : std::wstring(L(LS_ULTRA)) + L" (M+V)");
        vals[vi] = ct;
        prop_add_row(2, L(LS_COMPTYPE), vals[vi], g_prop_sel_idx); vi++;
        vals[vi] = f.lock_to_key ? L"Yes" : L"No";
        prop_add_row(1, L(LS_LOCKKEY), vals[vi], g_prop_sel_idx); vi++;
        vals[vi] = f.excluded ? L"Yes" : L"No";
        prop_add_row(1, L(LS_EXCLUDED), vals[vi], g_prop_sel_idx); vi++;
        vals[vi] = f.ext ? numhex(f.ext) : L"(auto)";
        prop_add_row(3, L(LS_EXTADDR), vals[vi], g_prop_sel_idx); vi++;
        vals[vi] = f.brk ? numhex(f.brk) : L"(auto)";
        prop_add_row(3, L(LS_BREAKADDR), vals[vi], g_prop_sel_idx); vi++;
    } else if (g_prop_sel_kind == 4 && g_prop_sel_idx >= 0 && (size_t)g_prop_sel_idx < g_proj.lics.size()) {
        const ProjLicense& lc = g_proj.lics[g_prop_sel_idx];
        prop_add_row(0, L(LS_LIC_PROPS), L"", 0);
        int base = 100 + g_prop_sel_idx * 10;   // id kodlama: lic alanlari
        vals[vi] = lc.name; prop_add_row(3, L(LS_CUSTNAME), vals[vi], base + 0); vi++;
        vals[vi] = lc.email; prop_add_row(3, L(LS_EMAIL), vals[vi], base + 1); vi++;
        vals[vi] = lc.date; prop_add_row(3, L(LS_DATE), vals[vi], base + 2); vi++;
        vals[vi] = lc.order; prop_add_row(3, L(LS_ORDER), vals[vi], base + 3); vi++;
        vals[vi] = lc.comments; prop_add_row(3, L(LS_COMMENTS), vals[vi], base + 4); vi++;
        prop_add_row(0, L(LS_SERIALCONT), L"", 0);
        vals[vi] = lc.s_name ? L"Yes" : L"No"; prop_add_row(1, L(LS_CUSTNAME), vals[vi], base + 5); vi++;
        vals[vi] = lc.s_email ? L"Yes" : L"No"; prop_add_row(1, L(LS_EMAIL), vals[vi], base + 6); vi++;
        vals[vi] = lc.s_hwid ? L"Yes" : L"No"; prop_add_row(1, L(LS_HWID), vals[vi], base + 7); vi++;
        vals[vi] = lc.s_expire ? (L"Yes / " + lc.s_expire_d) : L"No";
        prop_add_row(1, L(LS_EXPIRE), vals[vi], base + 8); vi++;
        vals[vi] = lc.s_time ? (L"Yes / " + lc.s_time_v) : L"No";
        prop_add_row(1, L(LS_TIMELIMIT), vals[vi], base + 9); vi++;
        vals[vi] = lc.s_maxbuild ? (L"Yes / " + lc.s_maxbuild_d) : L"No";
        prop_add_row(1, L(LS_MAXBUILD), vals[vi], base + 10); vi++;
        vals[vi] = lc.s_userdata ? (L"Yes / " + lc.s_userdata_v) : L"No";
        prop_add_row(1, L(LS_USERDATA), vals[vi], base + 11); vi++;
    }
    // listeye bas
    int shown = 0;
    for (size_t i = 0; i < g_rows.size(); i++) {
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT | LVIF_PARAM;
        it.iItem = shown; it.lParam = (LPARAM)i;
        it.pszText = (LPWSTR)g_rows[i].name;
        int idx = ListView_InsertItem(g_prop, &it);
        if (g_rows[i].kind == 0) {
            std::wstring hd = std::wstring(L"—— ") + g_rows[i].name + L" ——";
            ListView_SetItemText(g_prop, idx, 0, (LPWSTR)hd.c_str());
        } else {
            static std::wstring tmpv[512];
            tmpv[shown % 512] = g_rows[i].value;
            ListView_SetItemText(g_prop, idx, 1, (LPWSTR)tmpv[shown % 512].c_str());
        }
        shown++;
    }
}

// ---------------- menuler (birebir yapi) ----------------
static void add_item(HMENU m, int ls, int id, bool enabled = true, bool checked = false) {
    MENUITEMINFOW mi; memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    mi.wID = id;
    mi.dwTypeData = (LPWSTR)L(ls);
    mi.fState = (enabled ? MFS_ENABLED : MFS_DISABLED) | (checked ? MFS_CHECKED : MFS_UNCHECKED);
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
}
// 0.3.0: ivme ipucu eki (VMP birebir: Open Ctrl+O, Compile F9 ...)
static void add_item_acc(HMENU m, int ls, int id, const wchar_t* acc, bool enabled = true) {
    std::wstring txt = std::wstring(L(ls)) + L"\t" + acc;
    MENUITEMINFOW mi; memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    mi.wID = id;
    mi.dwTypeData = (LPWSTR)txt.c_str();
    mi.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
}
static void add_item_raw(HMENU m, const std::wstring& txt, int id, bool enabled = true) {
    MENUITEMINFOW mi; memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
    mi.wID = id;
    mi.dwTypeData = (LPWSTR)txt.c_str();
    mi.fState = enabled ? MFS_ENABLED : MFS_DISABLED;
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
}
static void add_sep(HMENU m) {
    MENUITEMINFOW mi; memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi); mi.fMask = MIIM_FTYPE; mi.fType = MFT_SEPARATOR;
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
}
static HMENU add_sub(HMENU m, int ls) {
    HMENU sub = CreatePopupMenu();
    MENUITEMINFOW mi; memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_SUBMENU | MIIM_STRING;
    mi.hSubMenu = sub; mi.dwTypeData = (LPWSTR)L(ls);
    InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
    return sub;
}
static void build_menu() {
    if (g_menu) DestroyMenu(g_menu);
    g_menu = CreateMenu();
    // 0.3.0: durum bayraklari artik WM_INITMENUPOPUP ile dinamik guncellenir;
    // menu yalnizca yapi/dil/recent degisince yeniden kurulur (flicker yok).
    HMENU mFile = add_sub(g_menu, LS_FILE);
    add_item_acc(mFile, LS_OPEN, IDM_OPEN, L"Ctrl+O");
    add_item_acc(mFile, LS_SAVE, IDM_SAVE, L"Ctrl+S");
    add_item(mFile, LS_SAVEAS, IDM_SAVEAS);
    add_item(mFile, LS_SAVELIC, IDM_SAVELIC);
    add_item_acc(mFile, LS_CLOSE, IDM_CLOSE, L"Ctrl+W");
    add_sep(mFile);
    for (size_t i = 0; i < g_recent.size(); i++)
        add_item_raw(mFile, g_recent[i], IDM_RECENT_BASE + (int)i, true);
    if (!g_recent.empty()) add_sep(mFile);
    add_item(mFile, LS_EXIT, IDM_EXIT);
    HMENU mEdit = add_sub(g_menu, LS_EDIT);
    add_item_acc(mEdit, LS_UNDO, IDM_UNDO, L"Ctrl+Z");
    add_item_acc(mEdit, LS_REDO, IDM_REDO, L"Ctrl+Y");
    add_sep(mEdit);
    add_item_acc(mEdit, LS_CUT, IDM_CUT, L"Ctrl+X", false);
    add_item_acc(mEdit, LS_COPY, IDM_COPY, L"Ctrl+C", false);
    add_item_acc(mEdit, LS_PASTE, IDM_PASTE, L"Ctrl+V", false);
    add_item_acc(mEdit, LS_DELETE, IDM_DELETE, L"Del");
    add_sep(mEdit);
    add_item_acc(mEdit, LS_SEARCH, IDM_SEARCH, L"Ctrl+F");
    add_item_acc(mEdit, LS_GOTO, IDM_GOTO, L"Ctrl+G");
    HMENU mProj = add_sub(g_menu, LS_PROJECT);
    HMENU mAdd = add_sub(mProj, LS_ADD);
    add_item_acc(mAdd, LS_ADDFUNC, IDM_ADDFUNC, L"Ins");
    add_item(mAdd, LS_ADDLIC, IDM_ADDLIC);
    add_item(mAdd, LS_ADDFILE, IDM_ADDFILE, false);
    add_item(mAdd, LS_ADDFOLDER, IDM_ADDFOLDER, false);
    add_sep(mProj);
    add_item(mProj, LS_EXPORTKEY, IDM_EXPORTKEY);
    HMENU mImp = add_sub(mProj, LS_IMPORT);
    add_item(mImp, LS_IMPLIC, IDM_IMPLIC);
    add_item(mImp, LS_IMPPROJ, IDM_IMPPROJ);
    add_sep(mProj);
    add_item_acc(mProj, LS_COMPILE, IDM_COMPILE, L"F9");
    add_item_acc(mProj, LS_EXECUTE, IDM_EXECUTE, L"F5");
    HMENU mTools = add_sub(g_menu, LS_TOOLS);
    add_item(mTools, LS_WATERMARKS, IDM_WATERMARKS);
    add_item(mTools, LS_DISASM, IDM_DISASM);
    add_item(mTools, LS_SETTINGS, IDM_SETTINGS);
    HMENU mHelp = add_sub(g_menu, LS_HELP);
    add_item_acc(mHelp, LS_CONTENTS, IDM_HELP, L"F1");
    add_sep(mHelp);
    add_item(mHelp, LS_HOMEPAGE, IDM_HOMEPAGE);
    add_item(mHelp, LS_ABOUT, IDM_ABOUT);
    SetMenu(g_main, g_menu);
    DrawMenuBar(g_main);
}
static void build_accel() {
    if (g_accel) DestroyAcceleratorTable(g_accel);
    ACCEL a[] = {
        { FCONTROL | FVIRTKEY, 'O', IDM_OPEN },
        { FCONTROL | FVIRTKEY, 'S', IDM_SAVE },
        { FCONTROL | FVIRTKEY, 'W', IDM_CLOSE },
        { FCONTROL | FVIRTKEY, 'F', IDM_SEARCH },
        { FCONTROL | FVIRTKEY, 'G', IDM_GOTO },
        { FCONTROL | FVIRTKEY, 'I', IDM_IMPLIC },
        { FCONTROL | FVIRTKEY, 'Z', IDM_UNDO },
        { FCONTROL | FVIRTKEY, 'Y', IDM_REDO },
        { FVIRTKEY, VK_F9, IDM_COMPILE },
        { FVIRTKEY, VK_F5, IDM_EXECUTE },
        { FVIRTKEY, VK_F1, IDM_HELP },
        { FVIRTKEY, VK_F2, 0x9001 },   // rename (context)
        { FVIRTKEY, VK_INSERT, IDM_ADDFUNC },
        { FVIRTKEY, VK_DELETE, IDM_DELETE },
    };
    g_accel = CreateAcceleratorTableW(a, 14);
}

// ---------------- toolbar ----------------
static void tb_add(HWND tb, int id, const wchar_t* txt, unsigned char style = BTNS_BUTTON) {
    TBBUTTON b; memset(&b, 0, sizeof(b));
    b.idCommand = id; b.iBitmap = I_IMAGENONE;
    b.fsState = TBSTATE_ENABLED;
    b.fsStyle = (unsigned char)(style | BTNS_SHOWTEXT | BTNS_AUTOSIZE);
    b.iString = (INT_PTR)txt;
    SendMessageW(tb, TB_ADDBUTTONS, 1, (LPARAM)&b);
}
static void tb_sep(HWND tb) {
    TBBUTTON b; memset(&b, 0, sizeof(b));
    b.fsStyle = BTNS_SEP;
    SendMessageW(tb, TB_ADDBUTTONS, 1, (LPARAM)&b);
}
static void rebuild_toolbar() {
    if (g_toolbar) DestroyWindow(g_toolbar);
    g_toolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NORESIZE,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_TOOLBAR, NULL, NULL);
    SendMessageW(g_toolbar, WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(g_toolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
    tb_add(g_toolbar, IDM_OPEN, L(LS_OPEN));
    tb_add(g_toolbar, IDM_SAVE, L(LS_SAVE));
    tb_sep(g_toolbar);
    tb_add(g_toolbar, IDM_COMPILE, L(LS_COMPILE));
    tb_add(g_toolbar, IDM_EXECUTE, L(LS_EXECUTE), BTNS_WHOLEDROPDOWN);
    tb_sep(g_toolbar);
    tb_add(g_toolbar, IDM_CUT, L(LS_CUT));
    tb_add(g_toolbar, IDM_COPY, L(LS_COPY));
    tb_add(g_toolbar, IDM_PASTE, L(LS_PASTE));
    tb_sep(g_toolbar);
    tb_add(g_toolbar, IDM_ADDFUNC, L(LS_ADDFUNC));
    tb_add(g_toolbar, IDM_GOTO, L(LS_GOTO));
    tb_sep(g_toolbar);
    tb_add(g_toolbar, IDM_TPL_SAVE, L(LS_TEMPLATES), BTNS_WHOLEDROPDOWN);
    SendMessageW(g_toolbar, TB_AUTOSIZE, 0, 0);
    update_toolbar_states();
    layout();
}
// 0.3.0: toolbar bir kez kurulur; durumlar burada guncellenir (flicker yok)
static void update_toolbar_states() {
    if (!g_toolbar) return;
    bool open = g_proj.open;
    SendMessageW(g_toolbar, TB_ENABLEBUTTON, IDM_SAVE, MAKELPARAM(open, 0));
    SendMessageW(g_toolbar, TB_ENABLEBUTTON, IDM_COMPILE, MAKELPARAM(open && !g_compiling, 0));
    SendMessageW(g_toolbar, TB_ENABLEBUTTON, IDM_EXECUTE, MAKELPARAM(open, 0));
    SendMessageW(g_toolbar, TB_ENABLEBUTTON, IDM_ADDFUNC, MAKELPARAM(open, 0));
    SendMessageW(g_toolbar, TB_ENABLEBUTTON, IDM_GOTO, MAKELPARAM(open, 0));
    // 0.3.0 DUZELTME (UI bug #3): filtre kutusu artik toolbar satirinda DEGIL,
    // Fonksiyonlar sekmesinin kendi seridinde — kucuk pencerede dugmelerin
    // uzerine binmiyor.
}
// ============================================================
// xprotect_ui.cpp — Part C: diyaloglar, derleme, proje I/O, WndProc
// ============================================================

// ---- ozel mesajlar ----
#define WM_APP_LOG        (WM_APP + 1)
#define WM_APP_DONE       (WM_APP + 2)
#define WM_APP_MODAL_OK   (WM_APP + 10)
#define WM_APP_MODAL_CANCEL (WM_APP + 11)

static void post_log_line(const std::string& s) {
    int wl = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (wl <= 0) return;
    wchar_t* w = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w, wl);
    PostMessageW(g_main, WM_APP_LOG, 0, (LPARAM)w);
}

// ---------------- modal yardimcisi ----------------
// 0.3.0 DUZELTME: pencereyi burada YIKMAZ — cagiran, kontrol degerlerini
// OKDUKTAN (read-before-destroy) sonra kendisi yikar. 0.2.0'daki
// "degerler bos donuyor" hatasinin kok nedeni buydu.
static bool modal_run(HWND w, bool* okout) {
    *okout = false;
    EnableWindow(g_main, FALSE);
    ShowWindow(w, SW_SHOW);
    UpdateWindow(w);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_APP_MODAL_OK && (m.hwnd == w || IsChild(w, m.hwnd))) { *okout = true; break; }
        if (m.message == WM_APP_MODAL_CANCEL && (m.hwnd == w || IsChild(w, m.hwnd))) break;
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        if (!IsDialogMessageW(w, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    EnableWindow(g_main, TRUE);
    SetActiveWindow(g_main);
    return *okout;
}
static HWND dlg_base(const wchar_t* title, int w, int h) {
    RECT r; GetWindowRect(g_main, &r);
    int x = r.left + ((r.right - r.left) - w) / 2;
    int y = r.top + ((r.bottom - r.top) - h) / 2;
    HWND d = CreateWindowExW(WS_EX_DLGMODALFRAME, L"XPROTECT_DLG", title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h, g_main, NULL, NULL, NULL);
    SendMessageW(d, WM_SETFONT, (WPARAM)g_font, TRUE);
    return d;
}
static LRESULT CALLBACK dlg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) PostMessageW(h, WM_APP_MODAL_OK, 0, 0);
        if (LOWORD(wp) == IDCANCEL) PostMessageW(h, WM_APP_MODAL_CANCEL, 0, 0);
        break;
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm && nm->code == TCN_SELCHANGE)
            PostMessageW(h, WM_APP + 30, TabCtrl_GetCurSel(nm->hwndFrom), 0);
        break;
    }
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_CLOSE) { PostMessageW(h, WM_APP_MODAL_CANCEL, 0, 0); return 0; }
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { PostMessageW(h, WM_APP_MODAL_CANCEL, 0, 0); return 0; }
        if (wp == VK_RETURN) { PostMessageW(h, WM_APP_MODAL_OK, 0, 0); return 0; }
        break;
    }
    return DefWindowProcW(h, msg, wp, lp);
}
static void register_dlg_class() {
    WNDCLASSW wc; memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = dlg_proc; wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"XPROTECT_DLG";
    RegisterClassW(&wc);
}
static void ctl_txt(HWND e, std::wstring* out) {
    int n = GetWindowTextLengthW(e) + 1;
    std::vector<wchar_t> b(n);
    GetWindowTextW(e, b.data(), n);
    *out = b.data();
}
static void set_txt(HWND e, const std::wstring& s) { SetWindowTextW(e, s.c_str()); }

static std::string w2s(const std::wstring& w) {   // UTF-16 -> UTF-8 (motor char* yollari)
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, NULL, NULL);
    return s;
}
static std::wstring s2w(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    std::vector<wchar_t> w(n ? n : 1);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w.data();
}

// ---------------- dosya diyaloglari ----------------
static bool pick_file(HWND parent, bool save, const wchar_t* defext, std::wstring* out) {
    wchar_t buf[MAX_PATH]; buf[0] = 0;
    OPENFILENAMEW of; memset(&of, 0, sizeof(of));
    of.lStructSize = sizeof(of); of.hwndOwner = parent;
    of.lpstrFilter = L(LS_CDFILTER);
    of.lpstrFile = buf; of.nMaxFile = MAX_PATH;
    of.lpstrDefExt = defext;
    of.Flags = OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (save ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of)) { *out = buf; return true; }
    return false;
}

// ---------------- undo / redo ----------------
static void push_undo() {
    g_undo.push_back(g_proj.funcs);
    if (g_undo.size() > 50) g_undo.erase(g_undo.begin());
    g_redo.clear();
}
static void do_undo() {
    if (g_undo.empty()) return;
    g_redo.push_back(g_proj.funcs);
    g_proj.funcs = g_undo.back(); g_undo.pop_back();
    g_proj.modified = true; update_all();
}
static void do_redo() {
    if (g_redo.empty()) return;
    g_undo.push_back(g_proj.funcs);
    g_proj.funcs = g_redo.back(); g_redo.pop_back();
    g_proj.modified = true; update_all();
}

// ---------------- tab gecisleri ----------------
static void func_cols_setup() {
    while (ListView_DeleteColumn(g_func, 0)) {}
    wchar_t* cols[6] = { (wchar_t*)L(LS_NAME), (wchar_t*)L(LS_TYPE), (wchar_t*)L(LS_ADDRESS),
                         (wchar_t*)L(LS_SIZE), (wchar_t*)L(LS_EXPORT), (wchar_t*)L(LS_SOURCE) };
    LVCOLUMNW c; memset(&c, 0, sizeof(c));
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    int wds[6] = { 210, 60, 90, 70, 50, 60 };
    for (int i = 0; i < 6; i++) { c.pszText = cols[i]; c.cx = wds[i]; ListView_InsertColumn(g_func, i, &c); }
}
static void details_cols_setup() {
    while (ListView_DeleteColumn(g_func, 0)) {}
    wchar_t* cols[6] = { (wchar_t*)L"Item", (wchar_t*)L(LS_VALUE), (wchar_t*)L"VA",
                         (wchar_t*)L"VSize", (wchar_t*)L"RawSize", (wchar_t*)L"Chars" };
    LVCOLUMNW c; memset(&c, 0, sizeof(c));
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    int wds[6] = { 130, 110, 80, 70, 70, 70 };
    for (int i = 0; i < 6; i++) { c.pszText = cols[i]; c.cx = wds[i]; ListView_InsertColumn(g_func, i, &c); }
}
static void details_fill_rows() {
    ListView_DeleteAllItems(g_func);
    if (!g_proj.open) return;
    int shown = 0;
    wchar_t v1[64], v2[64], v3[64], v4[64], v5[64];
    struct Row { const wchar_t* k; const wchar_t* v; };
    std::wstring mb = g_pe.is64 ? L"x64" : L"x86";
    _snwprintf(v1, 64, L"0x%04X", g_pe.machine);
    _snwprintf(v2, 64, g_pe.is64 ? L"0x%012llX" : L"0x%08llX", g_pe.imagebase);
    _snwprintf(v3, 64, L"0x%08llX", g_pe.ep);
    Row rows[] = {
        { L"Machine", mb.c_str() },
        { L"Machine (raw)", v1 },
        { L"Image Base", v2 },
        { L"Entry Point", v3 },
        { L"Subsystem", g_pe.subsystem == 2 ? L"GUI" : (g_pe.subsystem == 3 ? L"Console" : L"other") },
        { L"ASLR (DYNAMIC_BASE)", (g_pe.dllchars & 0x40) ? L"Yes" : L"No" },
        { L"DEP (NX_COMPAT)", (g_pe.dllchars & 0x100) ? L"Yes" : L"No" },
        { L"CFG (GUARD_CF)", (g_pe.dllchars & 0x4000) ? L"Yes" : L"No" },
        { L"Sections", NULL },
    };
    for (int i = 0; i < 9; i++) {
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT; it.iItem = shown; it.pszText = (LPWSTR)rows[i].k;
        int idx = ListView_InsertItem(g_func, &it);
        if (rows[i].v) ListView_SetItemText(g_func, idx, 1, (LPWSTR)rows[i].v);
        shown++;
    }
    for (size_t i = 0; i < g_pe.secs.size(); i++) {
        const PeSection& s = g_pe.secs[i];
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT; it.iItem = shown; it.pszText = (LPWSTR)s.name.c_str();
        int idx = ListView_InsertItem(g_func, &it);
        static wchar_t b1[8][40], b2[8][40], b3[8][40], b4[8][40];
        int k = shown % 8;
        _snwprintf(b1[k], 40, L"0x%08llX", s.va);
        _snwprintf(b2[k], 40, L"0x%llX", s.vsz);
        _snwprintf(b3[k], 40, L"0x%llX", s.rawsz);
        _snwprintf(b4[k], 40, L"0x%08llX", s.chars);
        ListView_SetItemText(g_func, idx, 2, b1[k]);
        ListView_SetItemText(g_func, idx, 3, b2[k]);
        ListView_SetItemText(g_func, idx, 4, b3[k]);
        ListView_SetItemText(g_func, idx, 5, b4[k]);
        shown++;
    }
}
static void switch_tab(int t) {
    g_tab = t;
    ShowWindow(g_tree, t == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_func, t == 0 ? SW_HIDE : SW_SHOW);
    if (t == 1) { func_cols_setup(); func_fill(); }
    if (t == 2) { details_cols_setup(); details_fill_rows(); }
    layout();   // 0.3.0: filtre seridi gosterimi sekme ile senkron
    // tabstrip isaretleri
    SendMessageW(g_tabstrip, TB_SETSTATE, IDC_TBB_PROJECT, MAKELPARAM((t == 0 ? TBSTATE_CHECKED : TBSTATE_ENABLED), 0));
    SendMessageW(g_tabstrip, TB_SETSTATE, IDC_TBB_FUNCS, MAKELPARAM((t == 1 ? TBSTATE_CHECKED : TBSTATE_ENABLED), 0));
    SendMessageW(g_tabstrip, TB_SETSTATE, IDC_TBB_DETAILS, MAKELPARAM((t == 2 ? TBSTATE_CHECKED : TBSTATE_ENABLED), 0));
}

// ---------------- diyalog: Hakkinda (about_dialog.cc birebir akis) ----------------
static void dlg_about() {
    HWND d = dlg_base(L(LS_ABOUT), 400, 210);
    mk_ctl(d, L"STATIC", XPROTECT_EDITION L" v " XPROTECT_VERSION, 0, 0, 20, 20, 340, 24);
    SendDlgItemMessageW(d, 0, WM_SETFONT, (WPARAM)g_font_bold, TRUE);
    mk_ctl(d, L"STATIC", (std::wstring(L"build ") + XPROTECT_BUILD).c_str(), 0, 0, 20, 46, 340, 18);
    mk_ctl(d, L"STATIC", L"universalObfusPublicSrc (VMProtect 3.5.1 kaynak agaci) tabanli C++ portu", 0, 0, 20, 70, 350, 18);
    mk_ctl(d, L"STATIC", L(LS_UNREG), 0, 0, 20, 94, 340, 18);
    HWND ok = mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 290, 150, 90, 26);
    (void)ok;
    bool okb; modal_run(d, &okb);
    DestroyWindow(d);   // 0.3.0: okuma sonrasi yik
}

// ---------------- diyalog: Ayarlar (settings_dialog.cc birebir) ----------------
static void dlg_settings() {
    HWND d = dlg_base(L(LS_SETTINGS), 420, 170);
    mk_ctl(d, L"STATIC", L(LS_LANGUAGE), 0, 0, 20, 22, 150, 18);
    HWND cbLang = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0, 175, 20, 210, 120);
    ComboBox_AddString(cbLang, L"English");
    ComboBox_AddString(cbLang, L"Türkçe");
    ComboBox_SetCurSel(cbLang, g_tr ? 1 : 0);
    mk_ctl(d, L"STATIC", L(LS_AUTOSAVE), 0, 0, 20, 56, 150, 18);
    HWND cbAuto = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0, 175, 54, 210, 120);
    ComboBox_AddString(cbAuto, L"No");
    ComboBox_AddString(cbAuto, L"Yes");
    ComboBox_SetCurSel(cbAuto, g_autosave ? 1 : 0);
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 215, 105, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 312, 105, 88, 26);
    bool ok; modal_run(d, &ok);
    if (ok) {
        // 0.3.0: DEGERLERI YIKMADAN ONCE OKU (read-before-destroy)
        bool tr = ComboBox_GetCurSel(cbLang) == 1;
        bool as = ComboBox_GetCurSel(cbAuto) == 1;
        bool lang_changed = tr != g_tr;
        g_tr = tr; g_autosave = as;
        save_settings();
        if (lang_changed) {
            build_menu();            // etiketler degisti
            rebuild_toolbar();
            update_welcome_texts();
            update_all();           // tum yuzeyler yeniden doldurulur
        }
    }
    DestroyWindow(d);
}

// ---------------- diyalog: Fonksiyon Ekle / Adrese Git (function_dialog.cc) ----------------
struct AFState {
    bool goto_mode;
    HWND addr, list, filter;
    HWND combo_type, chk_lock;
    int type; bool lock;
};
static void af_fill_list(AFState* st) {
    ListView_DeleteAllItems(st->list);
    if (!g_proj.open) return;
    std::wstring flt; ctl_txt(st->filter, &flt);
    int shown = 0;
    for (size_t i = 0; i < g_pe.funcs.size(); i++) {
        const PeFunc& f = g_pe.funcs[i];
        if (!flt.empty() && f.name.find(flt) == std::wstring::npos) continue;
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT | LVIF_PARAM; it.iItem = shown; it.lParam = (LPARAM)i;
        it.pszText = (LPWSTR)f.name.c_str();
        int idx = ListView_InsertItem(st->list, &it);
        static wchar_t ab[512], sb[512];
        _snwprintf(ab, 512, L"0x%08llX", f.rva);
        _snwprintf(sb, 512, L"%llu", f.size);
        ListView_SetItemText(st->list, idx, 1, ab);
        ListView_SetItemText(st->list, idx, 2, sb);
        ListView_SetItemText(st->list, idx, 3, (LPWSTR)(f.is_export ? L"Yes" : L"-"));
        ListView_SetItemText(st->list, idx, 4, (LPWSTR)src_name(f.source));
        shown++;
    }
}
static bool addr_in_exec(unsigned long long a) {
    for (auto& s : g_pe.secs)
        if (s.chars & 0x20000000 && a >= s.va && a < s.va + s.vsz) return true;
    return false;
}
static std::wstring sym_by_addr(unsigned long long a) {
    for (auto& f : g_pe.funcs) if (f.rva == a) return f.name;
    wchar_t b[32]; _snwprintf(b, 32, L"sub_%08llX", a); return b;
}
static void dlg_addfunction(bool goto_mode, unsigned long long def_addr) {
    AFState st; st.goto_mode = goto_mode;
    HWND d = dlg_base(goto_mode ? L(LS_GOTO) : L(LS_ADDFUNC), 560, 430);
    mk_ctl(d, L"STATIC", L(LS_ADDRESS), 0, 0, 16, 18, 60, 18);
    st.addr = mk_ctl(d, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0x501, 80, 15, 250, 22);
    if (def_addr) { wchar_t b[32]; _snwprintf(b, 32, L"%llX", def_addr); set_txt(st.addr, b); }
    HWND tab = mk_ctl(d, WC_TABCONTROLW, L"", WS_TABSTOP | WS_CLIPSIBLINGS, 0x502, 10, 44, 530, 300);
    TCITEMW ti; memset(&ti, 0, sizeof(ti)); ti.mask = TCIF_TEXT;
    ti.pszText = (LPWSTR)L(LS_FUNCS); TabCtrl_InsertItem(tab, 0, &ti);
    ti.pszText = (LPWSTR)L(LS_OPTIONS); TabCtrl_InsertItem(tab, 1, &ti);
    // sayfa 0: fonksiyon listesi (PDB/MAP/stub dahil — Source kolonu)
    st.filter = mk_ctl(d, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0x503, 20, 76, 300, 22);
    st.list = mk_ctl(d, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP |
                     (goto_mode ? LVS_SINGLESEL : 0), 0x504, 20, 102, 505, 220);
    ListView_SetExtendedListViewStyle(st.list, LVS_EX_FULLROWSELECT);
    LVCOLUMNW c; memset(&c, 0, sizeof(c)); c.mask = LVCF_TEXT | LVCF_WIDTH;
    wchar_t* cols[5] = { (wchar_t*)L(LS_NAME), (wchar_t*)L(LS_ADDRESS), (wchar_t*)L(LS_SIZE),
                         (wchar_t*)L(LS_EXPORT), (wchar_t*)L(LS_SOURCE) };
    int wds[5] = { 250, 90, 70, 50, 60 };
    for (int i = 0; i < 5; i++) { c.pszText = cols[i]; c.cx = wds[i]; ListView_InsertColumn(st.list, i, &c); }
    af_fill_list(&st);
    // sayfa 1: options (sadece add modu — VMProtect: smAddFunction)
    HWND pg1 = mk_ctl(d, L"BUTTON", L(LS_PROTECTION), BS_GROUPBOX, 0x505, 20, 76, 505, 100);
    HWND lbl_type = mk_ctl(d, L"STATIC", L(LS_COMPTYPE), 0, 0, 36, 104, 120, 18);
    st.combo_type = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0x506, 160, 100, 220, 120);
    ComboBox_AddString(st.combo_type, L(LS_VIRT));
    ComboBox_AddString(st.combo_type, L(LS_MUT));
    ComboBox_AddString(st.combo_type, (std::wstring(L(LS_ULTRA)) + L" (" + L(LS_MUT) + L" + " + L(LS_VIRT) + L")").c_str());
    ComboBox_SetCurSel(st.combo_type, 0);
    HWND chk = mk_ctl(d, L"BUTTON", L(LS_LOCKKEY), BS_AUTOCHECKBOX | WS_TABSTOP, 0x507, 36, 134, 300, 20);
    // 0.3.0 DUZELTME (UI bug #2): iki sekme sayfasi ayni anda gorunup
    // ust uste biniyordu — sayfa-1 kontrolleri BASLANGICTA gizlenir.
    ShowWindow(pg1, SW_HIDE);
    ShowWindow(lbl_type, SW_HIDE);
    ShowWindow(st.combo_type, SW_HIDE);
    ShowWindow(chk, SW_HIDE);
    mk_ctl(d, L"BUTTON", goto_mode ? L(LS_GOTO) : L(LS_ADDFUNC), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 350, 356, 96, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 452, 356, 88, 26);
    if (!g_pe.sym_note.empty())
        SetWindowTextW(d, (std::wstring(goto_mode ? L(LS_GOTO) : L(LS_ADDFUNC)) + L"   [" + g_pe.sym_note + L"]").c_str());
    SetFocus(st.addr);
    // tab gecisi: TCN_SELCHANGE -> WM_APP+30 (dlg_proc)
    bool okout = false;
    EnableWindow(g_main, FALSE);
    ShowWindow(d, SW_SHOW); UpdateWindow(d);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_APP_MODAL_OK && (m.hwnd == d || IsChild(d, m.hwnd))) { okout = true; break; }
        if (m.message == WM_APP_MODAL_CANCEL && (m.hwnd == d || IsChild(d, m.hwnd))) break;
        if (m.message == WM_APP + 30) {
            int p = (int)m.wParam;
            ShowWindow(st.filter, p == 0 ? SW_SHOW : SW_HIDE);
            ShowWindow(st.list, p == 0 ? SW_SHOW : SW_HIDE);
            ShowWindow(pg1, p == 1 ? SW_SHOW : SW_HIDE);
            ShowWindow(lbl_type, p == 1 && !goto_mode ? SW_SHOW : SW_HIDE);
            ShowWindow(st.combo_type, p == 1 && !goto_mode ? SW_SHOW : SW_HIDE);
            ShowWindow(chk, p == 1 && !goto_mode ? SW_SHOW : SW_HIDE);
            continue;
        }
        // 0.3.0: liste bildirimleri — secim -> adres doldur (VMP davranisi),
        // cift tiklama -> OK
        if (m.message == WM_NOTIFY && m.hwnd == d) {
            NMHDR* nm = (NMHDR*)m.lParam;
            if (nm && nm->hwndFrom == st.list) {
                if (nm->code == LVN_ITEMCHANGED) {
                    NMLISTVIEW* lv = (NMLISTVIEW*)m.lParam;
                    if ((lv->uNewState & LVIS_SELECTED) && lv->iItem >= 0) {
                        LVITEMW it; memset(&it, 0, sizeof(it));
                        it.mask = LVIF_PARAM; it.iItem = lv->iItem;
                        if (ListView_GetItem(st.list, &it) && (size_t)it.lParam < g_pe.funcs.size()) {
                            wchar_t b[32]; _snwprintf(b, 32, L"%llX", g_pe.funcs[it.lParam].rva);
                            set_txt(st.addr, b);
                        }
                    }
                } else if (nm->code == NM_DBLCLK) {
                    NMITEMACTIVATE* ia = (NMITEMACTIVATE*)m.lParam;
                    if (ia->iItem >= 0) { okout = true; break; }
                }
            }
            // filtre degisimi -> listeyi yeniden doldur
            if (nm && nm->hwndFrom == st.filter && nm->code == EN_CHANGE) af_fill_list(&st);
            if (!IsDialogMessageW(d, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
            continue;
        }
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        if (!IsDialogMessageW(d, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    EnableWindow(g_main, TRUE); SetActiveWindow(g_main);
    // 0.3.0 DUZELTME (UI bug #1): kontrol degerleri YIKMADAN ONCE okunur
    std::wstring addr_txt; ctl_txt(st.addr, &addr_txt);
    int type_sel = ComboBox_GetCurSel(st.combo_type);
    bool lock_chk = Button_GetCheck(chk) != BST_UNCHECKED;
    DestroyWindow(d);
    if (!okout) return;
    // OK akisi
    if (goto_mode) {
        // adrese git: fonksiyon listesinde sec
        unsigned long long a = wcstoull(addr_txt.c_str(), NULL, 16);
        int found = -1;
        for (size_t i = 0; i < g_proj.funcs.size(); i++)
            if (g_proj.funcs[i].rva == a) { found = (int)i; break; }
        if (found >= 0) {
            g_prop_sel_kind = 2; g_prop_sel_idx = found;
            fill_prop();
            // project agacinda sec
            switch_tab(0);
        } else {
            add_log(std::wstring(L"GoTo: ") + numhex(a) + L" not found in project functions");
        }
        return;
    }
    // add mode: adres listesi (virgul/Space ayracli, hex)
    std::vector<unsigned long long> addrs;
    std::wstring cur;
    for (size_t i = 0; i <= addr_txt.size(); i++) {
        wchar_t ch = i < addr_txt.size() ? addr_txt[i] : L',';
        if (ch == L',' || ch == L' ' || ch == L'\t') {
            if (!cur.empty()) { addrs.push_back(wcstoull(cur.c_str(), NULL, 16)); cur.clear(); }
        } else cur += ch;
    }
    if (addrs.empty()) { MessageBoxW(g_main, L"Invalid address", L(LS_ERROR), MB_ICONERROR); return; }
    push_undo();
    int added = 0;
    for (auto a : addrs) {
        if (!addr_in_exec(a)) {
            MessageBoxW(g_main, (std::wstring(L"Invalid address: ") + numhex(a)).c_str(), L(LS_ERROR), MB_ICONERROR);
            continue;
        }
        bool dup = false;
        for (auto& f : g_proj.funcs) if (f.rva == a) { dup = true; break; }
        if (dup) { MessageBoxW(g_main, L(LS_ALREADY_ADDED), L(LS_ERROR), MB_ICONWARNING); continue; }
        ProjFunc pf;
        pf.name = sym_by_addr(a);
        pf.rva = a;
        pf.src = 0;
        for (auto& f : g_pe.funcs) if (f.rva == a) { pf.size = f.size; pf.is_export = f.is_export; pf.src = f.source; }
        pf.type = type_sel;
        pf.lock_to_key = lock_chk;
        g_proj.funcs.push_back(pf);
        added++;
    }
    if (added) {
        g_proj.modified = true;
        add_log(L(LS_FUNC_ADDED));
        update_all();
    }
}

// ---------------- diyalog: Filigranlar + Filigran Ekle (watermark_dialog.cc) ----------------
static void gen_watermark(std::wstring* out) {
    wchar_t b[20]; srand((unsigned)time(NULL) ^ GetCurrentProcessId());
    for (int i = 0; i < 16; i++) {
        int d = rand() % 16;
        b[i] = (wchar_t)(d < 10 ? L'0' + d : L'A' + d - 10);
    }
    b[16] = 0; *out = b;
}
static bool dlg_addwm(std::wstring* name, std::wstring* value) {
    HWND d = dlg_base(L(LS_ADDWM), 400, 180);
    mk_ctl(d, L"STATIC", L(LS_DETAILS), 0, 0, 16, 16, 200, 18);
    mk_ctl(d, L"STATIC", L(LS_NAME), 0, 0, 16, 44, 80, 18);
    HWND eName = mk_ctl(d, L"EDIT", L"", WS_BORDER | WS_TABSTOP, 0x601, 100, 41, 270, 22);
    mk_ctl(d, L"STATIC", L(LS_VALUE), 0, 0, 16, 72, 80, 18);
    HWND eVal = mk_ctl(d, L"EDIT", L"", WS_BORDER | WS_TABSTOP, 0x602, 100, 69, 270, 22);
    mk_ctl(d, L"BUTTON", L(LS_GENERATE), WS_TABSTOP, 0x603, 16, 108, 100, 26);
    mk_ctl(d, L"BUTTON", L(LS_ADDWM), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 196, 108, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 292, 108, 88, 26);
    bool okout = false;
    EnableWindow(g_main, FALSE); ShowWindow(d, SW_SHOW); UpdateWindow(d);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_COMMAND && m.hwnd == d) {
            if (LOWORD(m.wParam) == 0x603) { std::wstring g; gen_watermark(&g); set_txt(eVal, g); continue; }
            if (LOWORD(m.wParam) == IDOK) { okout = true; break; }
            if (LOWORD(m.wParam) == IDCANCEL) break;
        }
        if (m.message == WM_APP_MODAL_OK) { okout = true; break; }
        if (m.message == WM_APP_MODAL_CANCEL) break;
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        if (!IsDialogMessageW(d, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    EnableWindow(g_main, TRUE); SetActiveWindow(g_main);
    if (okout) { ctl_txt(eName, name); ctl_txt(eVal, value); }
    DestroyWindow(d);
    return okout && !name->empty();
}
static void dlg_watermarks() {
    HWND d = dlg_base(L(LS_WATERMARKS), 520, 360);
    HWND lv = mk_ctl(d, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP, 0x610, 14, 14, 480, 240);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT);
    LVCOLUMNW c; memset(&c, 0, sizeof(c)); c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)L(LS_NAME); c.cx = 200; ListView_InsertColumn(lv, 0, &c);
    c.pszText = (LPWSTR)L(LS_VALUE); c.cx = 260; ListView_InsertColumn(lv, 1, &c);
    auto refill = [&]() {
        ListView_DeleteAllItems(lv);
        for (size_t i = 0; i < g_proj.wms.size(); i++) {
            LVITEMW it; memset(&it, 0, sizeof(it));
            it.mask = LVIF_TEXT; it.iItem = (int)i; it.pszText = (LPWSTR)g_proj.wms[i].name.c_str();
            int idx = ListView_InsertItem(lv, &it);
            ListView_SetItemText(lv, idx, 1, (LPWSTR)g_proj.wms[i].value.c_str());
        }
    };
    refill();
    mk_ctl(d, L"BUTTON", L(LS_ADDWM), WS_TABSTOP, 0x611, 14, 270, 110, 26);
    mk_ctl(d, L"BUTTON", L(LS_DELETE), WS_TABSTOP, 0x612, 130, 270, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CLOSE), BS_DEFPUSHBUTTON | WS_TABSTOP, IDCANCEL, 404, 270, 90, 26);
    EnableWindow(g_main, FALSE); ShowWindow(d, SW_SHOW); UpdateWindow(d);
    MSG m;
    bool done = false;
    while (!done && GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_COMMAND && m.hwnd == d) {
            if (LOWORD(m.wParam) == 0x611) {
                std::wstring n, v;
                if (dlg_addwm(&n, &v)) { ProjWatermark w; w.name = n; w.value = v; g_proj.wms.push_back(w); g_proj.modified = true; refill(); }
                continue;
            }
            if (LOWORD(m.wParam) == 0x612) {
                int sel = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
                if (sel >= 0) { g_proj.wms.erase(g_proj.wms.begin() + sel); g_proj.modified = true; refill(); }
                continue;
            }
            if (LOWORD(m.wParam) == IDCANCEL) done = true;
            continue;
        }
        if (m.message == WM_APP_MODAL_CANCEL || m.message == WM_APP_MODAL_OK) break;
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        TranslateMessage(&m); DispatchMessageW(&m);
    }
    EnableWindow(g_main, TRUE); SetActiveWindow(g_main);
    DestroyWindow(d);
    update_all();
}
// ============================================================
// xprotect_ui.cpp — Part D: lisans diyaloglari, derleme, proje I/O
// ============================================================

// ---------------- diyalog: Lisans Ekle (license_dialog.cc birebir alan seti) ----------------
static bool dlg_addlicense() {
    HWND d = dlg_base(L(LS_ADDLIC), 470, 560);
    mk_ctl(d, L"STATIC", L(LS_DETAILS), 0, 0, 16, 14, 200, 18);
    auto lbl_edit = [&](int y, const wchar_t* lab, int id, const wchar_t* defv = L"") -> HWND {
        mk_ctl(d, L"STATIC", lab, 0, 0, 16, y + 3, 130, 18);
        HWND e = mk_ctl(d, L"EDIT", defv, WS_BORDER | WS_TABSTOP, id, 150, y, 290, 22);
        return e;
    };
    HWND eName = lbl_edit(36, L(LS_CUSTNAME), 0x701);
    HWND eMail = lbl_edit(64, L(LS_EMAIL), 0x702);
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t today[24]; _snwprintf(today, 24, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    HWND eDate = lbl_edit(92, L(LS_DATE), 0x703, today);
    HWND eOrder = lbl_edit(120, L(LS_ORDER), 0x704);
    mk_ctl(d, L"STATIC", L(LS_COMMENTS), 0, 0, 16, 152, 130, 18);
    HWND eComments = mk_ctl(d, L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP, 0x705, 150, 148, 290, 44);
    // serial number contents
    mk_ctl(d, L"BUTTON", L(LS_SERIALCONT), BS_GROUPBOX, 0, 14, 202, 430, 220);
    struct SRow { int id; const wchar_t* lab; int y; bool* flag; std::wstring* val; };
    std::wstring exp_d = today, maxb_d = today, time_v, udata_v;
    bool f_name = false, f_mail = false, f_hwid = false, f_exp = false, f_time = false, f_maxb = false, f_udata = false;
    mk_ctl(d, L"BUTTON", L(LS_CUSTNAME), BS_AUTOCHECKBOX | WS_TABSTOP, 0x710, 26, 222, 200, 20);
    mk_ctl(d, L"BUTTON", L(LS_EMAIL), BS_AUTOCHECKBOX | WS_TABSTOP, 0x711, 26, 246, 200, 20);
    mk_ctl(d, L"BUTTON", L(LS_HWID), BS_AUTOCHECKBOX | WS_TABSTOP, 0x712, 26, 270, 200, 20);
    mk_ctl(d, L"BUTTON", L(LS_EXPIRE), BS_AUTOCHECKBOX | WS_TABSTOP, 0x713, 26, 294, 140, 20);
    mk_ctl(d, L"EDIT", exp_d.c_str(), WS_BORDER | WS_TABSTOP, 0x713e, 170, 292, 90, 22);
    mk_ctl(d, L"BUTTON", L(LS_TIMELIMIT), BS_AUTOCHECKBOX | WS_TABSTOP, 0x714, 26, 318, 140, 20);
    mk_ctl(d, L"EDIT", L"", WS_BORDER | WS_TABSTOP, 0x714e, 170, 316, 90, 22);
    mk_ctl(d, L"BUTTON", L(LS_MAXBUILD), BS_AUTOCHECKBOX | WS_TABSTOP, 0x715, 26, 342, 140, 20);
    mk_ctl(d, L"EDIT", maxb_d.c_str(), WS_BORDER | WS_TABSTOP, 0x715e, 170, 340, 90, 22);
    mk_ctl(d, L"BUTTON", L(LS_USERDATA), BS_AUTOCHECKBOX | WS_TABSTOP, 0x716, 26, 366, 140, 20);
    mk_ctl(d, L"EDIT", L"", WS_BORDER | WS_TABSTOP, 0x716e, 170, 364, 90, 22);
    mk_ctl(d, L"BUTTON", L(LS_ADDLIC), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 260, 436, 96, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 362, 436, 88, 26);
    bool okout = false;
    EnableWindow(g_main, FALSE); ShowWindow(d, SW_SHOW); UpdateWindow(d);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_APP_MODAL_OK && (m.hwnd == d || IsChild(d, m.hwnd))) { okout = true; break; }
        if (m.message == WM_APP_MODAL_CANCEL && (m.hwnd == d || IsChild(d, m.hwnd))) break;
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        if (!IsDialogMessageW(d, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    EnableWindow(g_main, TRUE); SetActiveWindow(g_main);
    // 0.3.0 DUZELTME: alanlari YIKMADAN ONCE topla
    ProjLicense lc;
    if (okout) {
        ctl_txt(eName, &lc.name); ctl_txt(eMail, &lc.email); ctl_txt(eDate, &lc.date);
        ctl_txt(eOrder, &lc.order); ctl_txt(eComments, &lc.comments);
        lc.s_name = SendDlgItemMessageW(d, 0x710, BM_GETCHECK, 0, 0) == BST_CHECKED;
        lc.s_email = SendDlgItemMessageW(d, 0x711, BM_GETCHECK, 0, 0) == BST_CHECKED;
        lc.s_hwid = SendDlgItemMessageW(d, 0x712, BM_GETCHECK, 0, 0) == BST_CHECKED;
        lc.s_expire = SendDlgItemMessageW(d, 0x713, BM_GETCHECK, 0, 0) == BST_CHECKED;
        ctl_txt(GetDlgItem(d, 0x713e), &lc.s_expire_d);
        lc.s_time = SendDlgItemMessageW(d, 0x714, BM_GETCHECK, 0, 0) == BST_CHECKED;
        ctl_txt(GetDlgItem(d, 0x714e), &lc.s_time_v);
        lc.s_maxbuild = SendDlgItemMessageW(d, 0x715, BM_GETCHECK, 0, 0) == BST_CHECKED;
        ctl_txt(GetDlgItem(d, 0x715e), &lc.s_maxbuild_d);
        lc.s_userdata = SendDlgItemMessageW(d, 0x716, BM_GETCHECK, 0, 0) == BST_CHECKED;
        ctl_txt(GetDlgItem(d, 0x716e), &lc.s_userdata_v);
    }
    DestroyWindow(d);
    if (!okout) return false;
    g_proj.lics.push_back(lc);
    g_proj.modified = true;
    update_all();
    return true;
}

// ---------------- diyalog: Anahtar Cifti (export_key_pair_dialog.cc) ----------------
static void dlg_exportkeypair() {
    HWND d = dlg_base(L(LS_EXPORTKEY), 380, 190);
    mk_ctl(d, L"STATIC", L"Algorithm", 0, 0, 16, 22, 120, 18);
    HWND cbA = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0x801, 140, 18, 210, 120);
    ComboBox_AddString(cbA, L"RSA");
    ComboBox_SetCurSel(cbA, 0);
    mk_ctl(d, L"STATIC", L"Key Length", 0, 0, 16, 52, 120, 18);
    HWND cbL = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0x802, 140, 48, 210, 260);
    const wchar_t* lens[] = { L"1024", L"1536", L"2048", L"2560", L"3072", L"3584", L"4096" };
    for (int i = 0; i < 7; i++) ComboBox_AddString(cbL, lens[i]);
    ComboBox_SetCurSel(cbL, 2);   // VMProtect default 2048
    mk_ctl(d, L"BUTTON", L(LS_GENERATE), WS_TABSTOP, 0x803, 16, 92, 110, 26);
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 170, 128, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 266, 128, 88, 26);
    EnableWindow(g_main, FALSE); ShowWindow(d, SW_SHOW); UpdateWindow(d);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (m.message == WM_COMMAND && m.hwnd == d && LOWORD(m.wParam) == 0x803) {
            MessageBoxW(g_main, L(LS_NOTPORTED), XPROTECT_EDITION, MB_ICONINFORMATION);
            continue;
        }
        if (m.message == WM_APP_MODAL_OK || m.message == WM_APP_MODAL_CANCEL) break;
        if (m.message == WM_QUIT) { PostQuitMessage((int)m.wParam); break; }
        if (!IsDialogMessageW(d, &m)) { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    EnableWindow(g_main, TRUE); SetActiveWindow(g_main);
    DestroyWindow(d);
}

// ---------------- diyalog: Parametreler (execute_parameters_act_) ----------------
static void dlg_params() {
    HWND d = dlg_base(L(LS_PARAMS), 440, 160);
    mk_ctl(d, L"STATIC", L(LS_PARAMS), 0, 0, 16, 20, 120, 18);
    HWND e = mk_ctl(d, L"EDIT", g_proj.exec_params.c_str(), WS_BORDER | WS_TABSTOP, 0x901, 16, 42, 390, 22);
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 230, 84, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 326, 84, 88, 26);
    bool ok; modal_run(d, &ok);
    if (ok) ctl_txt(e, &g_proj.exec_params);   // 0.3.0: yikmadan once oku
    DestroyWindow(d);
}

// ---------------- diyalog: Sokum / Disassembly (0.3.0 — engine disasm) ----------------
// 0.3.0 ileri bildirimleri (Part D/E'de tanimlananlar)
static bool save_project_to(const std::wstring& path);
static bool load_project_from(const std::wstring& path, std::wstring* err);
static bool dlg_input_text(const wchar_t* title, const wchar_t* label, const std::wstring& init, std::wstring* out);
// VMProtect'in dump/disasm gorunumunun Win32 muadili: secili fonksiyonun
// talimat tablosu (adres | baytlar | talimat) motorun cozumleyicisinden.
static void dlg_disasm(const std::wstring& name, unsigned long long rva, unsigned long long size) {
    if (g_pe.raw.empty()) { MessageBoxW(g_main, L(LS_NO_TARGET), XPROTECT_EDITION, MB_ICONWARNING); return; }
    unsigned long long off = rva_to_fileoff(g_pe.secs, rva);
    if (!off || off >= g_pe.raw.size()) {
        MessageBoxW(g_main, (std::wstring(L"Address not in raw data: ") + numhex(rva)).c_str(), L(LS_ERROR), MB_ICONERROR);
        return;
    }
    if (!size) size = 0x100;
    if (size > 0x10000) size = 0x10000;
    u32 imgsz = 0;
    for (auto& s : g_pe.secs) { unsigned long long e = s.va + s.vsz; if (e > imgsz) imgsz = (u32)e; }
    HWND d = dlg_base((std::wstring(L(LS_DISASM)) + L": " + name).c_str(), 780, 500);
    HWND lv = mk_ctl(d, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_TABSTOP, 0xD01, 12, 12, 744, 400);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT);
    LVCOLUMNW c; memset(&c, 0, sizeof(c)); c.mask = LVCF_TEXT | LVCF_WIDTH;
    wchar_t* cols[3] = { (wchar_t*)L(LS_ADDRESS), (wchar_t*)L"Bytes", (wchar_t*)L"Instruction" };
    int wds[3] = { 110, 180, 430 };
    for (int i = 0; i < 3; i++) { c.pszText = cols[i]; c.cx = wds[i]; ListView_InsertColumn(lv, i, &c); }
    unsigned long long pc = rva;
    unsigned long long o = off;
    int rows = 0;
    while (pc < rva + size && o < g_pe.raw.size() && rows < 5000) {
        int maxlen = (int)std::min<unsigned long long>(32, g_pe.raw.size() - o);
        Insn in = disasm(&g_pe.raw[(size_t)o], maxlen, g_pe.is64, (u32)pc, g_pe.imagebase, imgsz);
        if (in.len < 1) in.len = 1;
        LVITEMW it; memset(&it, 0, sizeof(it));
        it.mask = LVIF_TEXT; it.iItem = rows;
        static wchar_t ab[64];
        _snwprintf(ab, 64, L"%08llX", pc);
        it.pszText = ab;
        int idx = ListView_InsertItem(lv, &it);
        static wchar_t bb[128];
        std::string hx = hexs(&g_pe.raw[(size_t)o], in.len > 16 ? 16 : in.len);
        if (in.len > 16) hx += "...";
        size_t w = 0;
        for (; w < hx.size() && w < 126; w++) bb[w] = (unsigned char)hx[w];
        bb[w] = 0;
        ListView_SetItemText(lv, idx, 1, bb);
        std::wstring txt = s2w(in.text);
        static wchar_t ib[256];
        size_t k = 0;
        for (; k < txt.size() && k < 250; k++) ib[k] = txt[k];
        ib[k] = 0;
        ListView_SetItemText(lv, idx, 2, ib);
        pc += in.len;
        o += in.len;
        rows++;
    }
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 330, 424, 90, 26);
    bool okb; modal_run(d, &okb);
    DestroyWindow(d);
}
// secili fonksiyondan sokum diyalogu ac
static void open_disasm_for_selection() {
    if (!g_proj.open) { MessageBoxW(g_main, L(LS_NO_TARGET), XPROTECT_EDITION, MB_ICONWARNING); return; }
    // 1) ozellik izgarasindan secili proje fonksiyonu
    if (g_prop_sel_kind == 2 && g_prop_sel_idx >= 0 && (size_t)g_prop_sel_idx < g_proj.funcs.size()) {
        const ProjFunc& f = g_proj.funcs[g_prop_sel_idx];
        dlg_disasm(f.name, f.rva, f.size);
        return;
    }
    // 2) Fonksiyonlar sekmesinden secili satir
    if (g_tab == 1 && g_func) {
        int sel = ListView_GetNextItem(g_func, -1, LVNI_SELECTED);
        if (sel >= 0) {
            LVITEMW it; memset(&it, 0, sizeof(it));
            it.mask = LVIF_PARAM; it.iItem = sel;
            if (ListView_GetItem(g_func, &it) && (size_t)it.lParam < g_pe.funcs.size()) {
                const PeFunc& f = g_pe.funcs[it.lParam];
                dlg_disasm(f.name, f.rva, f.size);
                return;
            }
        }
    }
    // 3) adres sor
    std::wstring v;
    if (dlg_input_text(L(LS_DISASM), L(LS_ADDRESS), L"", &v)) {
        unsigned long long a = wcstoull(v.c_str(), NULL, 16);
        if (a) dlg_disasm(sym_by_addr(a), a, 0);
    }
}

// ---------------- diyalog: Yardim Icerigi (0.3.0) ----------------
static void dlg_help() {
    HWND d = dlg_base(L(LS_CONTENTS), 560, 420);
    const wchar_t* txt = g_tr
        ? L"XProtect 0.3.0 — hizli kilavuz\n\n"
          L"1. Dosya > Ac (Ctrl+O) ile hedef .exe/.dll seil.\n"
          L"2. Fonksiyon listesi otomatik dolar: pdb/.map varsa fonksiyon RVA\n"
          L"   listesi onlardan okunur; hicbiri yoksa stub adaylar listelenir\n"
          L"   (Kaynak kolonu: Export / COFF / PDB / MAP / Stub).\n"
          L"3. Fonksiyon Ekle (Ins) ile listeden ya da elle adres gir.\n"
          L"4. Derleme Turu: Sanallastirma / Mutasyon / Ultra (M+V).\n"
          L"5. Derle (F9) — korumali <ad>_vmp.exe ayni klasore uretilir.\n"
          L"6. Calistir (F5) — orijinal/korumaliyi parametrelerle acar.\n\n"
          L"Araclar > Sokum: secili fonksiyonun talimat tablosu.\n"
          L"Ayarlar: dil (EN/TR) ve otomatik kaydetme.\n"
          L"Proje .xproj, sablonlar .xtpl olarak kaydedilir."
        : L"XProtect 0.3.0 — quick guide\n\n"
          L"1. File > Open (Ctrl+O) to select the target .exe/.dll.\n"
          L"2. The function list fills automatically: if a .pdb/.map exists the\n"
          L"   function RVA list is read from it; otherwise stub candidates\n"
          L"   are listed (Source column: Export / COFF / PDB / MAP / Stub).\n"
          L"3. Add Function (Ins) from the list or type an address.\n"
          L"4. Compilation Type: Virtualization / Mutation / Ultra (M+V).\n"
          L"5. Compile (F9) — protected <name>_vmp.exe is created next to it.\n"
          L"6. Execute (F5) — run original/protected with parameters.\n\n"
          L"Tools > Disassembly: instruction table of the selected function.\n"
          L"Settings: language (EN/TR) and auto-save.\n"
          L"Projects save as .xproj, templates as .xtpl.";
    HWND lb = mk_ctl(d, L"EDIT", txt, WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 0xE01, 12, 12, 524, 330);
    (void)lb;
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 230, 356, 90, 26);
    bool okb; modal_run(d, &okb);
    DestroyWindow(d);
}

// ---------------- sablonlar (0.3.0 — gercek .xtpl kayit/yukleme) ----------------
static std::vector<std::wstring> list_templates() {
    std::vector<std::wstring> out;
    std::wstring mask = exe_dir() + L"\\*.xtpl";
    WIN32_FIND_DATAW fd; memset(&fd, 0, sizeof(fd));
    HANDLE h = FindFirstFileW(mask.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                out.push_back(exe_dir() + L"\\" + fd.cFileName);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return out;
}
static bool save_template_to(const std::wstring& path) {
    // sablon = hedefi bos birakilmis proje (options + funcs + wms + lics)
    Project tmp = g_proj;
    g_proj.target.clear();
    g_proj.path.clear();
    bool ok = save_project_to(path);
    g_proj = tmp;
    return ok;
}
static bool load_template_from(const std::wstring& path, std::wstring* err) {
    Project save = g_proj;
    if (!load_project_from(path, err)) { g_proj = save; return false; }
    Project tpl = g_proj;
    g_proj = save;                    // hedef/proje yolu korunur
    g_proj.options = tpl.options;
    g_proj.funcs = tpl.funcs;
    g_proj.wms = tpl.wms;
    g_proj.lics = tpl.lics;
    g_proj.modified = true;
    return true;
}
static void save_template_as() {
    std::wstring p;
    if (!pick_file(g_main, true, L"xtpl", &p)) return;
    if (save_template_to(p)) {
        add_log(L"Template saved: " + p);
        g_proj.modified = true;
    } else {
        MessageBoxW(g_main, L"Cannot save template", L(LS_ERROR), MB_ICONERROR);
    }
}

// ---------------- derleme: motoru in-process kos (cplport_gui mimarisi) ----------------
struct CompileJob { std::string in, out; Options opt; };
static DWORD WINAPI compile_thread(LPVOID p) {
    CompileJob* j = (CompileJob*)p;
    std::vector<std::string> log;
    int rc = protect(j->in, j->out, j->opt, log);
    for (size_t i = 0; i < log.size(); i++) post_log_line(log[i]);
    PostMessageW(g_main, WM_APP_DONE, (WPARAM)rc, 0);
    delete j;
    return 0;
}
static void start_compile() {
    if (!g_proj.open || g_compiling) return;
    if (g_proj.funcs.empty()) {
        int r = MessageBoxW(g_main, L"No functions marked. Compile ALL suitable functions?", L(LS_QUESTION), MB_YESNO | MB_ICONQUESTION);
        if (r != IDYES) return;
    }
    CompileJob* j = new CompileJob();
    j->in = w2s(g_proj.target);
    std::wstring out = protected_path();
    j->out = w2s(out);
    Options opt;
    opt.level = 1;
    bool any_mut = false;
    for (auto& f : g_proj.funcs) {
        if (f.excluded) continue;
        if (f.type != 0) any_mut = true;
        // 0.3.0: motor isim tabani export/COFF'tur; PDB/MAP/Stub kaynakli
        // fonksiyonlar RVA-hex olarak iletilir (motor RVA'dan da secer)
        bool engine_knows_name = (f.src == SYM_EXPORT || f.src == SYM_COFF);
        std::string key;
        if (!f.name.empty() && engine_knows_name) key = w2s(f.name);
        else {
            wchar_t b[32]; _snwprintf(b, 32, L"%llX", f.rva);
            key = w2s(b);
        }
        opt.funcs.push_back(key);
        // 0.3.0: derleme tipi (0=V Sanallastirma, 1=M Mutasyon, 2=U Ultra)
        // fonksiyon bazinda motora aktarilir — VMProtect davranisi
        opt.ftypes[key] = f.type;
    }
    if (g_proj.funcs.empty() || opt.funcs.empty()) opt.all = true;
    opt.antidebug = (g_proj.options & cpCheckDebugger) != 0;
    opt.strenc = (g_proj.options & cpCryptValues) != 0;
    opt.memprot = (g_proj.options & cpMemoryProtection) != 0;
    opt.vmenc = (g_proj.options & cpEncryptBytecode) == 0;   // vmenc varsayilan ACIK
    // 0.3.0: level artik ftypes oncelikli; "tumunu derle" yolu 0.2.0 uyumu
    // icin level=2 birakilir (ftypes bosken VM junk aktif)
    if (opt.all) opt.level = 2;
    (void)any_mut;
    j->opt = opt;
    add_log(std::wstring(L(LS_COMPILING)) + L"  ->  " + basename_of(out));
    g_compiling = true;
    if (g_cancel_event) CloseHandle(g_cancel_event);
    g_cancel_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE th = CreateThread(NULL, 0, compile_thread, j, 0, NULL);
    if (th) CloseHandle(th);
    build_menu();   // compile disabled iken
}

// ---------------- proje kaydet / yukle (.xproj) ----------------
static std::wstring esc_t(const std::wstring& s) {
    std::wstring o;
    for (wchar_t c : s) {
        if (c == L'\\') o += L"\\\\";
        else if (c == L'\t') o += L"\\t";
        else if (c == L'\n') o += L"\\n";
        else o += c;
    }
    return o;
}
static std::wstring unesc_t(const std::wstring& s) {
    std::wstring o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\\' && i + 1 < s.size()) {
            wchar_t n = s[++i];
            if (n == L't') o += L'\t';
            else if (n == L'n') o += L'\n';
            else if (n == L'\\') o += L'\\';
            else o += n;
        } else o += s[i];
    }
    return o;
}
static std::vector<std::wstring> split_t(const std::wstring& s) {
    std::vector<std::wstring> out; std::wstring cur;
    for (wchar_t c : s) { if (c == L'\t') { out.push_back(cur); cur.clear(); } else cur += c; }
    out.push_back(cur);
    return out;
}
static bool save_project_to(const std::wstring& path) {
    FILE* f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    std::string o = "XPROJ1\n";
    auto ln = [&](const std::string& s) { o += s + "\n"; };
    ln("target=" + w2s(g_proj.target));
    { wchar_t b[32]; _snwprintf(b, 32, L"%016llX", g_proj.options); ln("options=" + w2s(b)); }
    ln("execparams=" + w2s(esc_t(g_proj.exec_params)));
    for (auto& fn : g_proj.funcs) {
        std::wstring line = L"func\t" + esc_t(fn.name) + L"\t" + numdec(fn.rva) + L"\t" +
            numdec(fn.size) + L"\t" + (fn.is_export ? L"1" : L"0") + L"\t" +
            numdec((unsigned long long)fn.type) + L"\t" + (fn.excluded ? L"1" : L"0") + L"\t" +
            (fn.lock_to_key ? L"1" : L"0") + L"\t" + numdec(fn.ext) + L"\t" + numdec(fn.brk);
        ln(w2s(line));
    }
    for (auto& w : g_proj.wms)
        ln("wm\t" + w2s(esc_t(w.name)) + "\t" + w2s(esc_t(w.value)));
    for (auto& lc : g_proj.lics) {
        wchar_t fl[8]; _snwprintf(fl, 8, L"%d%d%d%d%d%d%d",
            lc.s_name, lc.s_email, lc.s_hwid, lc.s_expire, lc.s_time, lc.s_maxbuild, lc.s_userdata);
        std::wstring line = L"lic\t" + esc_t(lc.name) + L"\t" + esc_t(lc.email) + L"\t" + esc_t(lc.date) +
            L"\t" + esc_t(lc.order) + L"\t" + esc_t(lc.comments) + L"\t" + fl + L"\t" +
            esc_t(lc.s_expire_d) + L"\t" + esc_t(lc.s_maxbuild_d) + L"\t" + esc_t(lc.s_time_v) + L"\t" + esc_t(lc.s_userdata_v);
        ln(w2s(line));
    }
    fwrite(o.data(), 1, o.size(), f);
    fclose(f);
    return true;
}
static bool load_project_from(const std::wstring& path, std::wstring* err) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) { *err = L"Cannot open project"; return false; }
    std::string all;
    char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) all.append(buf, n);
    fclose(f);
    // satirlari char-domain'de ayristir; alanlar UTF-8 -> s2w -> unesc_t
    std::vector<std::string> lines;
    {
        std::string cur;
        for (size_t i = 0; i <= all.size(); i++) {
            char ch = i < all.size() ? all[i] : '\n';
            if (ch == '\n') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
            else cur += ch;
        }
    }
    g_proj = Project();
    bool first = true;
    for (size_t li = 0; li < lines.size(); li++) {
        const std::string& line = lines[li];
        if (first) {
            if (line != "XPROJ1") { *err = L"Not an XProtect project file"; return false; }
            first = false; continue;
        }
        auto sfield = [&](size_t idx) -> std::wstring {   // \t ile ayrilmis alan (char-domain)
            size_t start = 0, c = 0;
            for (size_t i = 0; i <= line.size(); i++) {
                if (i == line.size() || line[i] == '\t') {
                    if (c == idx) return s2w(line.substr(start, i - start));
                    c++; start = i + 1;
                }
            }
            return L"";
        };
        if (line.rfind("target=", 0) == 0) {
            g_proj.target = s2w(line.substr(7));
        } else if (line.rfind("options=", 0) == 0) {
            g_proj.options = wcstoull(s2w(line.substr(8)).c_str(), NULL, 16);
        } else if (line.rfind("execparams=", 0) == 0) {
            g_proj.exec_params = unesc_t(s2w(line.substr(11)));
        } else if (line.rfind("func\t", 0) == 0) {
            std::wstring body = line.size() > 5 ? s2w(line.substr(5)) : L"";
            auto p = split_t(body);
            if (p.size() >= 9) {
                ProjFunc fn;
                fn.name = unesc_t(p[0]);
                fn.rva = wcstoull(p[1].c_str(), NULL, 10);
                fn.size = wcstoull(p[2].c_str(), NULL, 10);
                fn.is_export = p[3] == L"1";
                fn.type = (int)wcstoul(p[4].c_str(), NULL, 10);
                fn.excluded = p[5] == L"1";
                fn.lock_to_key = p[6] == L"1";
                fn.ext = wcstoull(p[7].c_str(), NULL, 10);
                fn.brk = wcstoull(p[8].c_str(), NULL, 10);
                g_proj.funcs.push_back(fn);
            }
        } else if (line.rfind("wm\t", 0) == 0) {
            std::wstring body = line.size() > 3 ? s2w(line.substr(3)) : L"";
            auto p = split_t(body);
            if (p.size() >= 2) { ProjWatermark w; w.name = unesc_t(p[0]); w.value = unesc_t(p[1]); g_proj.wms.push_back(w); }
        } else if (line.rfind("lic\t", 0) == 0) {
            std::wstring body = line.size() > 4 ? s2w(line.substr(4)) : L"";
            auto p = split_t(body);
            if (p.size() >= 10) {
                ProjLicense lc;
                lc.name = unesc_t(p[0]); lc.email = unesc_t(p[1]); lc.date = unesc_t(p[2]);
                lc.order = unesc_t(p[3]); lc.comments = unesc_t(p[4]);
                const std::wstring& fl = p[5];
                lc.s_name = fl.size() > 0 && fl[0] == L'1';
                lc.s_email = fl.size() > 1 && fl[1] == L'1';
                lc.s_hwid = fl.size() > 2 && fl[2] == L'1';
                lc.s_expire = fl.size() > 3 && fl[3] == L'1';
                lc.s_time = fl.size() > 4 && fl[4] == L'1';
                lc.s_maxbuild = fl.size() > 5 && fl[5] == L'1';
                lc.s_userdata = fl.size() > 6 && fl[6] == L'1';
                lc.s_expire_d = unesc_t(p[6]); lc.s_maxbuild_d = unesc_t(p[7]);
                lc.s_time_v = unesc_t(p[8]); lc.s_userdata_v = unesc_t(p[9]);
                g_proj.lics.push_back(lc);
            }
        }
    }
    return true;
}
static void save_project(bool save_as) {
    if (!g_proj.open) return;
    std::wstring p = g_proj.path;
    if (p.empty() || save_as) {
        if (!pick_file(g_main, true, L"xproj", &p)) return;
    }
    if (save_project_to(p)) {
        g_proj.path = p; g_proj.modified = false;
        add_log(L"Project saved: " + p);
        build_menu();
    } else {
        MessageBoxW(g_main, L"Cannot save project", L(LS_ERROR), MB_ICONERROR);
    }
}
static void open_target(const std::wstring& path) {
    PeInfo pi; std::wstring err;
    if (!parse_pe_file(path, pi, err)) {
        MessageBoxW(g_main, (std::wstring(L"Cannot open file: ") + err).c_str(), L(LS_ERROR), MB_ICONERROR);
        return;
    }
    g_pe = pi;
    g_proj = Project();
    g_proj.open = true;
    g_proj.target = path;
    g_prop_sel_kind = 0; g_prop_sel_idx = -1;
    push_recent(path);
    save_settings();
    build_menu();          // 0.3.0: recent listesi degisti
    update_all();
    add_log(L"Opened: " + path + (pi.is64 ? L"  (x64)" : L"  (x86)"));
    if (!pi.sym_note.empty()) add_log(pi.sym_note);
    if (!pi.funcs.empty())
        add_log(std::wstring(L"Functions: ") + numdec(pi.funcs.size()) + L"  (" + src_name(pi.funcs[0].source) + L"...)");
}
static void open_project_file(const std::wstring& path) {
    std::wstring err;
    if (!load_project_from(path, &err)) {
        MessageBoxW(g_main, err.c_str(), L(LS_ERROR), MB_ICONERROR);
        return;
    }
    if (!g_proj.target.empty()) {
        PeInfo pi; std::wstring perr;
        if (parse_pe_file(g_proj.target, pi, perr)) g_pe = pi;
        else { MessageBoxW(g_main, (std::wstring(L"Target missing: ") + g_proj.target).c_str(), L(LS_ERROR), MB_ICONWARNING); }
    }
    g_proj.open = true;
    g_proj.path = path;
    g_prop_sel_kind = 0; g_prop_sel_idx = -1;
    update_all();
    add_log(L"Project opened: " + path);
}
static void close_project() {
    if (g_proj.modified) {
        int r = MessageBoxW(g_main, L"Save changes to project?", L(LS_QUESTION), MB_YESNOCANCEL | MB_ICONQUESTION);
        if (r == IDCANCEL) return;
        if (r == IDYES) save_project(false);
    }
    g_proj = Project();
    g_pe = PeInfo();
    g_undo.clear(); g_redo.clear();
    g_prop_sel_kind = -1; g_prop_sel_idx = -1;
    update_all();
}
// ============================================================
// xprotect_ui.cpp — Part E: layout, WndProc, komutlar, WinMain
// ============================================================

#define IDM_OPENPROJ  0x9002
#define IDM_RENAME    0x9003
#define IDM_EXCLUDE   0x9004
#define IDM_EXTADDR   0x9005
#define IDM_BRKADDR   0x9006
#define IDM_CTX_DEL   0x9007
#define IDM_TYPE_V    0x9010
#define IDM_TYPE_M    0x9011
#define IDM_TYPE_U    0x9012

static void update_caption() {
    std::wstring cap = XPROTECT_EDITION L" [unregistered]";
    if (g_proj.open) cap += (g_proj.path.empty() ? g_proj.target : g_proj.path);
    SetWindowTextW(g_main, cap.c_str());
}

static void prop_cols_setup() {
    while (ListView_DeleteColumn(g_prop, 0)) {}
    LVCOLUMNW c; memset(&c, 0, sizeof(c));
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)L"Property"; c.cx = 230; ListView_InsertColumn(g_prop, 0, &c);
    c.pszText = (LPWSTR)L(LS_VALUE); c.cx = 240; ListView_InsertColumn(g_prop, 1, &c);
}
static void log_cols_setup() {
    while (ListView_DeleteColumn(g_log, 0)) {}
    LVCOLUMNW c; memset(&c, 0, sizeof(c));
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)L"Time"; c.cx = 70; ListView_InsertColumn(g_log, 0, &c);
    c.pszText = (LPWSTR)L"Message"; c.cx = 700; ListView_InsertColumn(g_log, 1, &c);
}

static void update_all() {
    // 0.3.0: menu/toolbar artik yeniden KURULMAZ — durumlar guncellenir
    // (0.2.0'daki tum-secim-degistikce-menü-flicker / toolbar-sifirlanma bitti)
    update_toolbar_states();
    tree_fill();
    if (g_tab == 1) func_fill();
    else if (g_tab == 2) details_fill_rows();
    fill_prop();
    update_caption();
    ShowWindow(g_welcome, g_proj.open ? SW_HIDE : SW_SHOW);
    std::wstring t = g_proj.open ? (g_proj.path.empty() ? g_proj.target : g_proj.path) : L(LS_READY);
    if (g_proj.open && !g_pe.sym_note.empty()) t += std::wstring(L"  |  ") + g_pe.sym_note;
    SendMessageW(g_status, SB_SETTEXTW, 0, (LPARAM)t.c_str());
    SendMessageW(g_status, SB_SETTEXTW, 3, (LPARAM)(g_tr ? L" TR " : L" EN "));
    welcome_fill();
}

// ---------------- generic metin girisi ----------------
static bool dlg_input_text(const wchar_t* title, const wchar_t* label, const std::wstring& init, std::wstring* out) {
    HWND d = dlg_base(title, 400, 150);
    mk_ctl(d, L"STATIC", label, 0, 0, 16, 20, 340, 18);
    HWND e = mk_ctl(d, L"EDIT", init.c_str(), WS_BORDER | WS_TABSTOP, 0xB01, 16, 44, 350, 22);
    mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 200, 84, 90, 26);
    mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 296, 84, 88, 26);
    SetFocus(e);
    bool ok; modal_run(d, &ok);
    if (ok) ctl_txt(e, out);   // 0.3.0: yikmadan once oku
    DestroyWindow(d);
    return ok;
}

// ---------------- layout ----------------
static void layout() {
    if (!g_main) return;
    RECT rc; GetClientRect(g_main, &rc);
    int W = rc.right, H = rc.bottom;
    SendMessageW(g_status, WM_SIZE, 0, 0);
    RECT sr; GetWindowRect(g_status, &sr);
    int st_h = sr.bottom - sr.top;
    // 0.3.0: dar pencerede negatif bolumler klamp'
    if (W >= 560) {
        int parts[4];
        parts[0] = W - 360; parts[1] = W - 200; parts[2] = W - 90; parts[3] = -1;
        SendMessageW(g_status, SB_SETPARTS, 4, (LPARAM)parts);
    }
    SendMessageW(g_toolbar, TB_AUTOSIZE, 0, 0);
    RECT tr; GetWindowRect(g_toolbar, &tr);
    int tb_h = tr.bottom - tr.top;
    // 0.3.0 DUZELTME (UI bug #4): gercek toolbar yuksekligi kullanilir;
    // 44'e kirpma YOK (kirpilinca cok satirli toolbar alttaki panelleri ortuyordu)
    if (tb_h < 26) tb_h = 26;
    if (tb_h > 120) tb_h = 120;
    MoveWindow(g_toolbar, 0, 0, W, tb_h, TRUE);
    int top = tb_h;
    int body_h = H - top - g_log_h - st_h;
    if (body_h < 100) body_h = 100;
    MoveWindow(g_tabstrip, 0, top, 30, body_h, TRUE);
    // 0.3.0: filtre seridi — Fonksiyonlar sekmesinin ust seridi (toolbar'a mudahale etmez)
    int list_top = top;
    int list_h = body_h;
    if (g_tab == 1 && g_filter) {
        MoveWindow(g_filter, 30, top, g_split_x - 4, 24, TRUE);
        list_top = top + 26;
        list_h = body_h - 26;
    }
    MoveWindow(g_tree, 30, top, g_split_x, body_h, TRUE);
    MoveWindow(g_func, 30, list_top, g_split_x, list_h, TRUE);
    if (g_filter) ShowWindow(g_filter, g_tab == 1 ? SW_SHOW : SW_HIDE);
    MoveWindow(g_split, 30 + g_split_x, top, 4, body_h, TRUE);
    MoveWindow(g_prop, 34 + g_split_x, top, W - 34 - g_split_x, body_h, TRUE);
    MoveWindow(g_log, 0, H - st_h - g_log_h, W, g_log_h, TRUE);
    MoveWindow(g_welcome, 0, top, W, body_h + g_log_h, TRUE);
    MoveWindow(GetDlgItem(g_welcome, 0xA01), 40, 60, W - 80, 44, TRUE);
    MoveWindow(GetDlgItem(g_welcome, 0xA02), 40, 130, 160, 32, TRUE);
    MoveWindow(GetDlgItem(g_welcome, 0xA03), 210, 130, 160, 32, TRUE);
    MoveWindow(GetDlgItem(g_welcome, 0xA04), 40, 180, W - 80, body_h - 200 > 60 ? body_h - 200 : 60, TRUE);
    ShowWindow(g_welcome, g_proj.open ? SW_HIDE : SW_SHOW);
}

// ---------------- acilir menuler ----------------
static int track_menu(HMENU m) {
    POINT p; GetCursorPos(&p);
    SetForegroundWindow(g_main);
    int cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0, g_main, NULL);
    PostMessageW(g_main, WM_NULL, 0, 0);
    DestroyMenu(m);
    return cmd;
}
static void exec_dropdown() {
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_EXEC_ORIG, L(LS_EXEC_ORIG));
    AppendMenuW(m, MF_STRING, IDM_EXEC_PROT, L(LS_EXEC_PROT));
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_EXEC_PARAMS, L(LS_EXEC_PARAMS));
    int cmd = track_menu(m);
    switch (cmd) {
    case IDM_EXEC_ORIG: {
        std::wstring dir = exe_dir();
        ShellExecuteW(g_main, L"open", g_proj.target.c_str(), g_proj.exec_params.c_str(), dir.c_str(), SW_SHOWNORMAL);
        break;
    }
    case IDM_EXEC_PROT: {
        std::wstring pp = protected_path();
        if (GetFileAttributesW(pp.c_str()) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(g_main, (std::wstring(L"Protected file not found. Compile first.\n") + pp).c_str(), L(LS_ERROR), MB_ICONWARNING);
            break;
        }
        std::wstring dir = exe_dir();
        ShellExecuteW(g_main, L"open", pp.c_str(), g_proj.exec_params.c_str(), dir.c_str(), SW_SHOWNORMAL);
        break;
    }
    case IDM_EXEC_PARAMS: dlg_params(); break;
    }
}
static void templates_dropdown() {
    // 0.3.0: GERCEK sablon sistemi — .xtpl listele + yukle + farkli kaydet
    HMENU m = CreatePopupMenu();
    std::vector<std::wstring> tpls = list_templates();
    for (size_t i = 0; i < tpls.size() && i < 16; i++)
        AppendMenuW(m, MF_STRING, IDM_TPL_BASE + (int)i, basename_of(tpls[i]).c_str());
    if (!tpls.empty()) AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, IDM_TPL_SAVE, L(LS_SAVETPL));
    int cmd = track_menu(m);
    if (cmd == IDM_TPL_SAVE) { save_template_as(); return; }
    if (cmd >= IDM_TPL_BASE && cmd < IDM_TPL_BASE + (int)tpls.size()) {
        std::wstring p = tpls[cmd - IDM_TPL_BASE], err;
        if (!g_proj.open) { MessageBoxW(g_main, L(LS_NO_TARGET), XPROTECT_EDITION, MB_ICONWARNING); return; }
        if (load_template_from(p, &err)) {
            add_log(L"Template applied: " + basename_of(p));
            update_all();
        } else {
            MessageBoxW(g_main, err.c_str(), L(LS_ERROR), MB_ICONERROR);
        }
    }
}
static void tree_context(HTREEITEM hi) {
    if (!g_proj.open) return;
    TVITEMW it; memset(&it, 0, sizeof(it));
    it.mask = TVIF_PARAM | TVIF_HANDLE; it.hItem = hi;
    if (!TreeView_GetItem(g_tree, &it)) return;
    long id = (long)it.lParam;
    HMENU m = CreatePopupMenu();
    int cmd = 0;
    if (id == 0) {
        AppendMenuW(m, MF_STRING, IDM_ADDFUNC, L(LS_ADDFUNC));
        AppendMenuW(m, MF_STRING, IDM_ADDLIC, L(LS_ADDLIC));
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m, MF_STRING, IDM_COMPILE, L(LS_COMPILE));
        AppendMenuW(m, MF_STRING, IDM_EXECUTE, L(LS_EXECUTE));
        cmd = track_menu(m);
        if (cmd == IDM_ADDFUNC) dlg_addfunction(false, 0);
        else if (cmd == IDM_ADDLIC) dlg_addlicense();
        else if (cmd == IDM_COMPILE) start_compile();
        else if (cmd == IDM_EXECUTE) exec_dropdown();
        return;
    }
    if (id == 1) {
        AppendMenuW(m, MF_STRING, IDM_ADDFUNC, L(LS_ADDFUNC));
        cmd = track_menu(m);
        if (cmd == IDM_ADDFUNC) dlg_addfunction(false, 0);
        return;
    }
    if (id == 2) {
        AppendMenuW(m, MF_STRING, IDM_ADDLIC, L(LS_ADDLIC));
        cmd = track_menu(m);
        if (cmd == IDM_ADDLIC) dlg_addlicense();
        return;
    }
    if (id >= 100 && id < 200) {
        int idx = id - 100;
        AppendMenuW(m, MF_STRING, IDM_RENAME, L"Rename\tF2");
        AppendMenuW(m, MF_STRING, IDM_CTX_DEL, (std::wstring(L(LS_DELETE)) + L"\tDel").c_str());
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m, MF_STRING | (g_proj.funcs[idx].excluded ? MF_CHECKED : 0), IDM_EXCLUDE, L(LS_EXCLUDED));
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        HMENU mt = CreatePopupMenu();
        AppendMenuW(mt, MF_STRING | (g_proj.funcs[idx].type == 0 ? MF_CHECKED : 0), IDM_TYPE_V, L(LS_VIRT));
        AppendMenuW(mt, MF_STRING | (g_proj.funcs[idx].type == 1 ? MF_CHECKED : 0), IDM_TYPE_M, L(LS_MUT));
        AppendMenuW(mt, MF_STRING | (g_proj.funcs[idx].type == 2 ? MF_CHECKED : 0), IDM_TYPE_U, L(LS_ULTRA));
        AppendMenuW(m, MF_POPUP, (UINT_PTR)mt, L(LS_COMPTYPE));
        AppendMenuW(m, MF_STRING, IDM_EXTADDR, L(LS_EXTADDR));
        AppendMenuW(m, MF_STRING, IDM_BRKADDR, L(LS_BREAKADDR));
        AppendMenuW(m, MF_SEPARATOR, 0, NULL);
        AppendMenuW(m, MF_STRING, IDM_DISASM, L(LS_DISASM));
        cmd = track_menu(m);
        switch (cmd) {
        case IDM_RENAME: {
            std::wstring v;
            if (dlg_input_text(L"Rename", L(LS_NAME), g_proj.funcs[idx].name, &v)) {
                push_undo(); g_proj.funcs[idx].name = v; g_proj.modified = true; update_all();
            }
            break;
        }
        case IDM_CTX_DEL:
            push_undo(); g_proj.funcs.erase(g_proj.funcs.begin() + idx);
            g_prop_sel_kind = -1; g_proj.modified = true; update_all();
            break;
        case IDM_EXCLUDE:
            g_proj.funcs[idx].excluded = !g_proj.funcs[idx].excluded;
            g_proj.modified = true; update_all();
            break;
        case IDM_TYPE_V: g_proj.funcs[idx].type = 0; g_proj.modified = true; update_all(); break;
        case IDM_TYPE_M: g_proj.funcs[idx].type = 1; g_proj.modified = true; update_all(); break;
        case IDM_TYPE_U: g_proj.funcs[idx].type = 2; g_proj.modified = true; update_all(); break;
        case IDM_EXTADDR: {
            std::wstring v;
            std::wstring init = g_proj.funcs[idx].ext ? numhex(g_proj.funcs[idx].ext) : L"";
            if (dlg_input_text(L(LS_EXTADDR), L(LS_ADDRESS), init, &v)) {
                g_proj.funcs[idx].ext = wcstoull(v.c_str(), NULL, 16);
                g_proj.modified = true; update_all();
            }
            break;
        }
        case IDM_DISASM:
            dlg_disasm(g_proj.funcs[idx].name, g_proj.funcs[idx].rva, g_proj.funcs[idx].size);
            break;
        case IDM_BRKADDR: {
            std::wstring v;
            std::wstring init = g_proj.funcs[idx].brk ? numhex(g_proj.funcs[idx].brk) : L"";
            if (dlg_input_text(L(LS_BREAKADDR), L(LS_ADDRESS), init, &v)) {
                g_proj.funcs[idx].brk = wcstoull(v.c_str(), NULL, 16);
                g_proj.modified = true; update_all();
            }
            break;
        }
        }
        return;
    }
    if (id >= 200 && id < 300) {
        int idx = id - 200;
        AppendMenuW(m, MF_STRING, IDM_CTX_DEL, L(LS_DELETE));
        cmd = track_menu(m);
        if (cmd == IDM_CTX_DEL) {
            g_proj.lics.erase(g_proj.lics.begin() + idx);
            g_prop_sel_kind = -1; g_proj.modified = true; update_all();
        }
        return;
    }
    DestroyMenu(m);
}

// ---------------- ozellik izgari duzenleme ----------------
static void prop_row_edit(int row) {
    if (row < 0 || (size_t)row >= g_rows.size()) return;
    PropRow& r = g_rows[row];
    if (r.kind == 0 || r.kind == 4) return;
    if (r.kind == 1) {
        if (g_prop_sel_kind == 0) {
            g_proj.options ^= r.id;
            g_proj.modified = true;
        } else if (g_prop_sel_kind == 2) {
            int idx = (int)r.id;
            if (r.name == std::wstring(L(LS_LOCKKEY))) g_proj.funcs[idx].lock_to_key = !g_proj.funcs[idx].lock_to_key;
            if (r.name == std::wstring(L(LS_EXCLUDED))) g_proj.funcs[idx].excluded = !g_proj.funcs[idx].excluded;
            g_proj.modified = true;
        } else if (g_prop_sel_kind == 4) {
            int idx = g_prop_sel_idx;
            unsigned long long fld = r.id;
            ProjLicense& lc = g_proj.lics[idx];
            int base = 100 + idx * 10;
            (void)base;
            unsigned long long off = fld - (unsigned long long)(100 + idx * 10);
            switch (off) {
            case 5: lc.s_name = !lc.s_name; break;
            case 6: lc.s_email = !lc.s_email; break;
            case 7: lc.s_hwid = !lc.s_hwid; break;
            case 8: lc.s_expire = !lc.s_expire; break;
            case 9: lc.s_time = !lc.s_time; break;
            case 10: lc.s_maxbuild = !lc.s_maxbuild; break;
            case 11: lc.s_userdata = !lc.s_userdata; break;
            }
            g_proj.modified = true;
        }
        fill_prop(); tree_fill(); build_menu();
        return;
    }
    if (r.kind == 2) {
        int idx = (int)r.id;
        // derleme turu combo diyalogu
        HWND d = dlg_base(L(LS_COMPTYPE), 340, 140);
        HWND cb = mk_ctl(d, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 0xC01, 16, 20, 300, 120);
        ComboBox_AddString(cb, L(LS_VIRT));
        ComboBox_AddString(cb, L(LS_MUT));
        ComboBox_AddString(cb, (std::wstring(L(LS_ULTRA)) + L" (M+V)").c_str());
        ComboBox_SetCurSel(cb, g_proj.funcs[idx].type);
        mk_ctl(d, L"BUTTON", L(LS_OK), BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, 130, 76, 90, 26);
        mk_ctl(d, L"BUTTON", L(LS_CANCEL), WS_TABSTOP, IDCANCEL, 226, 76, 88, 26);
        bool ok; modal_run(d, &ok);
        int sel = ok ? ComboBox_GetCurSel(cb) : g_proj.funcs[idx].type;   // 0.3.0: yikmadan once oku
        DestroyWindow(d);
        if (ok) {
            push_undo();
            g_proj.funcs[idx].type = sel;
            g_proj.modified = true; update_all();
        }
        return;
    }
    if (r.kind == 3) {
        if (g_prop_sel_kind == 2) {
            int idx = (int)r.id;
            std::wstring v;
            std::wstring cur = r.value == L"(auto)" ? L"" : r.value;
            if (dlg_input_text(r.name, L(LS_ADDRESS), cur, &v)) {
                unsigned long long a = v.empty() ? 0 : wcstoull(v.c_str(), NULL, 16);
                if (r.name == std::wstring(L(LS_EXTADDR))) g_proj.funcs[idx].ext = a;
                else g_proj.funcs[idx].brk = a;
                g_proj.modified = true; update_all();
            }
        } else if (g_prop_sel_kind == 4) {
            int idx = g_prop_sel_idx;
            unsigned long long off = r.id - (unsigned long long)(100 + idx * 10);
            std::wstring v;
            if (dlg_input_text(r.name, r.name, r.value, &v)) {
                ProjLicense& lc = g_proj.lics[idx];
                switch (off) {
                case 0: lc.name = v; break;
                case 1: lc.email = v; break;
                case 2: lc.date = v; break;
                case 3: lc.order = v; break;
                case 4: lc.comments = v; break;
                }
                g_proj.modified = true; update_all();
            }
        }
        return;
    }
}
// ============================================================
// xprotect_ui.cpp — Part F: WndProc, welcome, WinMain
// ============================================================

static bool g_drag = false;
static std::wstring g_cmd_open;

// 0.3.0: dil degisince welcome panel etiketleri tazelenir
static void update_welcome_texts() {
    if (!g_welcome) return;
    HWND wl = GetDlgItem(g_welcome, 0xA01);
    if (wl) SetWindowTextW(wl, (std::wstring(L(LS_WELCOME)) + L" " + XPROTECT_EDITION).c_str());
    HWND b1 = GetDlgItem(g_welcome, IDM_OPEN);
    if (b1) SetWindowTextW(b1, L(LS_OPEN_FILE));
    HWND b2 = GetDlgItem(g_welcome, IDM_OPENPROJ);
    if (b2) SetWindowTextW(b2, L(LS_OPEN_PROJ));
}

static void welcome_fill() {
    if (!g_welcome) return;
    HWND lb = GetDlgItem(g_welcome, 0xA04);
    if (!lb) return;
    SendMessageW(lb, WM_SETREDRAW, FALSE, 0);
    ListBox_ResetContent(lb);
    for (size_t i = 0; i < g_recent.size(); i++)
        ListBox_AddString(lb, g_recent[i].c_str());
    SendMessageW(lb, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(lb, NULL, TRUE);
}

static LRESULT CALLBACK welcome_proc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_COMMAND:
        SendMessageW(g_main, WM_COMMAND, wp, lp);
        return 0;
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    return DefWindowProcW(h, m, wp, lp);
}

static LRESULT CALLBACK main_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITMENUPOPUP: {
        // 0.3.0: menu durumlari acilista dinamik guncellenir (menu hic
        // yeniden kurulmaz — TVN_SELCHANGED flicker'i bitti)
        if (HIWORD(lp) == 0) {   // sistem menusu degil
            HMENU pop = (HMENU)wp;
            UINT en = MF_BYCOMMAND | MF_ENABLED, dis = MF_BYCOMMAND | MF_GRAYED;
            bool open = g_proj.open;
            EnableMenuItem(pop, IDM_SAVE, open && g_proj.modified ? en : dis);
            EnableMenuItem(pop, IDM_SAVEAS, open ? en : dis);
            EnableMenuItem(pop, IDM_SAVELIC, open && !g_proj.lics.empty() ? en : dis);
            EnableMenuItem(pop, IDM_CLOSE, open ? en : dis);
            EnableMenuItem(pop, IDM_UNDO, !g_undo.empty() ? en : dis);
            EnableMenuItem(pop, IDM_REDO, !g_redo.empty() ? en : dis);
            EnableMenuItem(pop, IDM_DELETE, g_prop_sel_kind >= 0 ? en : dis);
            EnableMenuItem(pop, IDM_SEARCH, open ? en : dis);
            EnableMenuItem(pop, IDM_GOTO, open ? en : dis);
            EnableMenuItem(pop, IDM_ADDFUNC, open ? en : dis);
            EnableMenuItem(pop, IDM_ADDLIC, open ? en : dis);
            EnableMenuItem(pop, IDM_EXPORTKEY, open ? en : dis);
            EnableMenuItem(pop, IDM_IMPLIC, open ? en : dis);
            EnableMenuItem(pop, IDM_IMPPROJ, open ? en : dis);
            EnableMenuItem(pop, IDM_COMPILE, open && !g_compiling ? en : dis);
            EnableMenuItem(pop, IDM_EXECUTE, open ? en : dis);
            EnableMenuItem(pop, IDM_WATERMARKS, open ? en : dis);
            EnableMenuItem(pop, IDM_DISASM, open ? en : dis);
        }
        return 0;
    }
    case WM_SIZE: layout(); return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = 860; mm->ptMinTrackSize.y = 560;
        return 0;
    }
    case WM_TIMER:
        if (g_selftest && wp == 1) {
            KillTimer(h, 1);
            printf("SELFTEST OK\n"); fflush(stdout);
            PostQuitMessage(0);
        }
        return 0;
    case WM_APP + 40: welcome_fill(); return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == IDC_MAIN_FILTER && code == EN_CHANGE) {
            if (g_filter) ctl_txt(g_filter, &g_filter_txt);
            if (g_tab == 1) func_fill();
            return 0;
        }
        if (id == 0xA04 && code == LBN_DBLCLK) {
            int sel = (int)SendMessageW(GetDlgItem(g_welcome, 0xA04), LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)g_recent.size()) open_target(g_recent[sel]);
            return 0;
        }
        switch (id) {
        case IDM_OPEN: { std::wstring p; if (pick_file(h, false, L"exe", &p)) open_target(p); return 0; }
        case IDM_OPENPROJ: { std::wstring p; if (pick_file(h, false, L"xproj", &p)) open_project_file(p); return 0; }
        case IDM_SAVE: save_project(false); return 0;
        case IDM_SAVEAS: save_project(true); return 0;
        case IDM_SAVELIC: {
            std::wstring p;
            if (pick_file(h, true, L"lic", &p)) {
                FILE* f = _wfopen(p.c_str(), L"wb");
                if (f) {
                    std::string o;
                    for (auto& lc : g_proj.lics)
                        o += w2s(lc.name) + "\t" + w2s(lc.email) + "\t" + w2s(lc.date) + "\t" +
                             w2s(lc.order) + "\t" + w2s(lc.comments) + "\n";
                    fwrite(o.data(), 1, o.size(), f);
                    fclose(f);
                    add_log(L"Licenses saved: " + p);
                }
            }
            return 0;
        }
        case IDM_CLOSE: close_project(); return 0;
        case IDM_EXIT: SendMessageW(h, WM_CLOSE, 0, 0); return 0;
        case IDM_UNDO: do_undo(); return 0;
        case IDM_REDO: do_redo(); return 0;
        case IDM_DELETE:
            if (g_prop_sel_kind == 2 && g_prop_sel_idx >= 0) {
                push_undo();
                g_proj.funcs.erase(g_proj.funcs.begin() + g_prop_sel_idx);
                g_prop_sel_kind = -1; g_proj.modified = true; update_all();
            } else if (g_prop_sel_kind == 4 && g_prop_sel_idx >= 0) {
                g_proj.lics.erase(g_proj.lics.begin() + g_prop_sel_idx);
                g_prop_sel_kind = -1; g_proj.modified = true; update_all();
            }
            return 0;
        case IDM_SEARCH: if (g_filter) SetFocus(g_filter); return 0;
        case IDM_GOTO: dlg_addfunction(true, 0); return 0;
        case IDM_ADDFUNC: dlg_addfunction(false, 0); return 0;
        case IDM_ADDLIC: dlg_addlicense(); return 0;
        case IDM_ADDFILE:
        case IDM_ADDFOLDER:
            MessageBoxW(h, L(LS_NOTPORTED), XPROTECT_EDITION, MB_ICONINFORMATION); return 0;
        case IDM_IMPLIC: {
            std::wstring p;
            if (pick_file(h, false, L"lic", &p))
                MessageBoxW(h, L(LS_NOTPORTED), XPROTECT_EDITION, MB_ICONINFORMATION);
            return 0;
        }
        case IDM_IMPPROJ: {
            std::wstring p;
            if (pick_file(h, false, L"xproj", &p)) {
                Project tmp = g_proj;
                std::wstring err;
                Project loaded;
                Project save_cur = g_proj;
                (void)loaded;
                if (load_project_from(p, &err)) {
                    Project src = g_proj;
                    g_proj = tmp;
                    for (auto& lc : src.lics) g_proj.lics.push_back(lc);
                    g_proj.modified = true;
                    update_all();
                } else {
                    g_proj = save_cur;
                    MessageBoxW(h, err.c_str(), L(LS_ERROR), MB_ICONERROR);
                }
            }
            return 0;
        }
        case IDM_EXPORTKEY: dlg_exportkeypair(); return 0;
        case IDM_COMPILE: start_compile(); return 0;
        case IDM_EXECUTE: exec_dropdown(); return 0;
        case IDM_WATERMARKS: dlg_watermarks(); return 0;
        case IDM_DISASM: open_disasm_for_selection(); return 0;
        case IDM_SETTINGS: dlg_settings(); return 0;
        case IDM_HELP: dlg_help(); return 0;
        case IDM_HOMEPAGE:
            ShellExecuteW(h, L"open", L"https://github.com/0xByteDeveloper/xiangu", NULL, NULL, SW_SHOWNORMAL);
            return 0;
        case IDM_ABOUT: dlg_about(); return 0;
        default:
            if (id >= IDM_RECENT_BASE && id < IDM_RECENT_BASE + 8) {
                int i = id - IDM_RECENT_BASE;
                if (i < (int)g_recent.size()) open_target(g_recent[i]);
                return 0;
            }
            return 0;
        }
    }
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->hwndFrom == g_toolbar) {
            if (nm->code == TBN_DROPDOWN) {
                NMTOOLBARW* tb = (NMTOOLBARW*)lp;
                if (tb->iItem == IDM_EXECUTE) exec_dropdown();
                else if (tb->iItem == IDM_TPL_SAVE) templates_dropdown();
                return TBDDRET_DEFAULT;
            }
            return 0;
        }
        if (nm->hwndFrom == g_tabstrip) {
            if (nm->code == NM_CLICK) {
                // checkgroup degisimi: state oku
                int t = 0;
                if (SendMessageW(g_tabstrip, TB_ISBUTTONCHECKED, IDC_TBB_FUNCS, 0)) t = 1;
                else if (SendMessageW(g_tabstrip, TB_ISBUTTONCHECKED, IDC_TBB_DETAILS, 0)) t = 2;
                switch_tab(t);
            }
            return 0;
        }
        if (nm->hwndFrom == g_tree) {
            if (nm->code == TVN_SELCHANGEDW) {
                NMTREEVIEWW* tv = (NMTREEVIEWW*)lp;
                long idl = (long)tv->itemNew.lParam;
                if (idl == 0) { g_prop_sel_kind = 0; g_prop_sel_idx = -1; }
                else if (idl >= 100 && idl < 200) { g_prop_sel_kind = 2; g_prop_sel_idx = idl - 100; }
                else if (idl >= 200 && idl < 300) { g_prop_sel_kind = 4; g_prop_sel_idx = idl - 200; }
                else { g_prop_sel_kind = -1; g_prop_sel_idx = -1; }
                fill_prop();
                // 0.3.0: menu yeniden KURULMAZ — Delete durumu WM_INITMENUPOPUP'da
            } else if (nm->code == NM_RCLICK) {
                DWORD pos = GetMessagePos();
                POINT p; p.x = GET_X_LPARAM(pos); p.y = GET_Y_LPARAM(pos);
                TVHITTESTINFO ht; memset(&ht, 0, sizeof(ht));
                ht.pt = p; ScreenToClient(g_tree, &ht.pt);
                HTREEITEM hi = TreeView_HitTest(g_tree, &ht);
                if (hi) tree_context(hi);
            }
            return 0;
        }
        if (nm->hwndFrom == g_func) {
            if (nm->code == NM_DBLCLK) {
                NMITEMACTIVATE* ia = (NMITEMACTIVATE*)lp;
                if (ia->iItem >= 0 && g_tab == 1) {
                    LVITEMW it; memset(&it, 0, sizeof(it));
                    it.mask = LVIF_PARAM; it.iItem = ia->iItem;
                    ListView_GetItem(g_func, &it);
                    size_t fi = (size_t)it.lParam;
                    if (fi < g_pe.funcs.size()) {
                        const PeFunc& f = g_pe.funcs[fi];
                        int pidx = g_proj.find_func(f.name);
                        if (pidx < 0) dlg_addfunction(false, f.rva);
                        else { g_prop_sel_kind = 2; g_prop_sel_idx = pidx; fill_prop(); }
                    }
                }
            } else if (nm->code == NM_RCLICK) {
                // 0.3.0: Fonksiyonlar sekmesi sag-tik menusu
                int sel = ListView_GetNextItem(g_func, -1, LVNI_SELECTED);
                if (sel >= 0) {
                    HMENU m = CreatePopupMenu();
                    AppendMenuW(m, MF_STRING, IDM_ADDFUNC, L(LS_ADDFUNC));
                    AppendMenuW(m, MF_STRING, IDM_DISASM, L(LS_DISASM));
                    int cmd = track_menu(m);
                    if (cmd == IDM_ADDFUNC) dlg_addfunction(false, 0);
                    else if (cmd == IDM_DISASM) open_disasm_for_selection();
                }
            }
            return 0;
        }
        if (nm->hwndFrom == g_prop) {
            if (nm->code == NM_DBLCLK) {
                NMITEMACTIVATE* ia = (NMITEMACTIVATE*)lp;
                prop_row_edit(ia->iItem);
            }
            return 0;
        }
        return 0;
    }
    case WM_APP_LOG:
        add_log((wchar_t*)lp);
        HeapFree(GetProcessHeap(), 0, (void*)lp);
        return 0;
    case WM_APP_DONE: {
        g_compiling = false;
        wchar_t b[32]; _snwprintf(b, 32, L"rc=%d", (int)wp);
        add_log(std::wstring(L(LS_DONE)) + L"  " + b);
        if (g_autosave && !g_proj.path.empty()) save_project(false);
        build_menu();
        return 0;
    }
    case WM_CLOSE:
        if (g_proj.modified) {
            int r = MessageBoxW(h, L"Save changes to project?", L(LS_QUESTION), MB_YESNOCANCEL | MB_ICONQUESTION);
            if (r == IDCANCEL) return 0;
            if (r == IDYES) save_project(false);
        }
        save_settings();
        DestroyWindow(h);
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_LBUTTONDOWN: {
        RECT sr; GetWindowRect(g_split, &sr);
        POINT sp; sp.x = GET_X_LPARAM(lp); sp.y = GET_Y_LPARAM(lp);
        ClientToScreen(h, &sp);
        if (sp.x >= sr.left - 3 && sp.x <= sr.right + 3) { SetCapture(h); g_drag = true; }
        break;
    }
    case WM_MOUSEMOVE: {
        if (g_drag) {
            RECT sr; GetWindowRect(g_split, &sr);
            POINT sp; sp.x = GET_X_LPARAM(lp); sp.y = GET_Y_LPARAM(lp);
            ClientToScreen(h, &sp);
            int nx = g_split_x + (sp.x - sr.left);
            RECT rc; GetClientRect(h, &rc);
            if (nx < 200) nx = 200;
            if (nx > rc.right - 360) nx = rc.right - 360;
            if (nx != g_split_x) { g_split_x = nx; layout(); }
        }
        break;
    }
    case WM_LBUTTONUP:
        if (g_drag) { g_drag = false; ReleaseCapture(); save_settings(); }
        break;
    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT && g_split) {
            POINT p; GetCursorPos(&p); ScreenToClient(h, &p);
            RECT sr; GetWindowRect(g_split, &sr);
            POINT a; a.x = sr.left; a.y = sr.top; ScreenToClient(h, &a);
            POINT b; b.x = sr.right; b.y = sr.bottom; ScreenToClient(h, &b);
            if (p.x >= a.x - 3 && p.x <= b.x + 3) {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }
        }
        break;
    }
    }
    return DefWindowProcW(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nCmd) {
    // ini yolu
    {
        std::wstring d = exe_dir();
        wcsncpy(g_ini, (d + L"\\XProtectUI.ini").c_str(), MAX_PATH - 1);
    }
    // komut satiri
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (!wcscmp(argv[i], L"--selftest")) g_selftest = true;
            else if (!wcscmp(argv[i], L"--open") && i + 1 < argc) g_cmd_open = argv[++i];
            else if (!wcscmp(argv[i], L"--cli") && i + 2 < argc) {
                // ---- 0.3.0 CLI modu (otomatik test / betik kullanimi) ----
                // XProtectUI.exe --cli <girdi> <cikti> [--type V|M|U] [--strength 1..3]
                //   [--all | --funcs a,b] [--func Isim:TIP ...]
                //   [--antidebug] [--strenc] [--memprot] [--novmenc]
                // Not: GUI altprocessi oldugu icin log stdout'a VE <cikti>.log'a yazilir.
                std::string in = w2s(argv[i + 1]), out = w2s(argv[i + 2]);
                i += 2;
                Options opt;
                opt.default_type = 0;
                bool have_funcs = false;
                for (int j = i + 1; j < argc; j++) {
                    if (!wcscmp(argv[j], L"--type") && j + 1 < argc) {
                        std::wstring t = argv[++j];
                        opt.default_type = (t == L"M") ? 1 : (t == L"U" ? 2 : 0);
                    } else if (!wcscmp(argv[j], L"--strength") && j + 1 < argc) {
                        opt.mut_strength = _wtoi(argv[++j]);
                    } else if (!wcscmp(argv[j], L"--all")) {
                        opt.all = true;
                    } else if (!wcscmp(argv[j], L"--funcs") && j + 1 < argc) {
                        std::string fl = w2s(argv[++j]);
                        size_t p = 0;
                        while (p < fl.size()) {
                            size_t q = fl.find(',', p);
                            std::string one = fl.substr(p, q == std::string::npos ? std::string::npos : q - p);
                            if (!one.empty()) opt.funcs.push_back(one);
                            if (q == std::string::npos) break;
                            p = q + 1;
                        }
                        have_funcs = true;
                    } else if (!wcscmp(argv[j], L"--func") && j + 1 < argc) {
                        std::string one = w2s(argv[++j]);
                        size_t c = one.find_last_of(':');
                        std::string nm = (c == std::string::npos) ? one : one.substr(0, c);
                        int tv = 0;
                        if (c != std::string::npos) {
                            std::string t = one.substr(c + 1);
                            tv = (t == "M") ? 1 : (t == "U" ? 2 : 0);
                        }
                        opt.funcs.push_back(nm);
                        opt.ftypes[nm] = tv;
                        have_funcs = true;
                    } else if (!wcscmp(argv[j], L"--antidebug")) opt.antidebug = true;
                    else if (!wcscmp(argv[j], L"--strenc")) opt.strenc = true;
                    else if (!wcscmp(argv[j], L"--memprot")) opt.memprot = true;
                    else if (!wcscmp(argv[j], L"--novmenc")) opt.vmenc = false;
                }
                if (!have_funcs && !opt.all) opt.all = true;
                std::vector<std::string> log;
                int rc = protect(in, out, opt, log);
                FILE* lf = fopen((out + ".log").c_str(), "wb");
                for (auto& l : log) {
                    printf("%s\n", l.c_str());
                    if (lf) { fputs(l.c_str(), lf); fputc('\n', lf); }
                }
                printf("rc=%d\n", rc);
                if (lf) { fprintf(lf, "rc=%d\n", rc); fclose(lf); }
                fflush(stdout);
                LocalFree(argv);
                return rc;
            }
        }
        LocalFree(argv);
    }
    INITCOMMONCONTROLSEX icc; icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    NONCLIENTMETRICSW ncm; ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g_font = CreateFontIndirectW(&ncm.lfMessageFont);
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    g_font_bold = CreateFontIndirectW(&ncm.lfMessageFont);
    HDC dc = GetDC(NULL);
    make_imagelist(dc);
    g_il2 = ImageList_Create(16, 16, ILC_COLOR32, 3, 0);
    ImageList_Add(g_il2, letter_bmp(dc, L'P', RGB(255, 255, 255), RGB(60, 110, 200)), NULL);
    ImageList_Add(g_il2, letter_bmp(dc, L'F', RGB(255, 255, 255), RGB(40, 160, 60)), NULL);
    ImageList_Add(g_il2, letter_bmp(dc, L'D', RGB(255, 255, 255), RGB(120, 120, 130)), NULL);
    ReleaseDC(NULL, dc);
    register_dlg_class();
    WNDCLASSW wc; memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = main_proc; wc.hInstance = hI;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"XPROTECT_MAIN";
    RegisterClassW(&wc);
    WNDCLASSW wc2 = wc;
    wc2.lpfnWndProc = welcome_proc;
    wc2.lpszClassName = L"XPROTECT_WELCOME";
    RegisterClassW(&wc2);
    g_main = CreateWindowExW(0, L"XPROTECT_MAIN", XPROTECT_EDITION L" [unregistered]",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1150, 720, NULL, NULL, hI, NULL);
    if (!g_main) return 1;
    g_status = CreateWindowExW(0, STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_STATUS, hI, NULL);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_font, TRUE);
    g_tabstrip = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
        WS_CHILD | WS_VISIBLE | CCS_LEFT | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0, 0, 30, 200, g_main, (HMENU)IDC_MAIN_TABSTRIP, hI, NULL);
    SendMessageW(g_tabstrip, WM_SETFONT, (WPARAM)g_font, TRUE);
    SendMessageW(g_tabstrip, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(g_tabstrip, TB_SETIMAGELIST, 0, (LPARAM)g_il2);
    TBBUTTON tbs[3]; memset(tbs, 0, sizeof(tbs));
    tbs[0].iBitmap = 0; tbs[0].idCommand = IDC_TBB_PROJECT;
    tbs[0].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED; tbs[0].fsStyle = BTNS_CHECKGROUP;
    tbs[1].iBitmap = 1; tbs[1].idCommand = IDC_TBB_FUNCS;
    tbs[1].fsState = TBSTATE_ENABLED; tbs[1].fsStyle = BTNS_CHECKGROUP;
    tbs[2].iBitmap = 2; tbs[2].idCommand = IDC_TBB_DETAILS;
    tbs[2].fsState = TBSTATE_ENABLED; tbs[2].fsStyle = BTNS_CHECKGROUP;
    SendMessageW(g_tabstrip, TB_ADDBUTTONS, 3, (LPARAM)tbs);
    g_tree = CreateWindowExW(0, WC_TREEVIEWW, NULL,
        WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_TREE, hI, NULL);
    g_func = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_FUNC, hI, NULL);
    g_prop = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_PROP, hI, NULL);
    g_log = CreateWindowExW(0, WC_LISTVIEWW, NULL,
        WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_LOG, hI, NULL);
    g_split = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        0, 0, 0, 0, g_main, (HMENU)IDC_SPLIT_V, hI, NULL);
    g_welcome = CreateWindowExW(0, L"XPROTECT_WELCOME", NULL, WS_CHILD,
        0, 0, 0, 0, g_main, (HMENU)IDC_MAIN_WELCOME, hI, NULL);
    HWND wl = mk_ctl(g_welcome, L"STATIC", (std::wstring(L(LS_WELCOME)) + L" " + XPROTECT_EDITION).c_str(),
                     SS_CENTER, 0xA01, 40, 60, 400, 44);
    SendMessageW(wl, WM_SETFONT, (WPARAM)g_font_bold, TRUE);
    mk_ctl(g_welcome, L"BUTTON", L(LS_OPEN_FILE), BS_PUSHBUTTON | WS_TABSTOP, IDM_OPEN, 40, 130, 160, 32);
    mk_ctl(g_welcome, L"BUTTON", L(LS_OPEN_PROJ), BS_PUSHBUTTON | WS_TABSTOP, IDM_OPENPROJ, 210, 130, 160, 32);
    HWND lb = mk_ctl(g_welcome, L"LISTBOX", L"", LBS_NOTIFY | WS_VSCROLL | WS_BORDER | WS_TABSTOP,
                     0xA04, 40, 180, 400, 120);
    (void)lb;
    HWND ctl_font_targets[4] = { g_tree, g_func, g_prop, g_log };
    for (int i = 0; i < 4; i++) SendMessageW(ctl_font_targets[i], WM_SETFONT, (WPARAM)g_font, TRUE);
    ListView_SetImageList(g_func, g_il, LVSIL_SMALL);
    func_cols_setup();
    prop_cols_setup();
    log_cols_setup();
    // 0.3.0: filtre seridi (Fonksiyonlar sekmesinin ust seridi — toolbar'a binmez)
    g_filter = mk_ctl(g_main, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, IDC_MAIN_FILTER, 0, 0, 200, 24);
    build_accel();
    load_settings();
    update_welcome_texts();   // dil ayarina gore etiketler
    build_menu();             // dil + recent ile BIR KEZ kurulur
    rebuild_toolbar();        // bir kez kurulur
    // 0.3.0 DUZELTME (UI bug #5): sol panel baslangicta gorunur degildi
    // (agac WS_VISIBLE'siz yaratilip switch_tab hic cagrilmiyordu)
    switch_tab(0);
    update_all();
    welcome_fill();
    if (!g_cmd_open.empty()) open_target(g_cmd_open);
    if (g_selftest) SetTimer(g_main, 1, 1200, NULL);
    // 0.3.0 DUZELTME (UI bug #6): wine/launcher nCmd=0 (SW_HIDE) iletiyorsa
    // pencere HIC haritalanmiyor (X'te Map State: IsUnMapped — gorunumsuz
    // kaliyordu). Savunmaci gosterim:
    ShowWindow(g_main, (nCmd == SW_HIDE || nCmd == SW_SHOWMINNOACTIVE ||
                        nCmd == SW_SHOWNOACTIVATE || nCmd == SW_SHOWMINIMIZED)
                       ? SW_SHOWNORMAL : (nCmd ? nCmd : SW_SHOWNORMAL));
    UpdateWindow(g_main);
    layout();
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (g_accel && TranslateAcceleratorW(g_main, g_accel, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}
