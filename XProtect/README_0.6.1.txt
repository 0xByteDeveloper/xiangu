================================================================
XProtect 0.6.1 — Metin koruma motoru (VMProtect 3.5.1 portu)
================================================================

0.6.1 = 0.6.0 + KRITIK DÜZELTME (kullanıcı kanıtlı)

## Ne değişti?
0.6.0'da loaderCRC/selfCRC/textCRC (runtime bütünlük) seçeneklerinden
herhangi biri + VM sanallaştırma birlikte kullanıldığında, XRT cfg
bloğunun son 8 baytı İLK sanallaştırılmış fonksiyonun bytecode'unun
ilk 8 baytını eziyordu. Belirti: program/oyun AÇILIYOR, ÇÖKMÜYOR ama
korunan fonksiyon sessizce çalışmıyordu (metin2 örneği: damage yok).

Düzeltme: cfg rezervi 0x40 → 0x48 (XCFG_SIZE_E) + "cfg yazımından
sonra bytecode bütünlüğünü diskten doğrula; taşma varsa çıktı YAZMA"
koruması. Ayrıntı: CHANGELOG_0.6.1.md

## Kanıt (kanit/ klasörü)
- metin2client_vmp_d7610_Ultra_loaderCRC_061.exe
    0.6.1 ile derlenmiş düzeltilmiş dosya. Bytecode, ÇALIŞAN
    seçeneksiz ULTRA çıktısıyla bayt-bayt aynı (md5 637c56b7...).
- fix_crc_061.log   — 0.6.1 derleme kaydı (rc=0, taşma koruması temiz)
- rep_crc_060.log   — 0.6.0 hata tekrarı (DBG3 doğru, dosyada ezik)

## Kullanım (CLI)
  XProtectUI.exe --cli <girdi> <çıktı> --add RVA:SON:U --loadercrc --filecrc
  XProtectUI.exe --cli <girdi> <çıktı> --pack            (LZMA paketleme)
GUI: normal şekilde açıp seçenekleri işaretleyin.

## Windows'ta test (metin2 senaryosu)
1. metin2client.exe'yi 0.6.1 ile ULTRA + loaderCRC derleyin
   (veya kanit/ içindeki hazır dosyayı kullanın).
2. Oyunu açıp saldırı deneyin — damage çalışmalı.
3. Dosyayı bozarsanız (1 bayt değişiklik) ilk korunan fonksiyon
   çağrısında MessageBox + ExitProcess gelmeli (bütünlük kanıtı).

## Sürüm notu
0.6.0'ın LZMA paketleme, strip fixups/debug, VM/M/U çekirdeği,
IAT deneysel katmanı aynen korunur; yalnız rezerv hatası düzeltildi.
Seçeneksiz derlemeler 0.6.0 ile BAYT-BAYT aynıdır (md5 doğrulandı).
