# XProtect 0.6.1 — Değişiklik Günlüğü

## Kritik düzeltme: XRT cfg rezerv taşması (kullanıcı kanıtlı hata)

### Belirti (kullanıcı raporu, metin2client.exe ile doğrulandı)
- `metin2client_vmp_d7610_Ultra.exe` (ULTRA, seçeneksiz) → **çalışıyor**,
  D7610 (saldırı/damage fonksiyonu) gerçekten hasar veriyor.
- Aynı derleme + **loaderCRC** (`--loadercrc --filecrc`) → oyun açılıyor,
  hesaba giriliyor, **ÇÖKME YOK ama damage ATILMIYOR** (fonksiyon sessizce
  no-op'a düşüyor).

### Kök neden (bayt-düzeyi kanıtla tespit edildi)
0.6.0'da XCFG sözleşmesi 0x40 → 0x48 bayta büyütüldü (XCFG_OFF_PACKRVA
eklendi), ANCAK motor tarafındaki rezerv `thunk_area_end = cfg_off + 0x40`
ve `secimg.resize(cfg_off + 0x40)` **0x40'ta bırakıldı**. Sonuç:

```
cfg yazım aralığı : cfg_raw + 0x48
ilk bytecode başı : tam cfg_raw + 0x40
→ ÇAKIŞMA: cfg'nin son 8 baytı (pack_rva=0 + reserved=0)
   İLK BYTECODE'UN İLK 8 BAYTININ ÜZERİNE YAZILIYOR
```

İzleme (DBG kanıtı, wine tekrar-derleme):
- Motorun şifreleme çıktısı DOĞRU: `enc16=E35483C8EC34DA76...` (plain
  `14 05 14 01 ...` = push ebp, geçerli VM opcode'ları)
- Dosyaya yazılan bc: `00000000 00000000` (ilk 8 bayt ezilmiş)
- VM plain'i runtime'da `f7 51 97 c9 ...` = ÇÖP OPCODE'lar
- Interpreter çöp akışta sessiz çıkış yolu buluyor → fonksiyon
  **çökmeden** yanlış/no-op sonuç döndürüyor (kullanıcının gözlemi)

### Neden wine self-test kaçırdı
0.6.0 doğrulamaları: mini42c **pack-only** (VM fonksiyonu YOK) ve
pack+CRC=42 (yine VM YOK). **VM(U)+loaderCRC kombinasyonu hiç
koşulmadı** — cfg taşması yalnız "xrt blob + bytecode" birlikte
bulununca etkili.

### Düzeltme (0.6.1)
1. Rezerv `XCFG_SIZE_E (0x48)` ile güncellendi (`vm_opcodes.h` içinde
   tek kaynak; `engine_main.cpp` iki noktada kullanır) — xrt_core.c
   XCFG_SIZE ile birebir sözleşme.
2. **Sahte-başarı koruması**: cfg dosyaya yazıldıktan SONRA her
   bytecode'un ilk 16 baytı diskten doğrulanır; taşma varsa
   `[HATA] XRT cfg yazımı bytecode üzerine taştı` + rc=5 ile ÇIKTI
   YAZILMAZ (0.5.x sahte-başarı koruması geleneği).

## Doğrulama kanıtları (wine64 + Xvfb, 2026-11-09)
1. **REGRESYON**: 0.6.1 seçeneksiz metin2 ULTRA derlemesi =
   `6cc9efefee06132f23ecd28339f563ae` — kullanıcının ÇALIŞAN baseline
   dosyasıyla md5 BİREBİR.
2. **DÜZELTME**: 0.6.1 `--loadercrc --filecrc` derlemesinde bc ilk 16B =
   `e35483c8ec34da76449276e9d6058d20` = çalışan baseline bytecode'uyla
   **bayt-bayt aynı** (md5 637c56b7...). VM artık baseline ile aynı
   kodu çalıştırır → damage geri gelir.
3. **SELFTEST OK** (x64, Xvfb): `SELFTEST OK funcs=0 proj=0 lics=0`
4. **PACK**: mini42c pack+CRC — XPK1 akışı bağımsız Python LZMA
   (FORMAT_RAW, lc=3/lp=0/pb=2/dict=0x10000) ile çözüldü, orijinal
   .text ile birebir.
5. **CFG CRC**: file CRC (0x126E0556) ve self CRC (0x628CC283) bağımsız
   yeniden hesaplandı → eşleşiyor.

## Sürüm damgası
- XPROTECT_VERSION 0.6.0 → 0.6.1
