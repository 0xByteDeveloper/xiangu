# XProtect — Sürüm Geçmişi

> VMProtect 3.5.1 mimarisi referans alınarak C++ ile yazılmış PE sanallaştırıcı (CPlusPort motoru)
> Her sürüm ZIP olarak `versions/` klasöründe yedeklenir. En son sürüm ana klasörde durur.

---

## 0.3.0 — 2026-11-07 (MUTASYON ÇEKİRDEĞİ)

### Eklenen
- **Mutasyon tipi (M)** — VMProtect "Mutation" davranışının yerli uygulaması: fonksiyon
  makine kodunda kalır (VM yok, tam native hız); talimat ikamesi + çalışan junk +
  JMP kapıları ile yeniden üretilip `.vmp0` bölümüne taşınır, orijinal girişte E9 kapısı.
- **Ultra tipi (U)** — Mutasyon + Sanallaştırma zinciri: mutasyonlu akış VM bayt-koduna
  çevrilir (M+V, VMProtect "Ultra").
- **Canlı-register (liveness) analizi** — temel-blok CFG üzerinde 16 GPR + FLAGS
  sözü-register'ı ile geriye doğru veri-akışı (sabit nokta). Junk yalnız KANITLANMIŞ
  ölü ve VOLATILE yazmaçlara yazılır (ABI kuralı); bayrak yazan junk yalnız bayraklar
  ölüyken. RET RAX/RDX dönüş değerleri, CALL tüm yazmaçları okur sayılır (muhafazakar).
- **Talimat ikamesi kataloğu** — add/sub↔lea, add↔sub (CF ters dönüşü liveness'la
  kanıtlanır), mov r,imm→push imm/pop r, xor r,r→sub r,r, test r,r→or r,r,
  inc/dec→lea/add, not→xor r,-1, mov r1,r2→push/pop. RSP'ye LEA üretimi yasak
  (VM_DELTA yığın kuralı). Tüm formlar lifter-güvenli (U aynı akıştan beslenir).
- **JMP kapıları** — EB rel8 + atlanan junk (anti-disasm tuzak/honeypot bölgeleri),
  zincirli varyantlar.
- **XProtectUI --cli modu** — otomatik test/betik kullanımı:
  `--cli <in> <out> [--type V|M|U] [--strength 1..3] [--all|--funcs|--func İsim:TIP]
  [--antidebug] [--strenc] [--memprot] [--novmenc]`; log stdout + `<çıktı>.log`.
- **UI'da V/M/U tip aktarımı** — Add Function ile seçilen derleme tipi artık fonksiyon
  bazında motora gerçekten iletilir (VMProtect davranışı; 0.2.0'da hep U-junk'lı V idi).
- **verify_mut.py** — Unicorn tam-görüntü diferansiyel doğrulayıcı: orijinal vs korumalı
  PE'yi aynı kayıt/bellek durumuyla koşturur, RAX + yazılan belleği karşılaştırır.
  x64 VE x86 çalışır (wine32 konteynerde imkânsız olduğundan x86 doğrulaması budur).
- **muttest.cpp** — Linux CLI test koşumu (motoru wine'sız koşturur; fonksiyon:tip
  argümanlarıyla karışık senaryolar).
- **[MUT] / [ULTRA] log satırları** — ikame/junk/kapı sayıları, genişleme oranı, blok sayısı.

### Düzeltilen
- **vm_natcall RSP kayıt hatası (KRİTİK, gizli-bekleme)** — `vm_natcall` asm bloğu
  dönüşte RSP'yi r10'dan geri yüklüyordu; r10 VOLATILE'dır ve çağrılan fonksiyon onu
  ezebilir → RSP çöp → SIGSEGV. Düzeltme: kayıt yuvası rbx (callee-saved); derleyici
  rbx'i kendi çerçevesinde korur. vm64.bin/vmx86.bin + vm_blob.cpp yeniden üretildi
  (x64 blob boyutu 12344B değişmedi; yerleşim sabitleri geçerli).
- **ud_of CMP/TEST sınıflandırması** — CMP/TEST hedef yazmacı YAZAR sanılıyordu
  (okur olmalı): liveness geçersiz ölü yazmaç verip junk canlı değeri eziyordu.
- **ALU hedefi okuma eksikliği** — `add ecx,[mem]` gibi formlarda hedef aynı zamanda
  kaynaktır (a = a op b); uses'a R(r1) eklendi.
- **SHIFT/IMUL okuma eksikliği** — `shl ecx,4` (ecx = ecx<<n) ve `imul r1,r1*r2`
  hedefini de okur; junk taze yüklenen değeri eziyordu (x86 MovePlayer).
- **CFG düşme kenarı** — blok ardışılığı yerine TALİMAT indeksi yazılıyordu →
  `live_in[]` yığın taşması (ASan yakaladı) → aralıklı tutarsız üretim.
- **riprel disp konumu** — disp32 her zaman talimatın son 4 baytı değil (imm takip
  edebilir); konum `len-4-immsz`, çapa TALİMAT SONU (1 bayt hedef kaymaları giderildi).
- **Kuyruk çağrı (dışarı jmp) taraması** — mutasyon tarayıcısı saw_ret işaretlemezdi
  → GameTick reddi. Lifter politikasıyla hizalandı.
- **Junk yazmaç kümesi** — callee-saved (rbx/rbp/r12-r15) junk hedefi olmaktan
  çıkarıldı (ABI ihlali: VM'in canlı callee-saved değerleri ezilirdi).

### Doğrulama (tolerans yok)
- **Unicorn diferansiyeli**: x64+x86 × M/U/V = 6 senaryo × 5 fonksiyon: 5/5 ESIT
  (RAX + bellek yazmaları birebir; fonksiyon başına 1-5 argüman seti).
- **wine64 uçtan uca**: Test.exe → M/U/V ve karışım (M+U+V, güç 3, antidebug+strenc)
  → MiniPac oyun çıktısı orijinalle BİREBİR.
- **UI --selftest**: Xvfb+wine64 → SELFTEST OK.
- **ASan+UBSan**: 0 hata; **determinizm**: 3 koşu → 1 md5.
- Vitrin ikilileri: Test_vmp.exe / Test32_vmp.exe artık ULTRA (M+V) çıktısıdır.

### Kalıt / uyumluluk
- 0.2.0 CLI çağrı düzeni (`level=2`, ftypes yok) → bayt uyumlu V+junk davranışı.
- `engine_mut.cpp` CPL_NOSUBST/CPL_NOJUNK/CPL_NOGATE/CPL_NOS1..S13/CPL_MUTDUMP
  ortam anahtarları geliştirici hata avı için kalır (varsayılan kapalı).

---

## 0.2.0 — 2026-11-06 (WIN32 UI)

### Eklenen
- **XProtectUI.exe (x64) / XProtectUI32.exe (x86)** — VMProtect Qt GUI'sinin saf Win32
  birebir replikası (2.391 satır): menüler (File/Edit/Project/Tools/Help), araç çubuğu,
  Project/Functions/Details sekmeleri, V/M/U ikonlu proje ağacı, cp* tam seçenek ızgarası,
  Add Function / Go To / License / Watermark / Export Key Pair / Settings (EN+TR) diyalogları.
  Derleme, motoru süreç içinde `protect()` worker thread'iyle koşar. .xproj proje formatı,
  şablonlar (.xtpl), Recent + AutoSave + Undo/Redo.
- PDB/MAP fonksiyon isim keşfi (motor içinde; pdb yoksa 0.1.0 davranışı korunur).
- Config_info.md v2.0-r (56/56 L2 kanıt, 100 özellik/22 modül matrisi).

---

## 0.1.0 — 2026-10-20 (ilk resmi paket)

### Eklenen
- CPlusPort.exe (x64) / CPlusPort32.exe (x86) Win32 GUI koruyucu; PE32/PE32+
  okuma-yazma, fonksiyon keşfi, x86/x64 alt-küme çözücü, lifter, VM stub üretimi,
  `.vmp0` bölümü, manifest gömme.
- Test.exe / Test32.exe (MiniPac), Test_vmp.exe / Test32_vmp.exe, VMPHelper.exe v1.3.

### Düzeltilen (15 birikmiş hata; öne çıkanlar)
- BUGFIX #12: x86 stub `ret 0xFFFF` yığın bozması.
- BUGFIX #13: bayat `.reloc` girdileri ASLR altında VM anahtarını ezmesi →
  `filter_relocs()`.
- BUGFIX #14/#15: VMPHelper manifest anahtar/baz uyumu.
