# XProtect 0.7.0 (FINAL) — Değişiklik Günlüğü

0.7.0-beta'dan FINAL'e: **3 kritik motor hatası kapandı + lisans uçtan uca
doğrulandı + SDK-40 tam yüzeyi**. Ek olarak kullanıcının 0.6.2 raporu olan
"Loader CRC + Strip Fixups birlikte = butunluk kontrolu kod = #2" hatası bu
sürümde kökten çözüldü.

## 1) KRİTİK: Loader CRC × Strip Fixups etkileşimi ("#2" MessageBox)

**Belirti (kullanıcı raporu, metin2client d7610 ULTRA):** Loader CRC ve
Strip Fixups İKİSİ BİRLİKTE Yes iken oyunda hasar atınca
`Xprotect: butunluk kontrolu kod = #2` MessageBox'ı çıkıyordu (tek tek
açınca sorun yoktu; oyun çökmüyordu).

**Kök neden (bayt-düzeyi kanıtlı):**
- Derleme-zamanı .text CRC'si, strip'ten ÖNCEKİ .reloc tablosu ile
  "yamalı baytları atlayarak" hesaplanıyordu (engine_main.cpp).
- `strip_fixups()` sonra .reloc verisini sıfırlıyor + dizini 0/0 yazıyor +
  ASLR bayraklarını temizliyor.
- cfg'ye strip-ÖNCESİ RELOCRVA/RELOCSZ yazılıyordu → runtime, sıfırlanmış
  .reloc'u okuyup `bsz<8` ile HİÇBİR baytı atlamıyor.
- Derleme (atlayarak) ve runtime (atlamadan) CRC'si farklı → xrt_entry
  err=2 → "#2".
- Kanıt: kullanıcının upload ettiği `metin2client_vmp_LoaderCRCveStripFixups.exe`
  içinde saklanan TEXTCRC=0x01ED0622, runtime-yeniden-hesap=0x42E27385.

**Fix:** strip varsa İKİ taraf da atlamasız düz bayt CRC'si kullanır
(cfg'ye 0/0 yazılır — strip sonrası ASLR kapalı olduğundan yükleme-zamanı
delta uygulanmaz, düz karşılaştırma doğru semantiktir).

**Doğrulama:** sdk testi x64 wine (RCODE=42), metin2 d7610
ULTRA+loaderCRC+stripFixups statik (crc eşleşiyor, e9 VM kapısı), x86 PE32
statik, SELFTEST OK, regresyonlar (loadercrc-tek / stripfixups-tek /
pack+filecrc) 42.

## 2) KRİTİK: SetSerialNumber korumalı exe içinde INVALID dönüyordu (beta açık maddesi)

Üç ayrı hata üst üste binmişti:

a) **lic_div64 tanımsızdı → STALE BLOB.** beta oturumunda xrt_license.c
   son anda düzenlenmiş ama blob yeniden derlenmemişti; ayrıca lic_div64
   hiçbir dosyada tanımlı değildi (blob derlenemiyor, motor ESKI blob
   taşıyordu → exe içinde modexp YANLIŞ → em!=00 02 → INVALID).
   FIX: freestanding shift-subtract lic_div64 eklendi; blob her iki mimaride
   de taze derlendi.

b) **LSD CRC kendini zehirliyordu.** CRC, runtime-yazılabilir state(0x120)
   ve tick(0x124) alanlarını KAPSİYORDU → ilk SetSerial'dan sonra CRC
   bozuluyor, sonraki TÜM lic_lsd() taramaları FALSE-CRC ile LSD'yi
   bulamıyor → her lisans API'si CORRUPTED'a düşüyordu.
   FIX: LSD CRC aralığı 0x000..0x11F (LSD_CRC_LEN) — salt-okunur yapı.
   (Kanıt probe: set_g1=2 → post1=1; fix sonrası state kalıcı.)

c) **lcopy argüman sırası ters.** serial em önbelleği yazılırken
   `lcopy(em, cache+4, 256)` — lcopy(d,s,n) olduğundan cache'i em ile
   dolduracağına EM'İ CACHE'İN SIFIRLARIYLA EZİYORDU → GetSerialNumberData
   hep boş isim/e-posta.
   FIX: `lcopy(cache+4, em, 256)`.

**Uçtan uca doğrulama (wine, korumalı exe):**
```
ok:      SetSerial=0 State=0  name="Ali Veli" email="ali@veli.com"
badsum:  SetSerial=2 State=2      (INVALID)
badhwid: SetSerial=32 State=32    (BAD_HWID 0x20)
expire:  SetSerial=8 State=8      (DATE_EXPIRED)
reok:    SetSerial=0 State=0      (durum toparlanması)
```
Ayrıca GetSerialNumberData artık em önbelleğinden wUserName/wEMail
(UTF-8→UTF-16), dtExpire/dtMaxBuild, bRunningTime ve bUserData'yı doldurur
(VMProtectSerialNumberData birebir).

## 3) SDK-40: APIType tam yüzeyi (core/files.h:239)

fnid uzayı 1..15'ten **1..37**'ye genişletildi (atBegin/atEnd = bölge
işaretleri; atNone hariç 39 girdinin tamamı eşleşir). Yeni genel API'ler
(xprotect_sdk.h/c — korunmamış derlemede güvenli boş döner):

- Kaynak ailesi (16..28): FindResourceA/W, FindResourceExA/W, LoadResource,
  LoadStringA/W, EnumResourceNames/Languages/Types A/W — dispatch'te
  gerçek Windows API'sine PEB-hash (IAT'sız) delege edilir.
  Kanıt: shell32 üzerinden korumalı imajda types=13, icon-names=1840.
- DecryptBuffer (29): splitmix64 keystream, kendini götüren çözme.
- Random (35): rdtsc + KUSER_SHARED_DATA karışımı.
- BoxPointer/UnboxPointer (36/37): imaj-bazlı simetrik kutu.
- İç yüzey (30..34): RuntimeInit/LoaderData/SetupImage/FreeImage/CalcCRC.
- Yeni W2(api2) sarmalayıcısı (x86+x64).

## 4) Bilinen sınırlar (0.8.0'a)
- Kaynak API delegasyonu wine testinde doğrulandı; SectionCryptor ile
  şifreli-kaynak akışı 0.8.0'da.
- Gerçek-yığın bölge modu (heap ctx) 0.8.0'da.
- x86 PE32 çalışma-zamanı kanıtı bu ortamda wine-wow64 eksikliği nedeniyle
  statiktir (motor PE32 destekler; XProtectUI32 aynı kaynakla derlendi).

## Doğrulama özeti (wine64 + Xvfb)
- SELFTEST OK
- R1 loadercrc+stripfixups = 42 | R2 +filecrc+stripdebug = 42 |
  R3 pack+loadercrc+filecrc = 42 | R4 V+M+U+loadercrc = 42
- Lisans paketi: 5/5 senaryo + isim/e-posta dolu
- SDK-40: rand/box/decbuf/enum TAM OK = 42
- metin2 d7610 ULTRA+loaderCRC+stripFixups: statik crc eşleşmesi + e9 kapı
