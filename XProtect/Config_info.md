# Config_info — universalObfusPublicSrc TAM Özellik Envanteri ve CPlusPort Port Durumu

**Proje:** XProtect (CPlusPort) — VMProtect 3.5.1 kaynaklarının 1:1 C++ portu
**Rapor sürümü:** 1.0 · **Tarih:** 2026-10-24 · **Referans repo:** github.com/0xByteDeveloper/ReverseALL
**Kaynak zipler:** universalObfusPublicSrc.zip (pub1) + universalObfuspublicsrc2.zip (pub2)

---

## Yönetici Özeti

İki zip karşılaştırıldı: **pub1 ve pub2 birebir aynı ağaçtır** (tek fark: pub1'de klasör ön eki `vmp/` ve pub2'de EKSİK olan 2 dosya — `core/intel.cc` 31.494 satır ve `core/processors.cc` 2.852 satır — yalnız pub1'de vardır). Birleşik ağaç: 1.449 dosya, ~1,13M satır (üçüncü parti ve binary dump'lar dahil).

Port edilebilir gerçek proje kaynağı (third-party hariç): **~164,3K satır C++** + 29,3K satır C# (.NET runtime) + 7,2K satır test. CPlusPort (XProtect 0.1.0) bu kaynak karşısında **2.763 satır** gerçek port içermektedir.

**Port yüzdesi üç metrikle:**
| Metrik | Sonuç |
|---|---|
| M1 — Ham satır oranı | **%1,7** |
| M2 — Özellik matrisi (77 özellik, kategori ağırlıklı) | **%9** |
| M3 — Fonksiyonel yol (MiniPac sanallaştırma zinciri) | **%65** |

Kullanıcının ~%20 tahmini M1-M2 ile M3 arasındadır; "1:1 birebir port" hedefinin resmi takip sayısı **M2 = %9**'dur. Şu an VAR olan tek bütünsel yetenek **x86/x64 sanallaştırma zinciri** (disasm → lift → VM → stub → manifest → PE yazma, wine64 canlı doğrulamalı).

---

## 1. Kaynak Envanteri

### 1.1 Ağaç yapısı (birleşik pub1+pub2)

| Klasör | Dosya | Satır | Boyut | İçerik |
|---|---|---|---|---|
| core/ | 110 (+2) | 763.851 (+34.346) | 74,8 MB | Motor çekirdeği + gömülü runtime DLL dump'ları (.inc) |
| third-party/ | 285 | 151.182 | 5,8 MB | lzma, lua, libffi, libudis86, demangle, tinyxml, qt, scintilla, gtest/gmock, diskid |
| runtime/ | 246 | 54.792 | 2,3 MB | C++ runtime (15.250) + C# .NET runtime (29.330) + testler |
| VMProtect/ | 211 | 44.789 | 2,25 MB | Qt GUI (moc dahil) |
| test-binaries/ | 26 | 35.810 | 7,7 MB | Test PE'leri |
| help/ | 277 | 31.169 | 4,2 MB | Yardım (.htm) |
| utils/ | 82 | 12.472 | 0,5 MB | intel-x86.py, x86disasm, ipn_tool, ipn_sqlclr |
| unit-tests/ | 32 | 11.365 | 0,39 MB | gtest tabanlı motor testleri |
| vcproj/ | 53 | 8.328 | — | VS proje dosyaları |
| tools/ | 8 | 5.573 | 3,0 MB | as.exe, dlltool, implib, mkexp, reimp (link araçları) |
| examples/ | 21 | 2.958 | — | SDK örnekleri |
| sdk/ | 37 | 2.050 | — | VMProtectSDK (C/C++/PAS/VB/.NET) |
| langs/ | 4 | 1.121 | — | Arayüz dilleri |
| shellext/ | 9 | 918 | — | Windows sağ-tık menü eklentisi |
| VMProtectCon/ | 12 | 504 | — | Konsol derleyici |
| win_runtime.Tests/ | 11 | 669 | — | Runtime testleri |
| app/ | 3 | 633 | — | macOS .app paketi |
| minidumpfixer/ | 5 | 166 | — | Minidump onarım aracı |
| (kök) | 13 | 1.944 | — | makefile'lar, .sln, README |

**Toplam:** 1.449 dosya / 1.130.477 satır / ~113 MB. Kod dosyaları: 118 .cc + 224 .h + 51 .c + 124 .cs + 35 .cxx + 13 .cpp.

### 1.2 Kritik dosya satır sayıları

| Dosya | Satır | Rol |
|---|---|---|
| core/intel.cc | 31.494 | x86/x64 backend: disasm+mutation+sanallaştırma (yalnız pub1'de!) |
| core/script.cc | 8.885 | Lua scripting API (FFI ile tüm nesneler expose) |
| core/il.cc | 7.841 | .NET CIL backend |
| core/macfile.cc | 6.715 | Mach-O backend |
| core/pefile.cc | 6.125 | PE backend |
| core/elffile.cc | 5.514 | ELF backend |
| runtime/loader.cc | 2.946 | Gömülü loader (anti-patch/anti-debug/VM check) |
| core/processors.cc | 2.852 | VM processor soyutlaması (yalnız pub1'de!) |
| runtime/core.cc | 1.364 | Runtime çekirdeği (anti-debug hook'ları, cpuid, firmware) |
| core/dotnetfile.cc | 12.426 | .NET metadata okuma/yazma |
| runtime/VMProtect.Runtime/VirtualMachine.cs | 4.285 | .NET VM |
| runtime/VMProtect.Runtime/VmExecutor.cs | 7.505 | .NET VM yürütücü |
| VMProtect/mainwindow.cc | 3.712 | GUI ana pencere |
| VMProtect/models.cc | 3.943 | GUI modelleri |

### 1.3 Kategori bazlı gerçek kaynak (port hedefi, .inc dump + third-party hariç)

| Kategori | Satır |
|---|---|
| core/ C++ motor | 116.759 |
| runtime/ C++ | 15.250 |
| VMProtect GUI (Qt) | 28.605 |
| VMProtectCon CLI | 458 |
| sdk/ | 1.263 |
| utils/ | 926 |
| shellext/ | 866 |
| minidumpfixer/ | 166 |
| unit-tests/ | 7.162 |
| runtime C# (.NET) | 29.330 |
| **Port edilebilir toplam** | **~200.785** |
| **CPlusPort 0.1.0 mevcut** | **2.763** |

---

## 2. Modül Kataloğu (kanıtlı özellik listesi)

### 2.A Dönüşüm motoru — core/intel.cc (31.494) + processors.cc (2.852)

Motorun kalbi. `IntelCommand` (intel.h:532) her x86/x64 talimatını taşır; `IntelCommandType` (intel.h:4) ~200 talimat sınıfı (cmMov..cmPmullld, tüm SSE/FP dahil) tanımlar.

- **Derleme tipleri (intel.cc:18435-18447):** `ctVirtualization` → CompileToVM; `ctMutation` → CompileToNative; **`ctUltra` → Mutate(ctx,true) SONRA CompileToVM**. XProtect Level-2 planının birebir karşılığı.
- **Mutate() (intel.cc:16039):** ~60 ölü-talimat şablonu (Mov/Movsx/Movzx/Not/Neg/Inc/Dec/Cmp/Test/And/Or/Xor/Add — regFree + osRandom yer tutuculu); canlı register analizi (GetFreeRegisters); her gerçek talimattan 0-3 çöp enjeksiyonu ("add garbage code" intel.cc:16379); şablonlar GetCommandInfo ile doğrulanıp serbest register'a bağlanır.
- **Talimat ikamesi:** xor→sub, add→lea, jmp→push/ret, call→push/lea/xchg [esp] Mutate içinde.
- **JMP kapıları:** `CreateBlocks` + `AddGate`/`AddShortGate` — fonksiyonu bloklara bölüp aralarına atlama kapıları ekler (**disassembly obfuscation: binlerce jmp** etkisi).
- **CompileToVM (intel.cc:9744):** talimatları IntelVMCommand bytecode'una çevirir; her VM komutuna **komut-başına şifreleme** atar (`set_crypt_command` — crypt_command/crypt_size/crypt_key, intel.h:462); `can_merge` ile ardışık handler'lar birleşir (kod boyutu küçültme).
- **CompileToNative (intel.cc:6913):** mutation-modu yeniden derleme.
- **SectionCryptor (intel.h:693):** bölüm şifreleme; runtime'da çözülür.
- **Komut içi enjeksiyonlar:** AddCheckCRCSection, AddCheckBreakpointSection, AddStoreEIPSection, AddCorrectESPSection, AddExtSection — sanallaştırılmış kod içine CRC/breakpoint kontrolü gömer.
- **SEH parsing:** ParseCxxSEH/ParseCompressedCxxSEH/ParseDelphiSEH/ParseVB/ParseScopeSEH/ParseFilterSEH/ParseNewSEH/ParseBCBSEH — tüm derleyicilerin SEH zincirlerini tanır.
- **Compiler function detection (files.h:1036):** cfCxxSEH/cfCxxSEH3/cfCxxSEH4/cfVB6SEH/cfInitBCBSEH/cfBCBSEH/cfRelocatorMinGW/cfPatchImport/cfJmpFunction/cfDllFunctionCall — derleyici üretimi kalıp fonksiyonları tanır.
- **Watermark:** AddWatermarkReference — korunan koda sahiplik imzası gömer.

### 2.B PE backend — core/pefile.cc (6.125) + pe.h

- 16 IMAGE_DIRECTORY girdisinin tam işlenmesi (pefile.cc:348-376): EXPORT/IMPORT/RESOURCE/EXCEPTION/SECURITY/BASERELOC/DEBUG/ARCHITECTURE/GLOBALPTR/TLS/LOAD_CONFIG/BOUND_IMPORT/IAT/DELAY_IMPORT/COM_DESCRIPTOR.
- PEFile::Compile (pefile.cc:3644) + PEArchitecture::Compile (pefile.cc:4551): bölüm birleştirme, resource koruma (cpResourceProtection), anti-debug export enjeksiyonu (dbghelp.dll kontrolü, pefile.cc Save aşaması), runtime objelerinin birleştirilmesi (merge runtime objects: sections/fixups/SEH handlers/runtime functions), CRC tablosu yazımı, load config yazımı.
- Formatlar: PE32+ PE32; driver (.sys, WIN_DRIVER yolu runtime'da), .NET (dotnetfile.cc 12.426), ELF (elffile.cc 5.514, lin runtime .so gömme), Mach-O (macfile.cc 6.715, mac runtime dylib gömme), DWARF (dwarf.cc), Objective-C (objc.cc), PDB (pdb.h).

### 2.C Dosya/Proje çerçevesi — core/files.cc (4.158) + core.cc (3.625) + files.h

- `BaseArchitecture::Compile` (files.cc:3224): Prepare → FunctionList::Prepare → runtime_function/region analizi → MemoryManager::Pack → FunctionList::Compile → Save. CHECKED modunda her adımda runtime hash kontrolü.
- **ProjectOption bitleri (core.h:6-29) — TÜM koruma seçenekleri:** cpDebugMode, cpCryptValues, cpIncludeWatermark, cpRunnerCRC, cpEncryptRegs, cpStripFixups, cpPack, cpImportProtection, cpCheckDebugger, cpCheckVirtualMachine, cpMemoryProtection, cpResourceProtection, cpCheckKernelDebugger, cpStripDebugInfo, cpLoaderCRC, cpEncryptBytecode, cpVirtualFiles, cpInternalMemoryProtection, cpLoader, cpMaximumProtection.
- **CompilationType (files.h:308):** ctVirtualization / ctMutation / ctUltra / ctNone.
- **APIType (files.h:239):** 40 SDK fonksiyon tipi (atBegin..atUnboxPointer) — import koruması SDK'yı tanıyıp yeniden yönlendirir.
- **LicensingManager üretim (core.cc):** License/LicensingManager sınıfları — RSA serial üretimi (GenerateSerialNumber, DecryptSerialNumber), hash, product_code, blocked bayrağı; Watermark/WatermarkManager; ProjectTemplate; Script; InternalFile (cpVirtualFiles — sanal dosya sistemi).
- ImportOption (files.h:282): ioNoReturn/ioLockToKey/ioNative/ioHasCallPrefix... — import başına davranış.

### 2.D Scripting — core/script.cc (8.885) + script.h

- Lua 5.x gömülü; FFILibrary/FFIFunction (libffi); tüm PE/Core nesneleri Lua'ya bind'li (PEFileBinder, PEArchitectureBinder, PEImportsBinder, PEResourcesBinder...). VMProtectCon `-sf` parametresiyle script koşturur (main.cc:81).

### 2.E Packer — core/packer.cc

- LZMA (third-party/lzma, LzmaEnc) ile bölüm sıkıştırma (cpPack); runtime'da LzmaDecoder açar.

### 2.F Runtime — runtime/ (C++ 15.250 satır)

Korumalı dosyaya gömülen kütüphane. win_runtime32/64.dll.inc + .sys.inc olarak core'a hex-gömülür (8 varyant: dll/sys × demo/full × 32/64; ayrıca lin .so ve mac dylib).

- **loader.cc (2.946):** LoaderAlloc/Free; LoaderFindFirmwareVendor (VM tespiti); LoaderLoadLibraryEnc/LoaderGetModuleHandleEnc (şifreli adlar); **LoaderGetProcAddress (loader.cc:277)** — hash tabanlı gecikmeli import çözümü (import koruması); LoaderMessage (mtDebuggerFound/mtVirtualMachineFound/mtFileCorrupted/mtSerialNumberRequired/mtHWIDMismatched); **LoaderProcessFixups (loader.cc:840)**; **SetupImage (loader.cc:882)**: NtOpenFile(kendi dosyası) → NtCreateSection(SEC_COMMIT) → NtMapViewOfSection **taze image** → fixup'lı kopya → **CRC doğrulama (loader.cc:2700: crc_info.Hash != CalcCRC → mtFileCorrupted)** → CPU hash (Athlon bug fix, affinity mask) → bölüm korumaları → entry'e atlama.
- **LOADER_OPTION (common.h:207-211):** CHECK_PATCH / CHECK_DEBUGGER / CHECK_KERNEL_DEBUGGER / EXIT_PROCESS / CHECK_VIRTUAL_MACHINE.
- **core.cc (1.364):** ExportedIsValidImageCRC/IsDebuggerPresent(check_kernel_mode)/IsVirtualMachinePresent/IsProtected; HookedNtProtectVirtualMemory, HookedDbgUiRemoteBreakin, HookedNtClose, HookedNtQueryObject (anti-debug hook paketi); firmware table taraması (EnumSystemFirmwareTables).
- **crypto.cc/h (1.085):** CipherRC5 (RC5Key, FACE_RC5_P/Q), CryptoContainer, SHA1, CRC32, CRCValueCryptor, BigNumber.
- **hwid.cc (687):** HardwareID — CPU (cpuid method'lu), MachineName, MAC adresleri (sysfs dahil), disk; zincirleme bswap32 blok hash'i.
- **licensing_manager.cc (1.134):** CheckLicenseDataCRC, GetSerialNumberData, DecryptBuffer, **ActivateLicense (HTTP/curl — online aktivasyon)**, offline activation string'leri.
- **string_manager.cc (183):** StringManager::DecryptDirectory/DecryptEntry/DecryptString — RC5'li şifreli string havuzu.
- **resource_manager.cc (1.021):** şifreli resource sunumu (FindResource/LoadResource/LoadString/EnumResource SDK yolu).
- **registry_manager.cc (1.389):** trial/deneme yönetimi (registry), ApplyDirtyHack.
- **file_manager.cc (1.188):** memory protection — VirtualProtect ile bölge korumaları.
- **hook_manager.cc (737):** API hook kurma (PutJump/PutJumpMem/GetInstructionSize); **geçici bellek üretimi (662-727): VirtualAlloc RWX → çalıştırılınca RX (VirtualProtect) → bitince VirtualFree** — kullanıcının sorduğu "runtime boş free mem açıp yazma sonra silme" mekanizmasının kanıtı.
- **objects.cc (58), utils.cc (289):** nesne şifreleme, Random/BoxPointer/UnboxPointer, ShowMessage.

### 2.G .NET runtime — runtime/VMProtect.Runtime (C# 29.330)

Core.cs, CpuId.cs, Crypto.cs, HardwareID.cs, LicensingManager.cs, Loader.cs, LzmaDecoder.cs, StringManager.cs, VirtualMachine.cs (4.285), VmExecutor.cs (7.505), Numerics/, Win32.cs — .NET assembly'ler için C# tabanlı VM + lisans + string/resource yönetimi. (RefVm/VmExecutor.cs testleri unit altında.)

### 2.H SDK — sdk/ (1.263)

sdk.cc: VMProtectBegin/BeginMutation/BeginVirtualization/BeginUltra/BeginVirtualizationLockByKey/BeginUltraLockByKey/End; VMProtectIsProtected/IsDebuggerPresent/IsVirtualMachinePresent/IsValidImageCRC; VMProtectDecryptStringA/W/FreeString; VMProtectSetSerialNumber/GetSerialNumberState/GetSerialNumberData/ActivateLicense/DeactivateLicense/GetOfflineActivationString/GetOfflineDeactivationString/GetCurrentHWID; resource API'leri; VMProtectDecryptBuffer/RuntimeInit/LoaderData/SetupImage/FreeImage/CalcCRC/Random/BoxPointer/UnboxPointer. VMProtectSDK32/64.def + VMProtectDDK.h + .bas/.inc/.pas bindings + VMProtect.SDK (.NET).

### 2.I Uygulamalar

- **GUI (VMProtect/, Qt, 28.605):** mainwindow (File/Edit/Project/Add/Import/Tools/Help menüleri), 21 dialog (function/license/watermark(s)/template(s)/export_key_pair/import_license/settings/progress/about/message), models.cc (ProjectModel/FunctionsModel/InfoModel/SearchModel/DirectoryModel), property_editor (2.765), remotecontrol (uzaktan derleme), help_browser, resources.cc (10.424 gömülü kaynak).
- **VMProtectCon (504):** CLI: `input output [-pf proje.vmp] [-sf script.lua]` (main.cc:81).
- **shellext (866):** Explorer sağ-tık "koru" menüsü.
- **minidumpfixer (166):** korumalı process minidump'ını onarır (debug yardımcı).

### 2.J Araçlar/Test

- utils/intel-x86.py — IDA script'i ile opcode tablo üretimi; utils/x86disasm — bağımsız disasm test aracı; utils/ipn_tool + ipn_sqlclr — lisans dosya araçları.
- unit-tests/: intel_tests.cc (4.493), il_tests, elffile_tests, files_tests, core_tests, arm_tests (gtest).
- test-binaries/: 26 test PE'si.

---

## 3. TAM ÖZELLİK MATRİSİ (77 özellik)

Durumlar: **VAR** (çalışıyor, doğrulanmış), **KISMEN** (alt-küme çalışıyor), **YOK** (port edilmedi). Öncelik: P0 (0.2.x) → P3 (1.0+).

### 3.A Dönüşüm motoru (18 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| A1 | x86/x64 tam disassembler (~200 cm sınıfı, SSE/FP) | intel.cc, intel.h:4-270 | KISMEN | 12 | P1 |
| A2 | Sanallaştırma (CompileToVM → bytecode) | intel.cc:9744 | VAR | 65 | — |
| A3 | VM handler merge (can_merge) | intel.h:470 | KISMEN | 50 | P1 |
| A4 | Komut-başına bytecode şifreleme (crypt_command/key) | intel.h:462 | KISMEN | 40 | P0 |
| A5 | Mutation: ~60 çöp şablonu + garbage enjeksiyonu | intel.cc:16039,16379 | YOK | 0 | P0 |
| A6 | Talimat ikamesi (xor→sub, jmp→push/ret, call→push/lea/xchg) | intel.cc Mutate | YOK | 0 | P0 |
| A7 | CreateBlocks + AddGate/AddShortGate (JMP kapıları) | intel.cc, intel.h:739 | YOK | 0 | P0 |
| A8 | Ultra mod (Mutate + CompileToVM) | intel.cc:18435-18447 | YOK | 0 | P0 |
| A9 | CompileToNative (mutation yeniden derleme) | intel.cc:6913 | YOK | 0 | P1 |
| A10 | SectionCryptor (bölüm şifreleme) | intel.h:693 | YOK | 0 | P2 |
| A11 | Canlı register/flag analizi (GetFreeRegisters, PushFlags) | intel.cc | KISMEN | 35 | P0 |
| A12 | Dead-code elim + reference counting | files.h ReferenceList | KISMEN | 30 | P2 |
| A13 | SEH parsing (C++/Delphi/VB/BCB/Scope/Filter) | intel.cc Parse*SEH | YOK | 0 | P2 |
| A14 | Compiler function detection (cfCxxSEH, cfDllFunctionCall...) | files.h:1036 | YOK | 0 | P2 |
| A15 | Switch/VB parsing | intel.cc ParseSwitch/ParseVB | YOK | 0 | P3 |
| A16 | Watermark byte enjeksiyonu | intel.cc AddWatermarkReference | YOK | 0 | P3 |
| A17 | Komut içi CRC check enjeksiyonu | intel.cc AddCheckCRCSection | YOK | 0 | P2 |
| A18 | Breakpoint check enjeksiyonu | intel.cc AddCheckBreakpointSection | YOK | 0 | P2 |

### 3.B Format backend (19 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| B1 | PE32/PE32+ okuma-yazma | pefile.cc:6.125 | KISMEN | 25 | P1 |
| B2 | 16 PE directory tam işleme | pefile.cc:348-376 | KISMEN | 20 | P2 |
| B3 | Import tablosu yeniden yazma | pefile.cc:882... | KISMEN | 40 | P1 |
| B4 | Export tablosu yeniden yazma | pefile.cc:4101 | KISMEN | 45 | P2 |
| B5 | Resource tree işleme/koruma | pefile.cc:352 | YOK | 0 | P2 |
| B6 | TLS callback | pefile.cc:366 | YOK | 0 | P3 |
| B7 | Load Config / CFG | pefile.cc:368,2149 | YOK | 0 | P3 |
| B8 | Delay/Bound import | pefile.cc:374-375 | YOK | 0 | P3 |
| B9 | Relocation işleme + filtreleme | pefile.cc:358 | VAR | 80 | — |
| B10 | .pdata/RUNTIME_FUNCTION (x64 unwind) | pefile.cc:354 | KISMEN | 50 | P2 |
| B11 | .NET/CIL backend (dotnetfile+il.cc) | 12.426+7.841 satır | YOK | 0 | P3 |
| B12 | ELF backend | elffile.cc:5.514 | YOK | 0 | P3 |
| B13 | Mach-O backend | macfile.cc:6.715 | YOK | 0 | P3 |
| B14 | PDB/DWARF/map sembol importu | pdb.h, dwarf.cc | YOK | 0 | P3 |
| B15 | Objective-C (objc.cc) | objc.cc | YOK | 0 | P3 |
| B16 | Driver (.sys) koruma | core WIN_DRIVER | YOK | 0 | P3 |
| B17 | Lua scripting API | script.cc:8.885 | YOK | 0 | P3 |
| B18 | LZMA packer | packer.cc:110 | YOK | 0 | P2 |
| B19 | Demangle (third-party) | cp-demangle.c | YOK | 0 | P3 |

### 3.C Runtime (21 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| C1 | Gömülü loader: SetupImage taze-map + doğrulama | loader.cc:882 | YOK | 0 | P1 |
| C2 | LoaderProcessFixups (runtime reloc) | loader.cc:840 | YOK | 0 | P1 |
| C3 | Anti-patch CRC (CHECK_PATCH) | loader.cc:2700, common.h:207 | YOK | 0 | P1 |
| C4 | Anti-debug paketi (5+ yöntem, 4 API hook) | loader.cc:1701-2257, core.cc:1218-1235 | YOK | 0 | P2 |
| C5 | Anti-VM (firmware/registry/cpuid) | loader.cc:36, core.cc:264-401 | YOK | 0 | P2 |
| C6 | Kernel debugger check | common.h:209 | YOK | 0 | P3 |
| C7 | Memory protection (VirtualProtect + NtProtect hook) | file_manager.cc:1.188 | YOK | 0 | P1 |
| C8 | Import koruması (LoaderGetProcAddress gecikmeli) | loader.cc:277 | YOK | 0 | P1 |
| C9 | API hook koruması (PutJump, GetInstructionSize) | hook_manager.cc:156-212 | YOK | 0 | P2 |
| C10 | Geçici RWX→RX→VirtualFree bellek | hook_manager.cc:662-727 | YOK | 0 | P2 |
| C11 | String encryption (RC5 StringManager) | string_manager.cc:53-90 | YOK | 0 | P2 |
| C12 | Resource encryption | resource_manager.cc:1.021 | YOK | 0 | P2 |
| C13 | HWID (CPU/MAC/MachineName/disk) | hwid.cc:687 | YOK | 0 | P3 |
| C14 | Licensing runtime (serial CRC + HTTP aktivasyon) | licensing_manager.cc:1.134 | YOK | 0 | P3 |
| C15 | Trial/registry yönetimi | registry_manager.cc:1.389 | YOK | 0 | P3 |
| C16 | RC5/SHA1/CRC/BigNumber crypto çekirdeği | crypto.cc:1.085 | YOK | 0 | P2 |
| C17 | Random/BoxPointer | utils.cc:289 | KISMEN | 30 | P2 |
| C18 | Kernel runtime (.sys) | win_runtime32.sys | YOK | 0 | P3 |
| C19 | Şifreli modül adları (LoadLibraryEnc) | loader.cc:191,423 | YOK | 0 | P2 |
| C20 | Loader mesaj sistemi (5 mesaj) | loader.cc:514 | YOK | 0 | P1 |
| C21 | .NET C# VM (VirtualMachine+VmExecutor) | 29.330 satır | YOK | 0 | P3 |

### 3.D SDK (5 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| D1 | VMProtectBegin/End marker sistemi (7 varyant) | sdk.cc:36-66 | YOK | 0 | P2 |
| D2 | SDK sorgu fonksiyonları | sdk.cc:71-102 | YOK | 0 | P2 |
| D3 | DecryptString SDK | sdk.cc:102 | YOK | 0 | P2 |
| D4 | Serial/HWID SDK | sdk.cc:140-484 | YOK | 0 | P3 |
| D5 | Çok dilli bindings (.bas/.pas/.inc/.NET) | sdk/ | YOK | 0 | P3 |

### 3.E Uygulamalar (8 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| E1 | GUI proje ağacı (8 panel) | models.cc:3.943 | KISMEN | 10 | P2 |
| E2 | GUI 21 dialog (license/watermark/template...) | VMProtect/*.cc | YOK | 0 | P3 |
| E3 | Remote control | remotecontrol.cc | YOK | 0 | P3 |
| E4 | CLI (input/output/-pf/-sf) | VMProtectCon/main.cc:81 | KISMEN | 45 | P2 |
| E5 | Proje dosyası (.vmp XML) + inifile | inifile.cc, core.h:444 | YOK | 0 | P3 |
| E6 | Template sistemi | templates_window.cc | YOK | 0 | P3 |
| E7 | Shell extension | shellext/ | YOK | 0 | P3 |
| E8 | Minidumpfixer | minidumpfixer/ | YOK | 0 | P3 |

### 3.F Doğrulama/Üretim araçları (6 özellik)

| # | Özellik | VMP kanıt | Durum | % | Öncelik |
|---|---|---|---|---|---|
| F1 | Unit-test suite (gtest, intel_tests 4.493) | unit-tests/ | KISMEN | 20 | P2 |
| F2 | Serial üretici (RSA+BigNumber) | core.cc LicensingManager | YOK | 0 | P3 |
| F3 | Watermark üretici/yöneticisi | core.cc WatermarkManager | YOK | 0 | P3 |
| F4 | License DB (XML/INI) | core.cc, inifile.cc | YOK | 0 | P3 |
| F5 | intel-x86.py opcode tablo üretici | utils/intel-x86.py | YOK | 0 | P3 |
| F6 | Çok dilli arayüz (lang.cc) | lang.cc, langs/ | YOK | 0 | P3 |

**Dağılım:** VAR=2 · KISMEN=14 · YOK=61 (77 özellik).

---

## 4. Port Yüzdesi Hesabı

### 4.1 Kategori ortalamaları (matris)

| Kategori | Özellik | Ortalama % |
|---|---|---|
| A Dönüşüm motoru | 18 | %12,9 |
| B Format backend | 19 | %13,7 |
| C Runtime | 21 | %1,4 |
| D SDK | 5 | %0 |
| E Uygulamalar | 8 | %6,9 |
| F Araçlar | 6 | %3,3 |

### 4.2 Ağırlıklı genel (dönüşüm %35 · format %25 · runtime %25 · SDK %5 · GUI %5 · araç %5)

**M2 = 12,9·0,35 + 13,7·0,25 + 1,4·0,25 + 0·0,05 + 6,9·0,05 + 3,3·0,05 ≈ %9**

### 4.3 Üç metrik

| Metrik | Tanım | Sonuç |
|---|---|---|
| M1 Ham satır | 2.763 / ~164,3K (C++ kaynak, third-party hariç) | **%1,7** |
| M2 Özellik matrisi | 77 özellik ağırlıklı | **%9** |
| M3 Fonksiyonel yol | MiniPac x86/x64 sanallaştırma uçtan uca (disasm→lift→VM→stub→manifest→PE; wine64 canlı doğrulama + Unicorn 49/49+44/44) | **%65** |

**Resmi takip sayısı:** 1:1 birebir port hedefi için **M2 ≈ %9**. Kullanıcının ~%20 tahmini M1-M2 (genel kapsam) ile M3 (hedef işlev) arasında bir sezgidir; M2'yi modül-ağırlıksız düz ortalama ile alsaydık %6,3, ham modül sayısıyla (2-3/8 kategori) %25-30 çıkardı — bu yüzden ağırlıklı M2 standarttır.

### 4.4 VAR/KISMEN özelliklerin gücü

Mevcut port VMP'nin en zor zincirini (sanallaştırma) uçtan uca çalıştırır ve VMP'de OLMAYAN doğrulama altyapısına sahiptir: Unicorn harness (49/49 x64 + 44/44 x86 preferred + 44/44 ASLR + 10/10 tick), nativesim (gerçek CPU), wine64 canlı birebir diff, VMPHelper canlı denetleyici (manifest/stub/bytecode/key çıkarımı), BUGFIX #1-15 dokümante.

---

## 5. 1:1 Birebir Port Yol Haritası

| Sürüm | Kapsam | Özellikler | Kümülatif M2 |
|---|---|---|---|
| 0.2.0 | Mutation çekirdeği | A5, A6, A7, A8, A11→tam | ~%17 |
| 0.2.1 | VM derinliği | A4→tam, A3→tam | ~%19 |
| 0.3.0 | Runtime loader | C1, C2, C3, C20 | ~%23 |
| 0.3.1 | Bellek koruması | C7, C8, C19 | ~%27 |
| 0.4.0 | Anti-debug/VM + string | C4, C5, C11, D1-D3 | ~%32 |
| 0.4.1 | Hook + kaynak koruması | C9, C10, C12, A17, A18 | ~%35 |
| 0.5.0 | Lisans/HWID | C13, C14, C15, D4, F2, F3, F4, A16 | ~%40 |
| 0.6.0 | Pack + GUI + SEH | B18, A13, A14, E1, E2, E4→tam, F1 | ~%45 |
| 0.7.0 | PE tam kapsam | B1→tam, B2, B4→tam, B5, B10, A9, A10, A12 | ~%50 |
| 0.8.0 | Formatlar | B12, B13 (+F5, F6, E5-E7) | ~%55 |
| 0.9.0 | .NET | B11, C21, B15, B6-B8, B16 | ~%60 |
| 1.0.0 | Kalan + olgunlaşma | B14, B17, B19, D5, E3, E8, C6, C18 | ~%65-70 |

Not: %100 M2'ye ulaşmak için VMP'nin tüm üçüncü parti entegrasyonları (Qt/scintilla yardım arayüzü, 10+ dil) hariç tutulmuştur; bu tablo "koruyucu motor işlevselliği"ni baz alır. Satır-bazlı hedef: motor ~35-45K satır C++.

### Öncelik gerekçeleri

- **P0 (mutation + VM şifreleme):** Kullanıcının ana hedefi Level-2 = UltraVirt+mutation. A5-A8 VMP'nin "binlerce jmp" görünümünü üretir; A4+A3 bytecode güvenliğini VMP seviyesine çıkarır. İlk sıra çünkü mevcut zincirin ÜZERİNE eklenir, yeni altyapı gerektirmez.
- **P1 (runtime loader + memory protection + import koruması):** "crash vermeyen çalışan Test_vmp.exe" artığı olarak koruma kalitesini tek sıçramada yükseltir; VMPHelper'ın ölçtüğü yüzey (manifest/CRC) birebir karşılık bulur.
- **P2:** Anti-debug/VM, string/resource encryption, SDK marker'ları — saha (CE) dayanıklılığı.
- **P3:** Lisans/HWID ve diğer formatlar — ürünleştirme katmanı.

---

## 6. Sonuç

1. İki zip aynı ağaçtır; tam kaynak pub2 + pub1'in intel.cc + processors.cc'sidir. **Kritik uyarı: universalObfuspublicsrc2.zip tek başına eksiktir** (x86/x64 backend yok).
2. universalObfusPublicSrc'de kullanıcıya önceki sorularının yanıtına kanıt: mutation + binlerce-jmp VAR (intel.cc CreateBlocks/AddGate); runtime geçici bellek VAR (hook_manager.cc:662-727 RWX→RX→free); file→image açma VAR (loader.cc SetupImage); i4MConsoleLoader'daki interaktif file↔image editörü VMP'de YOK.
3. Port durumu: 77 özellikten VAR=2, KISMEN=14, YOK=61. Resmi port yüzdesi (ağırlıklı matris) **≈%9**; ana hedef işlev (sanallaştırma zinciri) **%65** fonksiyonel.
4. Birebir port için en yüksek getiri/kolaylık sırası: 0.2.x mutation → 0.3.x loader/CRC → 0.4.x anti-debug+string. Her sürüm XProtect_0.x.x paketiyle teslim edilip ReverseALL reposundaki bu matrisle güncellenecek.
