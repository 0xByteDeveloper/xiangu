XProtect 0.4.0 — UI Tam Kapsam Portu + PDBScope Entegrasyonu
=============================================================

Bu surumde ne var (0.3.0 -> 0.4.0)
-----------------------------------
1) PDBScope ENTEGRASYONU (tum fonksiyonlar):
   - Kullanicinin PDBScope-2.0.0 araci UI'ye gomuldu. Dosya acilinda
     XProtectUI, "pdbscope pdbless <hedef> --format csv" subprocess'ini
     kosar; CSV'yi parse edip TUM fonksiyonlari listeye ekler
     (CERTAIN/HIGH/MEDIUM siniflari, varsayilan guven esigi 20).
   - Test.exe orneginde: 13 -> 568 fonksiyon (CERTAIN/HIGH: 549).
   - Arac sirasiyla: XProtectUI.exe'nin dizini -> bin\win64 ->
     bin\win32 -> PATH icinde aranir. Bu paket PDBScope/ altinda
     hem kokte hem bin\win64 + bin\win32 olarak gomuludur.
   - Fonksiyon Ekle penceresindeki "Tara" dugmesi taramayi elle
     tetikler (Source kolonu: PDBScope).

2) Fonksiyon Ekle penceresi (kullanici istegi):
   - "Manual Address" grubu eklendi: Base + End kutulari.
   - Listeden secilen fonksiyonun adresi Base'e, fonksiyon sonu
     End'e otomatik yazilir. Tek adres + End girildiyse fonksiyonun
     "End of Function" siniri olarak kaydedilir.
   - Cift tiklama = OK (VMProtect davranisi).

3) Acilis (boot) sayfasi VMProtect birebir yeniden kuruldu:
   - Sol: Recent Files (10 dosya, tikla-ac).
   - Sag: Quick Start — Open File / Open Project / Examples... / Contents.
   - Open File / Open Project butonlari test edildi (wine icinde
     gercek fare tiklamasiyla dosya diyalogu acildigi dogrulandi).

4) Sag panel mimarisi (VMProtect directory_tree_ muadili):
   - Agacta "Functions (N)" secilince sag taraf dizin gorunumune gecer:
     Name | Address | Protection | Size | Source.
   - "Licenses (N)" secilince: Customer Name | Email | Date | Blocked.
   - Dizin satirina cift tiklama -> oge ozellik izgarasinda acilir.
   - Dosya kokune tiklayinca Core Properties (tum cp* secenekleri).

5) VMProtect .vmp proje dosyalari ACILIYOR:
   - File > Open artik .xproj + .vmp + exe'yi otomatik ayirt eder.
   - .vmp XML'i tolerant tarayici ile okunur: InputFileName, Options,
     Procedures (MapAddress/Address, CompilationType, Options
     (coLockToKey=0x2000), IncludedInCompilation).

6) Lisanslar (license_dialog.cc birebir varsayilanlar):
   - Blocked bayragi (diyalog + ozellik izgarasi + sag-tik menusu).
   - Varsayilanlar: Expiration = bugun+1 ay, Max Build = bugun+1 yil,
     Running Time Limit = 30.
   - Lisans duzenleme (License Properties) sag-tik menusu.

7) Derleme akisi (progress_dialog muadili):
   - Derleme sirasinda marquee ilerleme penceresi.
   - Basarili derleme sonrasi "Korumali dosya calistirilsin mi?" istemi.
   - Baslikta degisiklik yildizi: "proje* — XProtect Ultimate".

8) Diger:
   - Dosya surukle-birak ile acma (DragAcceptFiles).
   - Open filtresi VMProtect birebir: Executable Files + Project Files.
   - Agacta (N) sayaclari, engelli lisans [Blocked] etiketi.
   - CLI duzeltmesi: --funcs Isim:Tip (0.3.0'da sessizce 0 eslesme
   ureten hata giderildi; artik V/M/U tipi dogru aktarilir).
   - Cok satirli metin kutularinda Enter artik yeni satir (OK degil).

Dogrulama (wine64, bu paketin ikilileriyle)
--------------------------------------------
  SELFTEST OK funcs=0                        (bos acilis)
  SELFTEST OK funcs=568 proj=0               (--open Test.exe)
  SELFTEST OK funcs=568 proj=5               (--open test_proj.vmp)
  --cli Test.exe T40.exe --funcs WrapPos:V,GameTick:U  -> rc=0
  T40.exe (korumali) wine'da calisti, temiz cikis.

Kullanim
--------
  XProtectUI.exe                  -> arayuz (x64)
  XProtectUI32.exe                -> arayuz (x86)
  1. Open File ile hedef .exe/.dll sec (pdbscope taramasi otomatik).
  2. Fonksiyonlar sekmesi/agacindan fonksiyon ekle (Ins).
  3. Derleme Turu: Sanallastirma (V) / Mutasyon (M) / Ultra (U).
  4. Compile (F9) -> <ad>_vmp.exe ayni klasore uretilir.

Notlar
------
  - pdbscope.exe yoksa UI icindeki kendi kesfi (Export/COFF/PDB/MAP/
    Stub) kullanilir — arac opsiyoneldir ama TUM fonksiyon listesi
    icin onerilir.
  - Buyuk dosyalarda pdbless taramasi birkac saniye surebilir
    (200 MB Electron ~28 sn; arayuz bekleme imleci gosterir).
  - XProtectUI.ini: dil, recent, layout ayarlari.

— XProtect Ultimate 0.4.0
