XProtect 0.5.0 — RUNTIME KATMANI + SDK
======================================
Surum: XProtect_0.5.0  |  Tarih: 2026-11-09  |  VMProtect 3.5.1 (universalObfusPublicSrc) referans portu

YENILIKLER (0.4.3 -> 0.5.0)
---------------------------
1) XPROTECT RUNTIME KATMANI (xrt_core.c — .vmp0'a gomulu):
   - Loader CRC (bellek butunlugu): VM+xrt motor kodu self-CRC + .text sayfa
     sayfa CRC (reloc yamalari atlanarak — ASLR uyumlu). Her korumali
     fonksiyon girisinde BIR KEZ kontrol (cfg state kilidi).
   - Hata yolu: MessageBoxA (kapatilabilir) + ExitProcess(1) / ud2 zehiri.
   - API cozumleme IAT'siz: PEB gezintisi + ihracat tablosu FNV-1a hash'i.
   - Win32 API cagrilari stdcall/Win64 ABI inline-asm sarmalayicilariyla
     (blob Linux gcc ile derlenir; hizalama kurallari korunur).
2) SDK ISARETLERI (xprotect_sdk.h):
   - XProtectBegin{Virtualization,Mutation,Ultra}()/XProtectEnd() — kaynak
     kodda bolge secimi; motor tara, otomatik korur (VMProtect davranisi).
   - Orta-fonksiyon bolgeleri: govde ret icermeyebilir (stub region sonuna
     atlar; yigin/donus adresi korunur).
3) IAT KORUMASI (deneysel): FF15/FF25 cagri noktalari thunk'a cevrilir;
   thunk hash ile cozumler, .vmp0 cache'ler. Cozumlenemezse orijinal IAT
   yuvasindan okur (loader degeri) — her durumda calisir. ARACTA: cpImportProtection.
4) GUI: "File CRC (disk)" secenek satiri (cpFileCRC); CLI: --loadercrc,
   --selfcrc, --textcrc, --filecrc, --rt-nomsg, --rt-noexit, --iat, --nosdk,
   --sdkonly.
5) VM duzeltmeleri:
   - x64 pushmap tablosu .rodata+memcpy'den ARITMETIGE cevrildi (blob PIC
     olmaktan cikmisti -> 0x103000 mutlak okuma crash'i).
   - VM blob uretimi PIC kapisi: yalniz .text bolumu (rodata=data=HATA).
   - x86 PEB/BaseDllName ofsetleri (LDPTR, +0x2C) duzeltildi.

KANITLANMIS (wine64 + Unicorn):
- x64: SDK V/M/U bolgeleri + Loader CRC -> test exe exit=42 (tam dogru cikti)
- x86: M-modu diferansiyel EAX/ESP/bellek TAM ESLESME (Unicorn)
- SELFTEST OK (funcs=542)
- metin2client V/M/U: uc mod ciktilari FARKLI (md5), rc=0

BILINEN SINIRLAR (0.5.1 plani):
- x86 V/U: bolge icinde call olan fonksiyonlar DURU RED (x86 VM natcall
  kirik; M-modu native calisir). 0.4.3'te sessiz yanlis sonuc vardi —
  artik dürüst reddedilir.
- File CRC (disk) ve IAT korumasi: DENEYSEL — altyapi Unicorn + gercek
  wine kernel32.dll uzerinde dogrulandi; wine altinda uctan uca acik
  sorun var (0.5.1'de duzeltme).
- SDK x86 V bolgeleri cagrili olamaz (yukarida).

KULLANIM
--------
GUI : XProtectUI.exe (x64) / XProtectUI32.exe (x86)
CLI : XProtectUI.exe --cli <girdi> <cikti> [--add RVA:RVA:V|M|U]
      [--sdkonly] [--loadercrc] [--filecrc] [--iat] [--rt-nomsg] [--nosdk]
SDK : #include "xprotect_sdk.h"  (paketin icinde; ornek: test_rt.c)

DERLEME (kaynaktan)
-------------------
source/XProtectUI_Source/build_xrt.sh   # xrt blob (gcc, PIC kapili)
source/XProtectUI_Source/gen_blob.py     # vm blob (gcc -DVM_WINDOWS)
source/XProtectUI_Source/build_ui.sh     # zig c++ (x64 + x86 UI)
Gereksinimler: gcc (multilib), ziglang (pip), python3

KALITE KAPISI: SELFTEST OK (wine64+Xvfb) — sifir tolerans.
