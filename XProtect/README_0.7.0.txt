XProtect 0.7.0 (FINAL) — Surum Notlari
========================================

0.6.2'de raporlanan "Loader CRC + Strip Fixups birlikte = butunluk kontrolu
kod = #2" hatasi KOKTEN cozuldu; 0.7.0-beta'nin acik maddeleri (SetSerial
INVALID + SDK-40) tamamlandi.

HIZLI BASLANGIC
- XProtectUI.exe (x64) / XProtectUI32.exe (x86)
- Komut satiri ornekleri:
  XProtectUI.exe --cli gir.exe cik.exe --func FonksiyonAdi:U --loadercrc
  XProtectUI.exe --cli gir.exe cik.exe --add D7610:D766C:U --loadercrc --stripfixups
  XProtectUI.exe --cli gir.exe cik.exe --sdkonly --pubkey <512hex> --prodcode <16hex>
- Keygen: scripts/xprotect_keygen.py --key key.pem --prodcode ... [--name --email
  --expire --maxbuild --runtime --data --hwid] [--badsum --badhwid]

BU SURUMDE YENI
1) loaderCRC x stripFixups "#2" fixi (detay: CHANGELOG_0.7.0.md)
2) Lisans: RSA-2048 serial dogrulama korumali exe icinde uçtan uca calisir;
   HWID (volser+hostname), su dangtasi, prodcode, expire/maxbuild/runtime,
   kara-liste alanlari; GetSerialNumberData isim/e-posta/veri doldurur.
3) SDK-40: 37 fnid'lik tam APIType yuzeyi (kaynak ailesi IAT'siz delege,
   DecryptBuffer, Random, BoxPointer/UnboxPointer, ic kancalar).

SDK KULLANIMI
#include "xprotect_sdk.h" + xprotect_sdk.c'yi linkleyin.
Korunmamis derleme: -DXPROTECT_NO_SDK.
Ornek testler: test_lic_full.c, test_sdk40.c, sdk_ornek_test_rt.c

KANITLAR (kanit/ klasoru)
- r1..r4 kosu ciktlari (RCODE=42) + derleme loglari
- lisans 5-senaryo ciktisi
- sdk40 ciktisi (enum 13/1840)
- metin2 d7610 ULTRA+loaderCRC+stripFixups statik dogrulama ciktisi
- selftest ciktisi

SINIRLAR
- x86 PE32 calisma-zamani kaniti wine-wow64 eksik; statik dogrulandi.
- Paketleme (LZMA) TLS callback'li exe'lerde duru RED (0.9.0 tam-loader).
