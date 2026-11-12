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
- SONUC: (doldurulacak)

## 2026-11-12 04:28 — PLAN: B3 cpResourceProtection (Faz C kalan isi)

- YAPILACAK: resource sifreleme + FindResource/LoadResource/LoadString/
  EnumResource* yonlendirme (VMP IsInternal semantigi).
- SONUC: (doldurulacak)
