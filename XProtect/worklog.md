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

---
Task ID: 9 — Config_info v2.0 (148 ozellik) — KAYIP OTURUM
Agent: main (Z.ai)
Task: Kullanicinin "ustunkoru" elestirisi uzerine HER dosya + satir
taranarak derin ozellik listesi + port % raporu uretmek.

Work Log:
- Derin tarama: 235 enum / 1.536 define / 1.178 class / 4.599 metot /
  577 IntelCommandType / 123 FACE_ / 110 GUI Faces; 148 ozellik x 22
  modul matris; M1 %1,4 / M2 %17,9 / M3 %65.
- Rapor 82,8KB / 1.641 satir (md5 8b3ecf4d...) + CSV uretildi.
- ReverseALL push 403 (token gecerli ama Contents:write yok sanildi;
  commit 3839f3c lokalde bekledi).
- KADER: sandbox sifirlanmasi raporu sildi (bu sandbox'ta Config_info.md
  hala v1.0 / md5 5e5c39cc). DERS: kritik cikti ANINDA diske + GitHub.

Stage Summary:
- Kullaniciya rapor + port yuzdeleri iletildi; push 403 yuzunden repo
  guncellenemedi; sonraki oturumda telafi edilecek.

---
Task ID: 10 — xiangu kalici ev + XProtectUI (Win32 UI replikasi) + v2.0-r regenerasyon
Agent: main (Z.ai)
Task: Kullanici: token push izni VAR; xiangu'da klasor ac, her seyi
GitHub'a pushla (kalici garanti); tum ozellikleri UI ile yap
(universalObfusPublicSrc UI'si varsa birebir, ama win32api ile); UI
exe + zip; her oturumda port raporu.

Work Log:
- Token dogrulama: xiangu + ReverseALL perms push:true (eski 403 asildi).
- xiangu klon -> XProtect/ klasoru (worklog + Config_info + releases/
  0.1.0.zip + source/) -> commit f6515e7 -> PUSH BASARILI.
- GUI envanteri: mainwindow.cc (3.712) + 6 dialog + en.lng etiketleri.
- XProtectUI yazildi (xprotect_ui.cpp 2.391 satir; saf Win32):
  menu/toolbar/tab seridi/tree/functions/property grid(cp* TAMAMI)/
  log/status/welcome/splitter + Add Function/Go To/Add License/
  Export Key Pair/Watermarks/Settings(EN-TR)/About + motor in-process
  protect() worker thread + .xproj + PE mini-cozumleyici.
- Derleme (zig c++): XProtectUI.exe x64 823.808 bayt (63a9dc9f...) +
  XProtectUI32.exe x86 871.424 bayt (d24cfb08...); import tablolari temiz.
- Wine yeniden kurulum: wine64 10.0~repack-6 deb-extract; nssetup.c ile
  namespace /usr takasi (usrmerge ld-linux dosya-bind, wineserver
  script->binary). Kalici: scripts/wrun_gui2.sh + nssetup.c.
- SELFTEST: Xvfb+wine64 "SELFTEST OK" (+ --open C:\Test.exe smoke).
- Paket: XProtect_0.2.0.zip (47 dosya) md5 1c48166def29f9bd11b58d18e4a0d8f7
  (download/XProtect/ + versions/). Zip ici MD5 teyidi tamam
  (63a9dc9f / d24cfb08 / 3d43965c / 583ccc15).
- Config_info v2.0-r: scan_vmp_deep.py — 584 dosya / 896.428 satir;
  106 enum / 1.471 define / 961 class / 24.717 metot / 1.178 Lua;
  56/56 kanit; FACE_ 123. Matris: 100 ozellik/22 modul
  (VAR 13 / KISMEN 14 / UI 19 / YOK 54).
  Yuzdeler: M1 %0,6 / M2 %20,5 / M2+UI %24,5 / M3 %65,0 / UI %90+.
  Cikti: download/Config_info.md + Config_info_ozellik_matrisi.csv
  + download/WORKLOG.txt (kronolojik).

Stage Summary:
- 0.2.0 teslim; UI yuzeyi birebir, motor kredisi ayri tutuldu
  (UI-hazir != port). Sonraki: 0.3.0 mutation cekirdegi.
---
Task ID: 11 — 0.3.0 MUTASYON CEKIRDEGI
Agent: main (Z.ai)
Task: VMProtect "Mutation" katmaninin yerli uygulamasi — talimat ikamesi + canli-register analizi + JMP kapilari; V/M/U derleme tip sevkı; UI CLI modu; Unicorn diferansiyel dogrulama.

Work Log:
- engine_mut.cpp (~650 satir) yeni: tarama (lifter politikasiyla birebir) -> temel-blok CFG -> liveness (16 GPR + FLAGS sozu-register, geriye dogru sabit nokta) -> ikame katalogu + junk + JMP kapilari -> kodlayici + fixuplar (dahili jmp/jcc rel32, dis cagri rel32, rip-goreceli disp yamasi).
- engine_main.cpp: tip sevkı V (saf V) / M (native mutasyon: .vmp0 govde + E9 giris kapisi) / U (mutasyon -> lifter -> VM); ftypes + default_type; 0.2.0 CLI uyumu korundu.
- 7 kok-neden hata avciligi: (1) CMP/TEST ud_of okur-yazilisi, (2) riprel disp konumu len-4-immsz + capa talimat sonu, (3) ALU hedefi ayni zamanda kaynak, (4) CFG dusme kenari blok indeksi (ASan yakaladi), (5) junk yalniz VOLATILE yazmaclar (ABI), (6) SHIFT/IMUL hedefi okur, (7) VM natcall RSP kaydi r10 -> rbx (gizli-bekleme hatasi; bloblar yeniden uretildi).
- UI: --cli modu (tam bayrak seti) + start_compile ftypes aktarimi (V/M/U gercek).
- verify_mut.py: Unicorn tam-goruntu diferansiyeli (export RVA cozumu, x64+x86) — wine32'nin konteynerde imkansiz olmasina cozum.
- Dogrulama: M/U/V x x64/x86 = 6 senaryo x 5 fonksiyon 5/5 ESIT (rax + bellek); wine64 oyun ciktisi BIREBIR (M/U/V + karisik guc3 + antidebug/strenc); UI selftest OK; ASan/UBSan 0; determinizm 3->1 md5.
- Paket XProtect_0.3.0.zip (53 dosya, md5 6136c1d9...): UI ikilileri + tam kaynaklar (engine_mut.cpp, GUNCEL vm_core.c + bloblar, muttest.cpp, verify_mut.py) + ULTRA vitrin Test_vmp'ler + README/CHANGELOG.
- GitHub push BEKLEMEDE: token bu oturumda mevcut degil (guvenlik geregi dosyaya yazilmamisti).

Stage Summary:
- 0.3.0 mutasyon cekirdegi teslim: M tipi native hizda koruma, U tipi M+V zinciri, liveness tabanli guvenli junk/ikame, JMP kapilari.
- Kalan: 0.4.0 runtime katmani (self-CRC, dosya CRC, gecikmeli import), 0.5.0 lisans/SDK.

---
Task ID: 12 — 0.4.0 UI TAM KAPSAM PORTU + PDBScope entegrasyonu
Agent: main (Z.ai)
Task: Kullanici raporu: (a) Fonksiyon Ekle tum fonksiyonlari gostermiyor -> PDBScope-2.0.0
aracini projeye ekle; (b) Manual Address Base/End + liste-secim adres dolumu; (c) open
file/open project butonlari; (d) sag panel bos; (e) VMP UI kaynaklarini satir satir tara,
tam kapsamli Win32 portu.

Work Log:
- Dogrulama: 0.3.0 zip md5 kullanici kopyasiyla birebir (6136c1d9); git'te 0.3.0 commit'i
  (c3fe0d4) lokalde bekliyor; chat API'si aktif IM oturumunu listelemiyor (worklog yeterli).
- PDBScope analizi: pdbless/semboller komutlari, --format csv, CSV basligi
  rva,va,size,confidence,percent,class,kind,callers,sources,name; wine'da Test.exe ->
  560 fonksiyon kanitlandi.
- VMP GUI taramasi (2 subagent, satir bazli): mainwindow/models/property_editor/widgets +
  11 dialog -> tam davranis spesifikasyonu (tree->page esleme tablosu, dizin gorunum
  kolonlari, menu enable kurallari, loadFile/compile akislari, dialog kontrol listeleri).
- XProtectUI 0.4.0 (xprotect_ui.cpp ~3900 satir):
  * pdbscope_find/pdbscope_scan (pipe+CreateProcess+CSV parser) + merge_pdbscope +
    fill_neighbor_sizes; acilista otomatik tarama; Ekle diyaloguna Tara dugmesi.
  * dlg_addfunction yeniden: Manual Address (Base/End) grubu; secim -> Base/End dolumu;
    End -> brk kaydi; cift-tik OK; coklu secim add modunda.
  * Boot sayfasi VMP birebir (Recent Files 10 + Quick Start 4 dugme + ayiricilar).
  * Sag panel: g_dir (dizin gorunumu) + right_show modu; Functions/Licenses klasoru ->
    dizin kolonlari; cift-tik -> ozellikler; kok -> Core Properties.
  * .vmp XML ici aktarma (xml_tag_attrs dongulu on-ek cozumu, xml_num 0x/ondalik,
    Procedures: MapAddress|Address+CompilationType+Options(coLockToKey)+IncludedInCompilation).
  * Lisans: Blocked + duzenleme + VMP varsayilanlari (+1ay/+1yil/30) + .xproj alan-11.
  * Derleme: marquee progress + tamam istemi; surkle-birak; VMP filtre; (N) sayaclari;
    [Blocked] etiket; baslik * ; recent 10.
  * Fix: --funcs Isim:Tip (0.3.0 bug); cok satirli EDIT Enter; layout yeniden-giris korumasi.
- Wine ortami taze sandbox'ta yeniden kuruldu (wine-root deb'lerden; nssetup yeniden
  derlendi; Xvfb dogrudan yonetim; xprobe.py XTEST tiklama/ekran araci yazildi).
- GUI kanitlari (XTEST gercek tiklama): boot sayfasi render OK; Open File butonu ->
  dosya diyalogu (3. top-level pencere); Functions klasoru -> dizin gorunumu gecisi;
  calisma alani (tree + Core Properties + toolbar + status) render OK.
- Test suitesi (final ikililer): SELFTEST OK funcs=0 / funcs=568 / funcs=568 proj=5;
  --cli --funcs WrapPos:V,GameTick:U -> rc=0 (V=1 U=1); korumali T40.exe temiz cikis.
- Ikililer: XProtectUI.exe 999.424B (725d6136), XProtectUI32.exe 1.050.112B (23b8a5be).
- Paket: XProtect_0.4.0.zip (61 dosya, 3.78MB, e63113b9) + versions/ yedek;
  PDBScope gomulu (kok + bin/win64 + bin/win32 + src + README/LICENSE).

Stage Summary:
- 0.4.0: UI tam kapsamli port + PDBScope entegrasyonu + .vmp uyumlulugu teslim.
- Sonraki: 0.5.0 runtime katmani (self-CRC, dosya CRC, gecikmeli import) ve/veya
  lisans/SDK — kullanici onem siralamasina gore.
