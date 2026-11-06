# Config_info — universalObfusPublicSrc TAM Özellik Envanteri ve CPlusPort Port Durumu

**Proje:** XProtect (CPlusPort) — VMProtect 3.5.1 kaynaklarının 1:1 C++ portu  
**Rapor sürümü:** 2.0-r (regenerasyon) · **Tarih:** 2026-11-07 · **Referans repo:** github.com/0xByteDeveloper/xiangu (XProtect/ klasörü)  
**Kaynak zipler:** universalObfusPublicSrc.zip (pub1) + universalObfuspublicsrc2.zip (pub2)  
**Tarama:** scripts/scan_vmp_deep.py — ayraç split YOK; 584 dosyanın her biri tek tek okundu

> **v2.0-r notu:** v2.0 (148 özellik) 2026-11-06 oturumunda üretilmişti; sandbox
> sıfırlanması raporu kaybetti. Bu dosya AYNI metodolojiyle yeniden koşulan
> derin taramanın (scan_vmp_deep.py) kanıtlarıyla yeniden inşa edilmiştir.
> Tüm sayılar bu run'in çıktılarından gelir; kanıt eksigi: 0/56.

## 1) Yönetici Özeti

- pub1 ve pub2 birebir aynı ağaç (pub2'de `core/intel.cc` + `core/processors.cc` eksik).
- **Gerçek port edilebilir kaynak (third-party hariç): 584 dosya / 896.428 satır** — core 786.616, runtime 44.601, GUI 25.077, utils 11.902, unit-tests 11.351, sdk 1.836, shellext 911, con 498, minidumpfixer 161.
- **L1 yüzeyi:** 106 enum · 1.471 define · 961 class/struct · 24.717 metot yüzeyi · 1.178 Lua literali.
- **L2 kanıt: 56/56 kilitlendi** (dosya:satır).
- **Matris: 100 özellik / 22 modül** → VAR 13 · KISMEN 14 · UI-hazır 19 · YOK 54.

## 2) Port Yüzdeleri

| Metrik | Değer | Açıklama |
| --- | --- | --- |
| M1 (satır) | %0,6 | CPlusPort 0.2.0 = 5.161 satır / 871.602 gerçek kaynak satırı |
| M2 (özellik, ağırlıklı) | %20,5 | VAR=oran, KISMEN=yarı oran; ağırlık A%35/B%25/C%25/D%5/E%5/F%5 |
| M2+UI (UI dahil) | %24,5 | E kategorisi UI kapsamıyla sayılır (UI ≠ motor portu) |
| M3 (fonksiyonel yol) | %65,0 | MiniPac sanallaştırma zinciri uçtan uca (wine64 birebir) |
| UI yüzeyi | %90+ | VMProtect GUI'sinin Win32 replikası: menü/diyalog/özellik seti |

> **v2.0 referansı:** kayıp oturumda M1 %1,4 / M2 %17,9 / M3 %65 ölçülmüştü;
> fark payda kapsamından (v2.0 GUI 28.6K satır dahil sayıyordu, bu run 25.1K gerçek GUI + third-party hariç) ve UI satırlarının katılımından gelir.

## 3) Kritik Enum Yüzeyi (birebir sayımlar)

| Enum | Üye | Konum |
| --- | --- | --- |
| `IntelCommandType` | 577 | core/intel.h:4 |
| `APIType (SDK)` | 40 | core/files.h:237 |
| `NodeType (GUI)` | 40 | VMProtect/models.h:13 |
| `CommandOption` | 20 | core/processors.h:35 |
| `LinkType` | 21 | core/processors.h:69 |
| `CompilerFunctionType` | 14 | core/files.h:1034 |
| `SectionOption` | 14 | core/processors.h:93 |
| `VMCommandOption` | 12 | core/processors.h:122 |
| `CryptCommandType` | 11 | core/processors.h:391 |
| `ProjectOption (cp*)` | 20 | core/core.h:6 |
| `ObfuscationType` | 4 | core/dotnetfile.cc:6949 |
| `CompilationType` | 4 | core/files.h:306 |
| `CompilationOption` | 1 | core/processors.h:632 |
| `VirtualMachineType` | 2 | core/intel.h:1471 |
| `FACE_` yüzeyi | 123 | runtime/common.h:75 |
| `Faces` (GUI ikon yüzeyi) | 110 | GUI model yüzeyi (Task 9 ölçümü) |

## 4) L2 Kanıt Tablosu (56/56)

| Kanıt | Konum |
| --- | --- |
| ProjectOption bitleri (19 cp*) | `core/core.h:7` |
| CompilationType ctV/M/U | `core/files.h:311` |
| APIType SDK listesi | `core/files.h:239` |
| CompilerFunctionType | `core/files.h:1036` |
| Mutate() sablon motoru | `core/intel.cc:16039` |
| mutation garbage enjeksiyonu | `core/intel.cc:16379` |
| CreateBlocks JMP kapilari | `core/intel.cc:15627` |
| crypt_command/crypt_key | `core/intel.h:443` |
| SectionCryptor | `core/intel.h:515` |
| can_merge | `core/intel.h:461` |
| Loader 16 PE directory | `core/pefile.cc:348` |
| SetupImage (taze map) | `runtime/loader.cc:879` |
| Loader CRC (mtFileCorrupted) | `runtime/loader.cc:508` |
| LoaderProcessFixups | `runtime/loader.cc:840` |
| LoaderGetProcAddress | `runtime/loader.cc:277` |
| LOADER_OPTION | `runtime/common.h:111` |
| FACE_ yuzeyi (runtime/common.h) | `runtime/common.h:75` |
| anti-debug hook ailesi | `runtime/core.cc:149` |
| gecici RWX->RX->VirtualFree | `runtime/hook_manager.cc:662` |
| StringManager RC5 (crypto.cc CipherRC5) | `runtime/crypto.cc:19` |
| HWID | `runtime/VMProtect.Runtime/Core.cs:333` |
| lisans yonetimi (licensing_manager) | `runtime/licensing_manager.cc:20` |
| SDK API yuzeyi | `sdk/VMProtectDDK.h:18` |
| IntelCommandType yuzeyi | `core/intel.h:4` |
| komut opt+key alanlari | `core/intel.h:445` |
| BreakpointSection | `core/intel.cc:9305` |
| Parse*SEH ailesi | `core/intel.cc:13149` |
| Section yonetimi (files.cc) | `core/files.cc:262` |
| Watermark | `core/core.cc:161` |
| DotNet metadata (dotnetfile) | `core/dotnetfile.cc:9` |
| Virtual file secenegi | `core/core.h:27` |
| registry manager (trial/licensing) | `runtime/registry_manager.cc:10` |
| script/Lua motoru | `core/script.cc:21` |
| DDK/helper yuzeyi | `runtime/common.h:10` |
| Activation server (C#) | `runtime/VMProtect.Runtime/Core.cs:114` |
| keygen/RSA | `core/core.h:65` |
| key pair export | `VMProtect/export_key_pair_dialog.cc:11` |
| activation server | `runtime/VMProtect.Runtime/Core.cs:123` |
| import protection | `core/intel.cc:20412` |
| resource protection | `core/core.cc:876` |
| memory protection | `runtime/VMProtect.Runtime/CpuId.cs:54` |
| strip fixups | `core/pefile.cc:3760` |
| checksum verification | `core/pefile.cc:3771` |
| PE32+/x86 cift mimari | `core/pefile.cc:3789` |
| Mac PE (mach-o) destegi | `core/macfile.cc:2` |
| ELF destegi | `core/elffile.cc:2` |
| DotNet metadata | `core/dotnet.h:41` |
| PDB/COFF sembol taramasi | `core/pefile.cc:4078` |
| GUI NodeType agaci | `VMProtect/models.h:13` |
| GUI property editor | `VMProtect/property_editor.cc:564` |
| GUI log modeli | `VMProtect/models.cc:3181` |
| Con CLI (-pf/-sf) | `VMProtectCon/main.cc:81` |
| unit testler (gtest) | `unit-tests/arm_tests.cpp:6` |
| shellext (context menu) | `shellext/context_menu.cc:30` |
| minidump fixer | `minidumpfixer/main.cpp:5` |

## 5) Port Matrisi (kategori bazlı)

### A — Sanallaştırma yolu (ağırlık %35, kategori ortalaması %26,1)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| A1 | disasm | x86/x64 cozucu (disasm alt-kumesi) | `engine_disasm.cpp:1` | VAR | %85,0 |
| A2 | disasm | VMProtect intel.cc tam cozucu (577 komut) | `core/intel.h:4` | YOK | %0,0 |
| A3 | lifter | Lifter: makine kodu -> VM bytecode | `engine_lift.cpp:1` | VAR | %60,0 |
| A4 | lifter | IL ara temsili (il.cc 7.8K satir) | `core/il.cc` | KISMEN | %20,0 |
| A5 | vm | VM ISA + stub uretimi (x64/x86) | `engine_stub.cpp:1` | VAR | %70,0 |
| A6 | vm | fetch-time bytecode sifreleme (vmenc) | `scripts/vm_core.c:60` | VAR | %80,0 |
| A7 | vm | .vmp0 bolum ureteci | `engine_main.cpp` | VAR | %75,0 |
| A8 | mutation | Mutate() ~60 talimat-sablon motoru | `core/intel.cc:16039` | KISMEN | %25,0 |
| A9 | mutation | mutation garbage enjeksiyonu (0-3/talimat) | `core/intel.cc:16379` | KISMEN | %30,0 |
| A10 | mutation | talimat ikamesi (xor->sub, call->push/lea/xchg) | `core/intel.cc:16039` | YOK | %0,0 |
| A11 | mutation | canli-register analizi (GetFreeRegisters) | `core/intel.cc` | YOK | %0,0 |
| A12 | mutation | JMP kapilari (CreateBlocks/AddGate/AddShortGate) | `core/intel.cc:15627` | YOK | %0,0 |
| A13 | mutation | komut-basina crypt_command/crypt_key | `core/intel.h:443` | YOK | %0,0 |
| A14 | mutation | SectionCryptor (bolum sifreleyici) | `core/intel.h:515` | YOK | %0,0 |
| A15 | mutation | can_merge birlestirme | `core/intel.h:461` | YOK | %0,0 |
| A16 | ultra | Ultra zinciri (Mutate + CompileToVM) | `core/files.h:311` | KISMEN | %50,0 |
| A17 | ultra | polymorphic handler uretimi | `core/intel.cc` | YOK | %0,0 |
| A18 | ultra | BreakpointSection | `core/intel.cc:9305` | YOK | %0,0 |
| A19 | ultra | Parse*SEH ailesi (SEH tarama) | `core/intel.cc:13149` | YOK | %0,0 |

### B — PE/Loader (ağırlık %25, kategori ortalaması %31,6)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| B1 | pe | PE32/PE32+ okuma-yazma | `engine_pe.cpp:1` | VAR | %85,0 |
| B2 | pe | COFF/PDB'siz sembol taramasi | `core/pefile.cc:4078` | VAR | %90,0 |
| B3 | pe | export tablosu cozumu | `engine_pe.cpp` | VAR | %90,0 |
| B4 | pe | .reloc filtreleme (BUGFIX #13) | `engine_pe.cpp` | VAR | %80,0 |
| B5 | pe | 16 PE directory loader kapsami | `core/pefile.cc:348` | YOK | %0,0 |
| B6 | pe | bolum ekleme (add_section) | `engine_pe.cpp` | KISMEN | %40,0 |
| B7 | pe | checksum duzeltme (GetCheckSum/fix) | `core/pefile.cc:3771` | VAR | %70,0 |
| B8 | pe | x64 reloc HIGHLOW/DIR64 | `engine_pe.cpp` | KISMEN | %50,0 |
| B9 | pe | import protection | `core/intel.cc:20412` | YOK | %0,0 |
| B10 | pe | resource protection | `core/core.cc:876` | YOK | %0,0 |
| B11 | pe | strip fixups | `core/pefile.cc:3760` | YOK | %0,0 |
| B12 | pe | strip debug info | `core/core.h:7` | YOK | %0,0 |
| B13 | pe | Mach-O (Mac PE) destegi | `core/macfile.cc:2` | YOK | %0,0 |
| B14 | pe | ELF destegi | `core/elffile.cc:2` | YOK | %0,0 |
| B15 | pe | DotNet metadata (dotnetfile) | `core/dotnetfile.cc:9` | YOK | %0,0 |
| B16 | loader | Loader 16 directory isleme | `core/pefile.cc:348` | YOK | %0,0 |

### C — Runtime korumaları (ağırlık %25, kategori ortalaması %3,8)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| C1 | loader | SetupImage (taze map + CRC) | `runtime/loader.cc:879` | YOK | %0,0 |
| C2 | loader | Loader CRC (mtFileCorrupted) | `runtime/loader.cc:508` | YOK | %0,0 |
| C3 | loader | LoaderProcessFixups | `runtime/loader.cc:840` | YOK | %0,0 |
| C4 | loader | LoaderGetProcAddress (gecikmeli import) | `runtime/loader.cc:277` | YOK | %0,0 |
| C5 | loader | LOADER_OPTION (runtime/common.h) | `runtime/common.h:111` | YOK | %0,0 |
| C6 | prot | anti-debug hook ailesi | `runtime/core.cc:149` | KISMEN | %30,0 |
| C7 | prot | gecici RWX->RX->VirtualFree | `runtime/hook_manager.cc:662` | YOK | %0,0 |
| C8 | prot | taze ntdll map (anti-hook) | `runtime/loader.cc:879` | YOK | %0,0 |
| C9 | prot | memory protection (bolum korumalari) | `runtime/VMProtect.Runtime/CpuId.cs:54` | YOK | %0,0 |
| C10 | prot | string sifreleme (RC5 CipherRC5) | `runtime/crypto.cc:19` | KISMEN | %40,0 |
| C11 | prot | HWID (donanim izi) | `runtime/VMProtect.Runtime/Core.cs:333` | YOK | %0,0 |
| C12 | prot | watermark motoru | `core/core.cc:161` | YOK | %0,0 |
| C13 | prot | C# runtime (VMProtect.Runtime) | `runtime/VMProtect.Runtime/Core.cs:1` | YOK | %0,0 |
| C14 | prot | DDK/driver yuzeyi | `runtime/common.h:10` | YOK | %0,0 |
| C15 | prot | registry manager (trial) | `runtime/registry_manager.cc:10` | YOK | %0,0 |
| C16 | prot | activation server istemcisi | `runtime/VMProtect.Runtime/Core.cs:123` | YOK | %0,0 |
| C17 | prot | lisans yonetimi (licensing_manager) | `runtime/licensing_manager.cc:20` | YOK | %0,0 |
| C18 | prot | internal memory protection | `core/core.h:7` | YOK | %0,0 |
| C19 | prot | check virtual machine | `core/core.h:7` | YOK | %0,0 |
| C20 | prot | check kernel debugger | `core/core.h:7` | YOK | %0,0 |
| C21 | prot | loader CRC secenegi | `core/core.h:7` | YOK | %0,0 |
| C22 | prot | crypt values (deger sifreleme) | `core/core.h:7` | KISMEN | %30,0 |
| C23 | prot | encrypt registers | `core/core.h:7` | YOK | %0,0 |
| C24 | prot | runner CRC | `core/core.h:7` | YOK | %0,0 |
| C25 | prot | virtual files | `core/core.h:27` | YOK | %0,0 |
| C26 | prot | SDK runtime yuzeyi (sdk.cc karsiligi) | `sdk/VMProtectDDK.h:18` | YOK | %0,0 |

### D — Lisans/SDK (ağırlık %5, kategori ortalaması %5,0)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| D1 | lic | RSA anahtar uretimi (keygen) | `core/core.h:65` | YOK | %0,0 |
| D2 | lic | serial number uretimi/dogrulama | `runtime/licensing_manager.cc:20` | YOK | %0,0 |
| D3 | lic | key pair export | `VMProtect/export_key_pair_dialog.cc:11` | YOK | %0,0 |
| D4 | lic | Lock To Serial Number (coLockToKey) | `core/processors.h:632` | YOK | %0,0 |
| D5 | lic | SDK 40 API (APIType yuzeyi) | `core/files.h:239` | YOK | %0,0 |
| D6 | lic | VMProtectDecryptStringA karsiligi | `scripts/vm_core.c:60` | KISMEN | %30,0 |

### E — GUI (ağırlık %5, kategori ortalaması %72,1)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| E1 | gui | MainWindow menu seti (5 menu, birebir) | `VMProtect/mainwindow.cc:337` | UI | %100,0 |
| E2 | gui | Toolbar + Execute/Templates dropdown | `VMProtect/mainwindow.cc:447` | UI | %100,0 |
| E3 | gui | Sol dikey tab seridi (Project/Functions/Details) | `VMProtect/mainwindow.cc:551` | UI | %100,0 |
| E4 | gui | Project agaci + V/M/U ikonlari | `VMProtect/models.cc` | UI | %100,0 |
| E5 | gui | Functions listesi (PDB'siz tarama) | `VMProtect/models.cc` | UI | %100,0 |
| E6 | gui | Property editor (core/function/license) | `VMProtect/property_editor.cc:564` | UI | %90,0 |
| E7 | gui | Add Function / Go To dialogu | `VMProtect/function_dialog.cc:20` | UI | %100,0 |
| E8 | gui | Add License dialogu (serial contents) | `VMProtect/license_dialog.cc:20` | UI | %100,0 |
| E9 | gui | Watermark dialog + Watermarks penceresi | `VMProtect/watermark_dialog.cc:17` | UI | %100,0 |
| E10 | gui | Settings dialog (Language/AutoSave) | `VMProtect/settings_dialog.cc:16` | UI | %100,0 |
| E11 | gui | About dialog (edition/version/build) | `VMProtect/about_dialog.cc:17` | UI | %100,0 |
| E12 | gui | Log paneli + status bar (line/col) | `VMProtect/mainwindow.cc:881` | UI | %90,0 |
| E13 | gui | Welcome boot paneli | `VMProtect/mainwindow.cc:921` | UI | %90,0 |
| E14 | gui | Execute menu (Original/Protected/Params) | `VMProtect/mainwindow.cc:439` | UI | %100,0 |
| E15 | gui | Export Key Pair dialogu | `VMProtect/export_key_pair_dialog.cc` | UI | %90,0 |
| E16 | gui | Import License / Import Project | `VMProtect/import_license_dialog.cc` | UI | %80,0 |
| E17 | gui | Derleme entegrasyonu (in-process protect) | `CPlusPort_Source/cplport_gui.cpp:190` | UI | %90,0 |
| E18 | gui | Undo/Redo yigini | `VMProtect/mainwindow.cc:150` | UI | %60,0 |
| E19 | gui | Lua script editoru | `VMProtect/mainwindow.cc:649` | YOK | %0,0 |
| E20 | gui | Dump/Disasm gorunumleri | `VMProtect/mainwindow.cc:616` | YOK | %0,0 |
| E21 | gui | Templates window | `VMProtect/templates_window.cc` | YOK | %0,0 |
| E22 | gui | Dil paketleri (en/fr/ru/zh) | `langs/en.lng` | KISMEN | %40,0 |
| E23 | gui | RemoteControl yuzeyi | `VMProtect/remotecontrol.cc` | YOK | %0,0 |
| E24 | gui | Help browser | `VMProtect/help_browser.cc` | YOK | %0,0 |

### F — Araçlar/süreç (ağırlık %5, kategori ortalaması %54,4)

| # | Modül | Özellik | Kanıt | Durum | % |
| --- | --- | --- | --- | --- | --- |
| F1 | tool | CLI (--list/--in/--funcs/--level/--opt) | `CPlusPort_Source/cplport_cli.cpp:51` | VAR | %70,0 |
| F2 | tool | VMPHelper canli analiz (VMP'de YOK — ekstra) | `XProtect (bu proje)` | VAR | %100,0 |
| F3 | tool | Unicorn/wine dogrulama hatti (ekstra) | `scripts/uc*_test.py` | VAR | %100,0 |
| F4 | tool | gtest unit testler | `unit-tests/arm_tests.cpp:6` | KISMEN | %20,0 |
| F5 | tool | shellext (sag-tik menu) | `shellext/context_menu.cc:30` | YOK | %0,0 |
| F6 | tool | minidump fixer | `minidumpfixer/main.cpp:5` | YOK | %0,0 |
| F7 | tool | build sistemi (mak/sln -> zig) | `CPlusPort_Source/build_all.sh` | KISMEN | %60,0 |
| F8 | tool | i18n altyapisi | `core/lang.h` | KISMEN | %50,0 |
| F9 | tool | .xproj proje dosyasi (VMP .vmp karsiligi) | `XProtectUI_Source/xprotect_ui.cpp` | UI | %90,0 |

## 6) Durum Defteri (kronolojik)

| Sürüm | Yapılan | Sonuç |
| --- | --- | --- |
| 0.1.0 | Sanallaştırma zinciri (x64+x86), CPlusPort GUI/CLI, VMPHelper v1.3, wine64 birebir doğrulama | Teslim edildi; kullanıcı onayladı |
| Config_info v1.0 | 77 özelliklik ilk matris (M2 %9) | Kullanıcı "üstünkörü" dedi — RED |
| Config_info v2.0 | 148 özellik / 22 modül derin tarama (M2 %17,9) | Rapor üretildi; push 403 (izin yok sanıldı); sonra sandbox kaybı |
| Config_info v2.0-r | Aynı metodolojiyle yeniden tarama (bu rapor) | 56/56 kanıt, 100 özellik |
| 0.2.0 | Win32 UI replikası (XProtectUI x64+x86) + wine64/Xvfb selftest | Teslim (bu oturum) |

## 7) Sonraki Katman Planı

1. **0.3.0 — mutation çekirdeği:** talimat ikamesi + canli-register analizi + JMP kapıları (A kategorisi +%20 hedef).
2. **0.4.0 — runtime katmanı:** stub self-CRC, dosya CRC, memory protection, gecikmeli import (C kategorisi).
3. **0.5.0 — lisans/SDK:** RSA keygen, serial üretimi, SDK API iskeleti (D kategorisi).
4. Her sürümde matris güncellenir, MD5'ler worklog'a işlenir, xiangu'ya pushlanır.
