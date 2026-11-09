# XProtect 0.6.0 — Değişiklik Günlüğü

## Hedef
0.6.0 yol haritası: "LZMA paketleme + kaynak koruması + IAT v2 + strip
fixups/debug + File CRC". Kullanıcı talimatı uyarınca **1 çok sağlam özellik
(birebir)** seçildi: **cpPack — LZMA paketleme**. Kaynak koruması 0.7.0'a,
IAT v2 olgunlaştırması 0.7.0'a kaydedildi (dürüst kapsam).

## Yeni: cpPack (LZMA packing)
- `lzma/` — LZMA SDK portu (LzmaEnc.c + LzFind.c encoder; LzmaDecode.cc
  decoder; public domain). Encoder `_7ZIP_ST` tek-iplik; decoder xrt
  blob'una gömülü (`#include "lzma/LzmaDecode.cc"`) — PIC kapısı temiz
  (yalnız .text; statik veri yok).
- `engine_pack.cpp` (YENİ): `lzma_compress()`, `build_packstub_x64/x86()`,
  `do_pack()` (bolum paketleme + XPK1 + EP redirect + ASLR temizliği +
  dosya compaction), `strip_fixups()`, `strip_debug()`.
- `xrt_core.c`: `xrt_unpack()` (PEB→kernel32→VirtualAlloc/Protect/Free
  FNV çözümü; LZMA decode; RW geçişli bölüm yazımı; OEP'ye dönüş),
  XCFG 0x40→0x48 (XCFG_OFF_PACKRVA), rt_fatal teşhis kodları
  (0x70+6..10 erken hatalar; +1..4 kayıt/alloc/decode/protect).
- `engine_main.cpp`: sıra KRİTİK — (1) self+text CRC PRE-pack,
  (2) pack+strip, (3) file CRC FINAL dosya + cfg yazımı (0x48).
  cfg yazım koşulu `rt_any` → `rt_any || opt.pack` (pack-only'de cfg
  şart). `text/rdata` işaretçileri add_section realloc'u sonrası
  `secbyname` ile tazelenir.
- `gen_xrt_blob.py`: xrt_unpack ofsetleri (XRT_BLOB*_UNPACK_OFF).
- `xprotect_ui.cpp`: cpPack/cpStripFixups/cpStripDebugInfo satırları
  motora bağlandı (satırlar 0.4.x'ten beri vardı, etkisizdi); CLI
  --pack/--stripfixups/--stripdebug.

## Düzeltmeler (geliştirme sırasında yakalanan)
1. **compaction sahte-başarısı**: compaction, paketlenen bölümlere akışı
   değil ORİJİNAL baytları kopyalıyordu → dosya hem compressed hem
   uncompressed içeriyordu, unpack gereksizdi (T1 ilk "42" SAHTEYDI).
   Artık akış yazılıyor.
2. **pack stub ABI**: xrt blob SysV derlenir → cfg RDI'de (lea rdi);
   RCY (Win64) idi → xrt_unpack NULL cfg ile magiç hatası veriyordu.
3. **VirtualAlloc arite**: 4 argüman (protect eksikti) → probs alloc
   başarısız (exit 122).
4. **TLS algısı**: x64 AddressOfCallBacks 8 bayt VA — 4 bayt okunup
   RVA sanılıyordu → Test.exe reddedilmiyordu. VA→RVA dönüşümü eklendi.
5. **cfg yazım koşulu**: pack-only derlemede cfg yazılmıyordu → unpack
   hiç çalışmıyordu.

## Doğrulama (wine64 + Xvfb)
- SELFTEST OK funcs=13 (final binary, sifir-tolerans kapisi)
- mini42c (x64, TLS'siz, lld-uretim): pack-only=42, pack+CRC=42
- Test.exe: TLS callback'li → rc=4 RED (çıktı YOK)
- metin2client (x86): 5,242,880 → 3,810,304 B (%27); XPK1 3 kayıt;
  python LZMA bağımsız çözümleme = uzunluk+baş/son TAM; x86 stub
  call$+5/pop/sub/add doğrulandı. Windows çalıştırma testi kullanıcıda.
- strip: Test.exe dir5/dir6=0, DllChars 0x8160→0x8100

## Sürüm damgası
- XPROTECT_VERSION 0.5.1 → 0.6.0
