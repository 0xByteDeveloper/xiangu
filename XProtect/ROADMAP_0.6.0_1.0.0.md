# XProtect Yol Haritası — 0.6.0'dan %100'e (1.0.0)

Tarih: 2026-11-09 | Durum bazı: 0.5.1 (teslim edildi, kullanıcı onaylı: V/M/U gerçek ayrılık kanıtı)
Referans: VMProtect 3.5.1 (vmpsrc/pub2, 584 dosya / 896.428 satır derin tarama) + Config_info_ozellik_matrisi.csv (100 satır)

## 0) Bugünkü Durum (0.5.1)

Referans `core/core.h` ProjectOption enum'undaki 19 bayrağın XProtect karşılığı:

| VMProtect bayrağı | Anlamı | XProtect 0.5.1 |
|---|---|---|
| cpEncryptBytecode | bytecode şifreleme | VAR (vmenc, keystream+fonksiyon key) |
| cpLoaderCRC | loader CRC | VAR (self + .text sayfa sayfa, ASLR uyumlu) |
| cpImportProtection | IAT koruması | DENEYSEL (0.5.0; 0.6.0'da olgunlaşacak) |
| cpCheckDebugger | hata ayıklayıcı taraması | KISMEN (--antidebug) |
| — SDK işaretleri (Begin/End) | kaynak bölge seçimi | VAR (xprotect_sdk.h) |
| — Runtime API çözümü | gecikmeli import | VAR (PEB + FNV-1a thunk) |
| cpPack | LZMA paketleme | YOK (0.6.0) |
| cpResourceProtection | kaynak koruma | YOK (0.6.0) |
| cpStripFixups | reloc temizliği | YOK (0.6.0) |
| cpStripDebugInfo | debug bilgisi silme | YOK (0.6.0) |
| cpIncludeWatermark | filigran | YOK (0.7.0) |
| cpCryptValues | değer şifreleme | KISMEN |
| cpRunnerCRC | runner CRC | YOK (0.9.0) |
| cpEncryptRegs | register şifreleme | YOK (0.8.0) |
| cpCheckVirtualMachine | VM tespiti | YOK (0.9.0) |
| cpMemoryProtection | bellek koruması | YOK (0.9.0) |
| cpCheckKernelDebugger | kernel debugger | YOK (0.9.0) |
| cpDebugMode | debug modu | YOK (0.9.5) |
| cpVirtualFiles | sanal dosyalar | YOK (0.9.0) |
| cpInternalMemoryProtection | iç bellek koruması | YOK (0.9.0) |
| cpLoader | tam loader (SetupImage/Fixups) | KISMEN |

Lisans sistemi (RSA/serial/HWID/activation): YOK (0.7.0)
Mutasyon derinliği (tam çözücü 577 komut, canlı-register, crypt_command/key,
SectionCryptor, can_merge, polimorfik handler, BreakpointSection, SEH): KISMEN (0.8.0)

Kategori yüzdeleri (matris ağırlıklı, 0.5.x teslimleriyle düzeltilmiş):
- A Çekirdek (disasm/lifter/VM/mutation): ~%65
- B PE işleme: ~%80
- C Runtime/koruma: ~%35
- D Lisans: ~%5
- E GUI: ~%85
- F Araçlar/CLI: ~%65
- **GENEL: ~%60** (Windows PE kapsamı)

## 1) Sürüm Planı

### 0.6.0 — Paketleme (LZMA) + Import Protection v2 + PE temizliği
- cpPack: LZMA sıkıştırma (referans: runtime/loader.cc LZMA decoder'ının tersi),
  .vmp0 girişinde açıcı stub, orijinal bölüm sıkıştırma, boyut kazanımı kanıtı.
- cpResourceProtection: .rsrc bölüm şifreleme + runtime açıcı.
- cpImportProtection: "deneysel" etiketi kalkar — wine uçtan uca + Unicorn kanıtı.
- cpStripFixups, cpStripDebugInfo, File CRC (disk) olgunlaştırma.
- Kanıt: metin2 V/M/U + pack → çalışır + boyut küçülür; SELFTEST OK.

### 0.7.0 — Lisans sistemi (RSA) + Watermark
- RSA-2048 anahtar üretimi + key pair export (referans: licensing_manager.cc).
- Serial üretimi/doğrulama, tarih/süre limitleri, blacklist, Lock-To-Serial.
- HWID (donanim izi; referans: runtime/hwid.cc).
- SDK yüzeyi: SetSerialNumber, IsValidLicense, GetCurrentHWID, ActivateLicense,
  GetOfflineActivationRequest/Response, DecryptStringA/W (tam).
- cpIncludeWatermark: filigran motoru + GUI Watermarks penceresi fiilen çalışır.
- registry_manager (trial) minimal.
- Kanıt: lisanslı test exe: geçersiz serial → korumalı çıkış; geçerli → 42.

### 0.8.0 — Mutasyon/VM derinliği (kalite sıçraması)
- Tam çözücü: intel.cc 577 komut kapsamı (metin2'nin kullandığı set + temel SSE).
- Canlı-register analizi (GetFreeRegisters karşılığı) → akıllı ikame/junk.
- cpCryptValues tam + cpEncryptRegs.
- Komut-başına crypt_command/crypt_key + SectionCryptor (bölüm şifreleme) + can_merge.
- Polimorfik VM handler üretimi: her derlemede farklı interpreter (kalıcı md5 değişimi).
- BreakpointSection + Parse*SEH ailesi (SEH'li fonksiyonlar güvenli koruma).
- Kanıt: aynı girdi iki derleme → farklı md5 (polimorfizm); Unicorn diferansiyel set 50+.

### 0.9.0 — Runtime sertleştirme (anti-debug / bellek)
- cpCheckDebugger tam, cpCheckVirtualMachine, cpCheckKernelDebugger.
- cpMemoryProtection + cpInternalMemoryProtection: bölüm sayfa korumaları,
  RWX→RX geçişi, guard sayfalar (referans: hook_manager.cc:662).
- Taze ntdll map (anti-hook), cpVirtualFiles, cpRunnerCRC.
- Kanıt: wine'da debugger saptama yolu kanıtı; SELFTEST OK.

### 0.9.5 — GUI tamamlama + araçlar
- Dump/Disasm görünümleri, Templates window, script editörü (JS otomasyon),
  Undo/Redo tam, dil paketleri (TR/EN/RU), Help browser, RemoteControl yüzeyi.
- shellext (sağ-tık menü), minidump fixer, gtest seti, tek-komut build, i18n.

### 1.0.0 — Parite sürümü (Windows PE kapsamında %100)
- Yukarıdakilerin tümü yeşil; 19 cp* bayrağının Windows-PE'de anlamlı 16'sı TAM.
- Uçtan uca mega-derleme kanıtı: metin2 → V/M/U + pack + IAT + resource +
  memory prot + lisans hepsi TEK derlemede; SELFTEST OK; docs + release paketi.

### Kapsam dışı (istenirse 1.1+)
- Mach-O (Mac) ve ELF (Linux) destekleri, .NET metadata koruması, C# runtime,
  DDK/driver yüzeyi, activation server (Weblm) istemcisi.

## 2) Cevap: "0.6.0'a kadar max kaç sürüm?"
0.6.0 → 0.7.0 → 0.8.0 → 0.9.0 → 0.9.5 → **1.0.0** = **5 büyük sürüm**
(0.9.5 istenirse 0.9.2'ye katlanır; toplam 5-6 release). Bugün ~%60'tayız.

## 3) Muhasebe Notları
- GitHub deposu 0.5.0'da duruyor (36caae1) — **0.5.1 push edilmedi**.
- Kullanıcının PAT'i GitHub API'de **401 Bad credentials** dönüyor
  (süresi dolmuş/iptal). Push için yeni token gerekli (repo public olduğu
  için okuma anonim de mümkün).
