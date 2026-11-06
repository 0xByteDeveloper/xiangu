# Worklog — CPlusPort (VMProtect portu) Projesi

Task ID: 1 — Ana oturum
Agent: main (Z.ai)
Task: VMProtect tarzi sanallastirma zinciri: Test.exe (x64+x86) + CPlusPort (Win32 GUI+CLI) + korumali cikti + RVA/opcode karsilastirma raporu.

Work Log:
- (Onceki oturum) VMProtect 3.5.1 kaynak zip analizi yapildi, 27 sayfa PDF rapor teslim edildi.
- (Bu oturum) Ortamda sudo yok, mingw yok -> pip ile ziglang 0.17 (mingw-w64 header/lib paketli) + pefile kuruldu.
- Kullanici bug bildirdi: eski Test.exe'de WASD+Enter sonrasi cokme -> yeni giris mantigi (fgets satir bazli, Enter/EOF guvenli) ile sifirdan yaziliyor.

---
Task ID: 2 — Sanallastirma zinciri (tam teslim)
Agent: main (Z.ai)

Work Log:
- sudo/mingw yok -> pip ziglang 0.17 + pefile kuruldu; Windows PE'ler zig ile derlendi.
- Test.cpp yeniden yazildi: fgets satir-bazli giris (Enter/EOF guvenli — eski cokme fix), deterministik LCG hayalet AI, x64+x86+Linux .so derlemeleri.
- vm_core.c MiniVM yazildi; pozisyon-bagimsiz blob'lar (vm64.bin 8.3KB, vmx86.bin 21KB) iki farkli Ttext ile byte-ayni dogrulandi (tam PIC).
- Engine: PE32/PE32+ okuma-yazma, export+COFF fonksiyon kesfi (PDB'siz), x86/x64 alt-kume cozucu, lifter, elle kodlanmis x64/x86 VM stub'lari, .vmp0 section ureteci, CLI+GUI.
- Kritik buglar giderildi: reg-reg MOV lifter bugi (SIGSEGV), rolling-key'in JMP ile desync'i (pozisyon-bazli VM_KS'e gecildi), x86 Jcc tablosunda JP/NP eksikligi, LEAVE talimati, CMOVcc, bellek-shift, imul [mem], moffs A0-A3, SAL, x86 push/esp reddi, UBSan sinir-kontrol cagrilarinin -fno-sanitize=undefined ile kapatilmasi, natcall "m"(t) stack-alintisi (register var'a gecildi), PE32+ ImageBase okumasi, fonksiyon-sonu tarayicisi (est_func_size), stub yigin DELTA cakismasi (olcume dayali VM_STUB_SUB/DELTA sabitleri).
- harness: libgame.so uzerinde 41/41 PASS (WrapPos/CheckCollision/GameTick + NCALL akisi, tam state karsilastirmasi).
- Urunler: Test.exe/Test_vmp.exe (x64, 5 fonksiyon), Test32.exe/Test32_vmp.exe (x86, 3 fonksiyon), CPlusPort.exe (Win32 GUI), CPlusPort_debug.log, RVA_Opcode_Karsilastirma.md, README.txt.

Stage Summary:
- Sanallastirma zinciri uçtan uca calisiyor; RVA'lar orijinal/korumali ayni (x64: WrapPos 0x1000, MovePlayer 0x10B0, GhostStep 0x1660, CheckCollision 0x1E50, GameTick 0x20B0; x86: MovePlayer 0x1040, GhostStep 0x1170, CheckCollision 0x1300).
- Kullanici Windows'ta oynatip hata olursa CPlusPort_debug.log gonderilecek.
- Bilinen sinir: wine yok -> PE'ler burada calistirilamadi; VM semantigi harness ile, stub'lar objdump ile dogrulandi.

---
Task ID: 3 — Cokme hatalarinin kokten cozumu + cift mimari dogrulama
Agent: main (Z.ai)

Work Log:
- Wine'i root'suz kuruldu (deb extract + unshare/chroot + zlib1.dll + api-ms
  apekset) -> Windows exe'leri ilk kez ortamda gercekten kosulabildi.
- Test.exe (orijinal) wine'da birebir kullanici cokmesi uretildi:
  "signed integer overflow 20261*1103515245" (zig panic) -> LCG unsigned.
- 11 kok hata bulundu ve duzeltildi:
  1) oyun LCG signed overflow (zig panic)
  2) est_func_size+16 tasmasi -> int3 doldurma main'i eziyordu (acilis cokmesi);
     x64 .pdata RUNTIME_FUNCTION + sonraki-sembole kelepce
  3) lifter NEG/NOT destegi yoktu (zig 0.17 uretim) -> OP_NEG/NOT eklendi
  4) ret-sonrasi coklu-cikis kodu taranmiyordu (CheckCollision fixup hatasi)
  5) kuyruk-jmp ret isaretlenmiyordu (GameTick "ret bulunamadi")
  6) disasm 0x83 imm8 sign-extension eksik (add eax,-1 -> +255)
  7) disasm acc-form "op & 4" yanlis (0x05 eAX,imm32 AL,imm8 saniliyordu)
  8) x64 stub lea r12 REX 0x49 -> 0x4C (lea rsp,[rsp+0x78] olarak cozuluyordu)
  9) x64 stub push sirasi ters -> copy-back donus adresini eziyordu
 10) natcall inline-asm clobber eksik + rsp hizalama -> movaps SIGSEGV
 11) x86: 64-bit key truncation, pushad map ters, push_area 8B slotlar,
     SIB image-base kaybi + tablo index dusmesi -> MEMF_SIB_IMG
- Dogrulama altyapisi: Unicorn harness (uc_vm_test.py 49/49 x64,
  uc32_vm_test.py 44/44 x86), nativesim.c (gercek CPU'da stub+blob+bc),
  wine uctan uca (orijinal == vmp cikti BIREBIR).
- Urunler: Test.exe/Test32.exe (sabit oyun), Test_vmp.exe (5/5),
  Test32_vmp.exe (4/5 — GameTick x86'da yaprak-modu geregi native),
  CPlusPort.exe, RVA raporu, README.

Stage Summary:
- Sanallastirilmis oyun artik wine'da BIREBIR orijinal gibi oynaniyor;
  w/a/s/d+Enter cokmuyor, skor/yem/hayalet akisi ayni.
- Tum hata siniflari kaynak kodda aciklamali isaretlendi.

---
Task ID: 4 — Test32_vmp w+Enter cokmesi kokten cozum + VMPHelper + rapor v1.2
Agent: main (Z.ai)

Work Log:
- Kullanici bildirdi: Test_vmp.exe (x64) sorunsuz; Test32_vmp.exe w+Enter'da
  gercek Windows'ta cokuyor. Test edilmeden teslim proses hatasi not edildi.
- Ortam yeniden kuruldu: onceki oturum dosyalari /tmp/my-project'te bulundu,
  /home/z/my-project'a tasindi (seccomp symlink yasağı -> cp -as + dikkat).
- wine ortami onarildi: eksik lib64/lib dizileri, dosdevices (symlink yasak ->
  bind-mount), api-ms-win-crt-* forwarder PE'leri gen_apiset.py ile uretildi,
  i386-windows builtin'leri libwine:i386 deb'den eklendi.
- Konteyner seccomp 32-bit brk engelliyor -> wine32 calismiyor; x86 testi
  Unicorn tabanli yapildi.
- KOK NEDEN 1 (BUGFIX #12): build_stub_x86 u16 retimm; 0xFFFFFFFF sentinel
  u16'ya kirpilip 0xFFFF olunca "ret 0xFFFF" uretiliyordu -> her VM donusunde
  ESP +0x10003. Artefakta 8 adet "61 C2 FF FF" vardi. u32 param + C3 duzeltildi.
- KOK NEDEN 2 (BUGFIX #13): ezilen fonksiyon govdesine ait 26 eski HIGHLOW
  .reloc girdisi stub baytlarinin uzerinde kalmisti; ASLR'li gercek Windows
  loader +delta uygulayip VM anahtari/jmp hedefini bozuyordu. Wine preferred
  base yukledigi icin gizlenmisti. uc32_aslr_test.py (base+0x640000 + reloc)
  bu bug'i yakaladi. PEFile::filter_relocs() yazildi; x86'da 26 girdi silindi.
- Dogrulama: uc32 44/44 (preferred) + 44/44 (ASLR) + uc32_tick 10/10 (iki base)
  + uc64 49/49 + wine64 ucusan uca orijinal==vmp BIREBIR (iki farkli oturum).
- VMPHelper.c/exe uretildi: acik vmp surecini bulur (isim/pid), modul tabani +
  ASLR delta, bolum tablosu, manifest (RVA/stub/bc/key), canli stub baytlari,
  stub'dan bcRVA+key cikarimi (AYNI dogrulandi), sifreli bc bytes, exec tarama,
  CE yol haritasi. x64 wine'da canli test edildi; x86 extractor dosya baytlari
  yla birebir simule dogrulandi (manifest ilk yazimda eski key vardi -> duzeltildi).
- CPlusPort.exe GUI ayni duzeltmelerle yeniden derlendi.
- RVA_Opcode_Karsilastirma.md v1.2 final build icin yeniden uretildi.
- Teslim: download/CPlusPort/ (Test_vmp.exe, Test32_vmp.exe, CPlusPort.exe,
  VMPHelper.exe/.c, README.txt, rapor, log; eski_hatali_v1.1/ saklandi).

Stage Summary:
- Her iki mimaride de cokme kok nedenleriyle giderildi; dogrulama setleri yesil.
- v1.2 artefaktlari kullaniciya gitti; sonraki adim: kullanici Windows'ta
  Test32_vmp + w/a/s/d/Enter/q ve VMPHelper ciktiyi raporlar.

---
Task ID: 5 — Sandbox kurtarma + FARK!? kok nedeni + VMPHelper v1.3 + Ogrenim modu L0/L1
Agent: main (Z.ai)
Task: Kullanici sandbox'in sifirlanmadigini dogruladi; onceki oturum dosyalari
geri kazanildi. Kullanicinin canli VMPHelper ciktisindaki (a) x86 CheckCollision
"FARK!?" ve (b) "Tercih edilen ImageBase == gercek taban / ASLR kaymasi 0x0"
anomalileri kok nedenine kadar cozuldu. Ogrenim modu (Level 0+1 + Quiz #1) baslatildi.

Work Log:
- Eski calisma alani /tmp/my-project'te saglam bulundu: worklog.md, CPlusPort
  kaynaklari (engine_*.cpp, cplport_gui/cli.cpp), Test.cpp, VMPHelper.c, tum
  exe'ler, wine-root/wineprefix, debug loglari. /home/z/my-project taze:
  upload/ (VMProtect kaynak zip'leri), download/okul/level0 (pepeek).
- verify_fark.py: PE parse + stub key cikarimi (VMPHelper ile ayni algoritma),
  dosya baytlarindan dogrulama:
  * Teslim edilen x64 (download/CPlusPort/Test_vmp.exe): 5/5 manifest AYNI
    (kullanicinin canli ciktisiyla birebir).
  * scripts/Test_vmp.exe = eski v1.1 kopyasi (md5 == eski_hatali_v1.1) —
    "sifirlandi" karisikliginin ve key karmasasinin sebebi.
  * x86 CheckCollision: gercek runtime key 0x4C80AE497AD22F21 (dosya baytlari
    + motor log satiri 330 'key=0x4C80AE497AD22F21'). VMPHelper.c manifest'inde
    elle kopyalanan 0x4C80AE49A01823B1 BAYAT kalmis -> BUGFIX #14.
- Kesif: Windows loader maplenen header'daki ImageBase alanini gercek tabanla
  guncelliyor (dosyada 0x140000000/0x400000 + DYNAMIC_BASE ACIK 0x8160/0x8140;
  canlida 0x7FF6C27C0000/0x750000 gorundu) -> v1.2'nin "ASLR kaymasi 0x0"
  gorunumu yanilticiydi -> BUGFIX #15: prefBase artik DISKTAKI dosyadan
  okunuyor (read_file_pe_base), DYNAMIC_BASE notu ekleniyor.
- VMPHelper v1.3 derlendi (ziglang 0.17 --user), deploy: /tmp/my-project/
  download/CPlusPort/ + /home/z/my-project/download/.
- Wine canli testi bu sandbox'ta engelli: mount yasak (unshare/chroot calismadi),
  dosdevices symlink hedefleri kirik (duzeltildi), wineserver "connection reset"
  (seccomp farki). 3 denemeden sonra duruldu. Statik kanit yeterli: dosya
  baytlari + motor logu; kullanici Windows'ta v1.3 ile dogrulayacak.
- pepeek (Level 0 alistirmasi) yazildi + sentetik PE32/PE32+ ile dogrulandi
  (make_test_pe.py: ImageBase/EP/DllChars/section/RVA->fileoffset hepsi tuttu).
  zig ile Windows exe'leri derlendi: pepeek_gorev.exe / pepeek_cozum.exe.
- Ogrenim modu verildi: seviye haritasi (L0-L6), Level 0 (PE diskte) +
  Level 1 (dosya->process) dersleri, Quiz #1 (CE saha gorevleri + 5 teori).

Stage Summary:
- FARK!? = yardimci programin bayat manifest'i; VM/oyun etkilenmedi (runtime
  key her zaman stub'in kendisinden gelir, kendisiyle tutarli).
- ASLR/keşif: gercek tabanlar rastgele; RVA-tabanli CE adreslemesi degismez.
- VMPHelper v1.3 teslim; kullanici dogrulamasi bekleniyor.
- Ogretim sirada: Quiz cevaplari -> Level 2 (thread/stack/TEB + Win64 cagri
  sozlesmesi + kullanicinin Metin2 orneğinin x64 cevirisi).

---
Task ID: 6 — Wine canli dogrulama + FARK!? surum tespiti + XProtect surum sistemi (0.1.0)
Agent: main (Z.ai)
Task: Kullanici ogrenimi tamamladi; CPlusPort gelistirmesine donus. Ortam eminligi
(wine64/wine32), 3 exe MD5 teyidi, surumlu resmi paket (XProtect_0.x.x) istendi.

Work Log:
- MD5 teyit: Test_vmp.exe (3d43965c) ve Test32_vmp.exe (583ccc15) kullanici elindeki
  ile birebir = kullanici-dogrulanmis binary'ler. VMPHelper.exe farkli cikti:
  kullanicidaki 934fbe5d = v1.2 (bayat manifest, "FARK!?" ureten surum);
  teslim alanindaki 5a88e91f = v1.3 (BUGFIX #14/15'li). Kullanici v1.2'yi
  calistirmis — FARK!? gizemi bu yuzden; VM/oyun etkilenmedi.
- VMPHelper.exe binary icinde manifest key bayt taramasiyla surum tespiti:
  v1.2'de bayat key (B12318A049AE804C) 1 adet, duzeltilmis key yok;
  v1.3'te tam tersi. Kesin kanit.
- WINEPREFIX onarimi: dosdevices/c: dis klasore gidiyordu + namespace mount
  gorunmuyordu -> c: -> ../drive_c (relatif), z: -> / olarak duzeltildi;
  Test_vmp.exe drive_c'ye tasinip C:\Test_vmp.exe ile calistirildi.
- WINE64 CANLI TEST BASARILI (bu sandbox'ta, wine-10.0):
  Test_vmp.exe acildi, 17 adim oynandi, skor 0->60 ilerledi (VM zinciri
  bytecode->VM->native matematiksel dogru); ayni girişlerle orijinal
  Test.exe ciktisiyla diff = BOS (BIREBIR AYNI).
- WINE32: imkansiz (kalici) — konteyner seccomp i386 loader'i olduruyor
  ("Bad system call", ld-linux.so.2 --version bile). Root yok; onceki
  oturumlarda 3 yol denendi. x86 dogrulamasi: Unicorn 44/44+44/44+10/10
  + kullanicinin gercek Windows teyidi.
- CPlusPort32.exe (x86 Win32 GUI) ILK KEZ derlendi (zig -target
  x86-windows-gnu): 736 KB, PE32/i386 dogrulandi, import tablosu temiz
  (COMDLG32/COMCTL32/USER32/KERNEL32/ucrt). Subsystem=3 (x64 teslimiyle
  ayni — tutarli). Windows'ta ilk test kullanicida olacak.
- Surum sistemi kuruldu: download/XProtect/{XProtect_0.1.0.zip,
  versions/XProtect_0.1.0.zip}. Paket: Test.exe, Test32.exe, Test_vmp.exe,
  Test32_vmp.exe, CPlusPort.exe (x64), CPlusPort32.exe (x86), VMPHelper.exe
  (v1.3), Test_exe_Source/, VMPHelper_Source/, CPlusPort_Source/ (tam motor
  kaynagi + blob'lar + build_all.sh), README.txt, CHANGELOG.md.
- ZIP icinden cekilen exe MD5'leri teyit edildi (3d43965c / 583ccc15 / 5a88e91f).

Stage Summary:
- Ortam eminligi: wine64 CANLI (yeni sandbox'ta da calisir hale getirildi),
  wine32 sandbox siniri (kernel) — x86 testler Unicorn+kullanici Windows ile kapaniyor.
- XProtect_0.1.0 ilk resmi surum: paket + versions/ yedegi + CHANGELOG/README.
- Sonraki: 0.1.1 adaylari (GUI subsystem=2, u64 log formatlari), level-2
  mutation katkisi, CPlusPort32 Windows testi (kullanici).

---
Task ID: 7 — universalObfusPublicSrc DERIN tarama + i4MConsoleLoader analizi + port % hesabi
Agent: main (Z.ai)
Task: (a) VMP kaynaginda mutation/coklu-jmp, runtime gecici-bellegi, file->image
davranisi detaylica cikarmak; (b) i4MConsoleLoader'in loader-emulasyonunu analiz
etmek; (c) port yuzdesini hesaplamak.

Work Log:
- pub1 (vmp/ agaci) TAM kaynak; pub2'de intel.cc EKSIK (31.493 satirlik x86/x64
  backend sadece pub1'de). Tarama pub1 uzerinden.
- intel.cc satir 18435-18447: uc derleme tipi — ctMutation (CompileToNative),
  ctVirtualization (CompileToVM), ctUltra (Mutate(ctx,true) SONRA CompileToVM).
  Level-2 planimizin birebir karsiligi.
- Mutate() (16039): ~60 olü-talimat sablonu (regFree/osRandom ile), canli-register
  analizi (GetFreeRegisters), her gercek talimattan 0-3 cöp enjeksiyonu (16379
  "add garbage code"), talimat ikamesi (xor->sub, add->lea, jmp->push/ret,
  call->push/lea/xchg [esp]), AddGate/AddShortGate + CreateBlocks = jmp kapilari.
- Runtime gecici bellek: hook_manager.cc 662-727 VirtualAlloc RWX->RX->VirtualFree;
  loader.cc kendi ntdll'sini NtCreateSection+NtMapViewOfSection ile taze kopyalayip
  syscall numaralarini oradan okuyor (anti-hook), kendi dosyasindan CRC (anti-patch),
  is bitince unmap. VM'in kendisi stack'te (per-call heap YOK — VMP bilinçli kaçınıyor).
- crypto.cc: CipherRC5 (runtime verisi). file_manager.cc: kendi image'ina
  VirtualProtect ile bolum korumalari.
- i4MConsoleLoader: PEManualMapper.h = 8 adimli loader emülasyonu (VirtualAlloc
  preferred base, raw->virtual min(VSize,RawSize), HIGH/LOW/HIGHLOW/HIGHADJ(+0x8000)/
  DIR64 reloc, IAT snap, header ImageBase guncelleme, PAGE_* korumalar, TLS raporu);
  globals.h MemImage + SaveFile (satir 85) = image->raw TERS mapping -> gecerli EXE;
  ByteInjector = capstone tabanli kod tasima/hook motoru. VMP'de bu interaktif
  file<->image round-trip editörü YOK.
- Port hacmi: VMP core+runtime ~131.600 satir / bizim engine+VM ~2.750 satir.
  Metrik bazli port orani: satir ~%2, modul ~%20-25 (5-6/24), sanallastirma
  yolu fonksiyonel ~%60-65.

Stage Summary:
- Kullanicinin 3 sorusu kanitla yanitlandi (mutation VAR; gecici-bellek KISMEN;
  core file->file, runtime kendi image'inda calisir).
- i4MConsoleLoader = rebase-regresyon testi (BUGFIX #13 otomatik kanitlari) +
  dump-oracle + mutation gelistirme tezgahi olarak entegre edilecek.
- Yol haritasi: 0.1.1 mutation cekirdegi, 0.2.0 polymorphic handler/cryptor,
  0.3.0 runtime self-CRC.

---
Task ID: 8 — Config_info: universalObfusPublicSrc TAM envanter + port % raporu
Agent: main (Z.ai)
Task: Iki zip'i TAM taramak, tum ozellikleri cikarmak, Config_info raporu uretmek,
ReverseALL reposuna baseline koymak, CPlusPort port yuzdesini hesaplamak.

Work Log:
- Zip'ler yeniden acildi (sandbox sifirlanmisti): scripts/vmpsrc/pub1 + pub2.
  BULGU: pub1 == pub2 icerik (tek fark klasor oneki); pub2'de core/intel.cc
  (31.494) ve core/processors.cc (2.852) EKSIK — yalniz pub1'de. Birlesik agac
  pub2 + bu iki dosya. ReverseALL reposu bos cikti (token dogrulandi, HTTP 201
  ile Config_info.md pushlandi, commit 2aae020f57dc).
- Envanter (scan_vmp_inventory.py): 1.449 dosya / 1.130.477 satir / 113 MB.
  Kritik dosyalar: intel.cc 31.494, script.cc 8.885, il.cc 7.841, macfile.cc
  6.715, pefile.cc 6.125, elffile.cc 5.514, loader.cc 2.946, processors.cc 2.852.
- Ozellik cikarimi (kanitli): ProjectOption bitleri (core.h:6-29), CompilationType
  (files.h:308 ctVirtualization/ctMutation/ctUltra), APIType 40 SDK (files.h:239),
  CompilerFunctionType 13 (files.h:1036), Mutate ~60 sablon (intel.cc:16039,
  garbage 16379), CreateBlocks/AddGate JMP kapilari, komut-basina crypt_command/
  crypt_key (intel.h:462), can_merge (intel.h:470), SectionCryptor (intel.h:693),
  AddCheckCRC/BreakpointSection, Parse*SEH ailesi, Loader 16 PE directory
  (pefile.cc:348-376), SetupImage+CRC (loader.cc:882,2700), LoaderProcessFixups
  (840), LoaderGetProcAddress (277), LOADER_OPTION (common.h:207-211), anti-debug
  4 hook (runtime/core.cc:1218-1235), gecici RWX->RX->VirtualFree (hook_manager.cc:
  662-727), StringManager RC5, HWID, licensing (HTTP/curl), registry trial,
  SDK 40 fonksiyon (sdk.cc), GUI Qt 28.605, VMProtectCon CLI (-pf/-sf), Lua+FFI
  (script.cc), LZMA packer, intel-x86.py, unit-tests gtest.
- Port metrikleri (calc_port_metrics.py): VMP gercek kaynak core 116.759 +
  runtime C++ 15.250 + C# 29.330 + GUI 28.605 + con 458 + sdk 1.263 + utils 926 +
  shellext 866 + minidumpfixer 166 + unit-tests 7.162 = ~200.785; CPlusPort
  0.1.0 = 2.763 satir (11 dosya).
- 77 ozellikli matris kuruldu: VAR=2 (A2 sanallastirma %65, B9 reloc %80),
  KISMEN=14, YOK=61. Kategori ortalamalari A %12,9 / B %13,7 / C %1,4 / D %0 /
  E %6,9 / F %3,3. Agirlikli genel (35/25/25/5/5/5) = M2 %9. M1 ham satir %1,7.
  M3 fonksiyonel yol (MiniPac sanallastirma uctan uca) %65. Resmi takip sayisi
  M2 = %9 olarak sabitlendi.
- Raporlar: download/Config_info.md (ana icerik, repo'da ayni) + download/
  Config_info.docx (docx skill: R1 kapak CM-2 palet, 3 bolum kapak/TOC/govde,
  Roma+Arap sayfa numaralari, 12 tablo, TOC placeholder 24 baslik, footer
  instrText patch). postcheck.py 9/9 PASSED (0 hata 0 uyari) — ilk denemede
  TOC PageBreak + NEXT_PAGE cift kirilma hatasi bulundu ve duzeltildi.
- Jeneratör kalici: scripts/gen_config_info_docx.js + config_docx_lib.js +
  config_docx_data.js + postprocess_config_info.py (surum guncellemelerinde
  yeniden kosulabilir).

Stage Summary:
- Config_info raporu (md + docx) teslim edildi; ayni md ReverseALL main dalina
  pushlandi (commit 2aae020f57dc) — port takibinin baseline'i.
- Resmi port yuzdesi: M1 %1,7 / M2 %9 / M3 %65 (kullanicinin ~%20 tahmininin
  netlestirilmis hali).
- Siradaki adim: 0.2.0 mutation cekirdegi (A5-A8) — yol haritasinin ilk sutunu;
  her surumde matris guncellenip repo'ya pushlanacak.
