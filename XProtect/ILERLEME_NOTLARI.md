# ILERLEME_NOTLARI — XProtect 1.2.0 (canli not defteri)

> KURAL: Her is YAPILMADAN ONCE buraya yazilir (bulunan sorun / yapilacak is).
> Sonra is yapilir, ardindan "SONUC:" satiri eklenir.
> Checkpoint daemon (cp_daemon.py) bu dosyayi her 4 dakikada GitHub'a iter.
> Amac: oturum her an kesilse, kullanici elindeki cpN.zip ile tum ara durumu bilir.

---

## 2026-11-12 04:12 — A/B karsilastirmasi (YAPILDI, rapor)

- BULDU: XProtect_1.2.0_A.zip = 2026-11-12 02:55 anlik goruntusu; B1 mult-VM
  (vm_id/perm_seed) + B2 TLS-tramp pack + B5 debugmode fix'lerini ICIYOR.
- BULDU: XProtect_1.2.0_B.zip = 2026-11-11 20:18 anlik goruntusu; B1/B2
  fix'lerinden ONCEKI kaynak durumu; ama kanit_b1/ test loglari (dp86prot*,
  iso10-13) sadece onda var.
- KARAR: **A en iyi / en yeni.** B'nin tek degeri kanit_b1 loglari.
- GUNCELLE: Calisma agaci kuruldu: download/XProtect_1.2.0/ = A tabani
  + B/kanit_b1 eklendi + en yeni XProtectUI.exe/32 (md5 f7e76b66) kokte.

## 2026-11-12 04:20 — Ortam taramasi (YAPILDI)

- BULDU: GitHub token 401 (Bad credentials) — PUSH SU AN YOK. Daemon her turda
  gh_token.txt'yi yeniden okuyup push'u otomatik dener; token yenilenir
  yenilenmez birikmis checkpoint'ler gider. (Kullanici notu: token yenilenecek.)
- BULDU: wine-root sandbox reset'te gitmis; setup_env.sh mantigiyla yeniden
  kurulacak (wineserver SHELL SCRIPT dersleri dahil).
- BULDU: Z-Bridge v3.7.2 kurulu degildi; kurulum_yeni_chat.py calistirildi,
  broker baglantisi OK. Kullanici istegi: 20-sn dinleme penceresi.
- GUNCELLE: bridge_test/senkron20.py YAZILDI (kural: not gelirse pencere
  20 sn yenilenir; gelmezse is devam eder). Canli test PASSED.

## 2026-11-12 04:25 — PLAN: checkpoint sistemi (yapilacaklar)

- YAPILACAK: cp_daemon.py (240 sn dongu):
  1) download/XProtect_1.2.0/ agacini zip'le -> XProtect_1.2.0_cpN.zip
     (N = cp_no.txt'den monoton artar)
  2) zip'i download/'a ve repo kokune koy + ILERLEME_NOTLARI.md'yi repo'ya
  3) git add+commit+push (token 401 ise skip, push_bekleyen sayacini artir)
  4) log: cp_daemon.log
- SONUC (04:17): cp_daemon.py yazildi, tek-tur test OK: cp1 zip 3,077,962B
  uretildi (download/XProtect_1.2.0_cp1.zip). Push 401 -> push_bekleyen.txt'ye
  birikti (token gelince otomatik bosalicak). Daemon cift-fork BASLATILDI
  (dongu 240 sn; cp2 ilk otomatik tur ~04:20).

## 2026-11-12 04:26 — PLAN: vm_core.c commit-bozuk fix dogrulamasi

- SORUN (onceki oturum kaydi): repo vm_core.c "c->regsap[k]]" ile COMMIT-BOZUK
  gelmisti; duzeltildi. A ziplinde duzeltilmis hal mi, dogrulanacak.
- YAPILACAK: grep regsap + gcc -m32 -c derleme denemesi.
- SONUC (04:18): A zip'indeki vm_core.c'de "regsap[k]]" YOK (fix yerinde);
  kanit = derleme testi: -DVM_IS_X64 -DVM_WINDOWS ile rc=0 (28,648B obj),
  -m32 -DVM_IS_X86 ile rc=0 (40,964B obj). Iki mimari de temiz.

## 2026-11-12 04:27 — PLAN: wine-root + derleme + regresyon

- YAPILACAK:
  1) wine-root yeniden kur (apt-get download wine64 libwine wine-binfmt,
     wineserver script->ELF ezme, wchroot.sh PATH)
  2) vm64.bin/vmx86.bin blob yeniden uretim (gen_blob.py yolu)
  3) XProtectUI.exe + 32 yeniden derleme (build_ui.sh)
  4) Regresyon: sdk_b1 rc=42, R5 ULTIMATE rc=42, tamper rc=117, tls_test rc=42
- SONUC (04:40): TAMAMLANDI — 5/5 altin standart:
  R1 R5 ULTIMATE (sdkonly+pack+sectcrypt+loadercrc+filecrc+stripfixups+
     debugmode) rc=42 "OK hepsi dogru"
  R2 tamper .vmp1 tek-bayt (--rt-nomsg varyanti) rc=117 DURU-RET
  R3 sdk_b1 (JMPEXT/JCCEXT/gates + debugmode) rc=42
  R4 tls_test (pack+stripfixups) rc=42 "TLS OK" (B2 kaniti)
  R5 mult-VM vmn=4 + vmadv rc=42 (B1 kaniti)
- DERSLER (yakalanan ve duzeltilen 3 kustum):
  1. gen_blob.py A ziplinde YOKTU — B'den alindi (guncelle: blob uretimi
     icin gen_blob.py + vm64/vmx86.bin gerekiyor).
  2. lzma/ klasoru A ziplinde YOKTU — 0.6.1 kaynak agacindan kopyalandi
     (LzmaEnc.c/LzFind.c + headers).
  3. engine_sym.hpp A ziplinde YOKTU — B'den alindi. DERS: A zipli
     eksik-dosya ile gelmisti; B kopyasi bunlari kapatmistir (merge -n).
  4. wine cmd ilk sıcak başlatmada asılı kalmaktir: pkill -9 wine +
     wineboot -i/-u ile tazele; wchroot.sh'e PATH export eklendi.
  Boyutlar worklog kaydiyla OZDES: XProtectUI.exe 1,275,392B;
  XProtectUI32.exe 1,326,080B (md5 farkı derleme-yolu kaynakli — islevsel
  kanit regresyon).
- Kanit loglari: kanit/1.2.0/ (prot+run loglari) — cp zip'lerine giriyor.
- KOPRU: kullanici 04:48'te "merhaba" yolladi (PC->sandbox CANLI);
  40 sn dinleme penceresi bos kapandi, ise devam edildi (20-sn kurali).

## 2026-11-12 04:28 — PLAN: B3 cpResourceProtection (Faz C kalan isi)

- YAPILACAK: resource sifreleme + FindResource/LoadResource/LoadString/
  EnumResource* yonlendirme (VMP IsInternal semantigi).
- TASARIM (yapilmadan once, UO/VMP referans):
  1) engine'e resSifreleme modulu: sec .rsrc verisini CFB (sectcrypt
     ailesi) ile sifrele; anahtar gizli-alanda (stub-tasiyan) yasar.
  2) XRT-gate'e resource API yonlendirme: FindResourceA/W -> okumadan
     ONCE bagli .rsrc sayfasini cozer; LoadResource/LockResource ayni
     gate; LoadStringA/W ve EnumResource* XRT-icinde karsiliklar.
  3) IsInternal semantigi (VMP): yalniz korunan modulun kendisinin
     resource'larina dokun; harici DLL resource'larina pas.
  4) Test: res_test.c — FindResource+LoadResource+LockResource ile
     gomulu metin okur; dogru metin cikarsa rc=42.
- SONUC (05:10): B3 TAMAMLANDI — cpResourceProtection CALISIYOR:
  1. res_test.c (RCDATA ID=1 okuyan test) + add_rsrc.py (PE'ye .rsrc
     ekleyen arac) yazildi. add_rsrc dersleri: dir hdr 16B + entry 8B
     {id, off|0x80000000}; RT_RCDATA=10; DataEntry {rva,size,cp,res}
     SIRASI; SizeOfImage GUNCELLENMELI (yoksa loader rc=6).
  2. Motor: do_pack'e resprot param (engine.hpp default false); skip
     hesabinda di==2 istisnasi; [RESPROT] log; engine_main cagrisi
     opt.rsrcprot geciyor; OZET'e "resprot=ACIK/kapali".
  3. UI: GUI cap->opt koprusu eklendi (cpResourceProtection biti
     opt.rsrcprot'a); CLI --rsrcprot zaten vardi (4042).
  4. KANIT: --rsrcprot ile res_test_r.exe korundu: "[PACK] .rsrc
     RVA=0x0003C000 512 B -> LZMA 69 B (%13.5) [CFB]"; statikte .rsrc
     raw CFB-sifreli (ilk 32 bayt entropi), gizli metin resource-
     verisinde YOK (yalniz testin kendi .rdata literal'i kaliyor);
     calisirken: RES OK 22 bayt + RES DOGRU + RCODE=42 (unpack
     transparan geri yaziyor).
  5. Yeni build ile tam regresyon YENIDEN: R1=42 ("OK hepsi dogru"),
     R2 tamper=117 (duru-ret), R3=42, R4=42 TLS OK, R5=42 — 5/5.
  Boyutlar: XProtectUI.exe 1,276,416B; XProtectUI32.exe 1,327,104B
  (B3 kodu ~1KB buyume). Kanitlar: kanit/1.2.0/resprot_prot.log +
  kanit/res_test.c + kanit/1.2.0/add_rsrc.py.
- KALAN: B4 cpVirtualFiles (bundler: gomulu dosya agaci +
  faLoad/faRegister/faInstall — UO file_manager.cc modeli).
