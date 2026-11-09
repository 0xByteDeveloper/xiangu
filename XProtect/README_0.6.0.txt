XProtect 0.6.0 — OKUMA DOSYASI
==============================

Surum: 2026-11-09 | VMProtect 3.5.1 referans portu | C++ + saf Win32 (Qt YOK)

BU SURUMDE NE VAR? (0.5.1 -> 0.6.0)
-----------------------------------
1) cpPack — LZMA PAKETLEME (birebir VMProtect altyapisi):
   - LZMA SDK (Igor Pavlov, public domain) repoya port edildi: encoder
     (LzmaEnc/LzFind, _7ZIP_ST tek-iplik) + decoder (LzmaDecode, xrt blob'una
     gomulu — PIC kapisi: yalniz .text, statik veri YOK).
   - Motor: dizin tasimayan bolumler (.text/.data/...) LZMA1+end-mark ile
     sikistirilir; .vmp1 bolumune XPK1 bloku yazilir:
       [XPK1][kayit {src_rva,dst_rva,dst_size}][props 5B][akislar][EP stub]
   - Runtime: EP -> unpack stub -> xrt_unpack (xrt_core.c):
     PEB'den kernel32 -> VirtualAlloc/VirtualProtect/VirtualFree (FNV-1a,
     IAT bagimsiz) -> LZMA coz -> bolum bellekine RW gecisiyle yaz -> OEP.
   - Dosya COMPACTION: bolumler baslarin arkasina yigilir (gercek boyut
     kazanci). ASLR bayraklari (DYNAMIC_BASE/HIGH_ENTROPY_VA) temizlenir
     (fixups yukleyici uygulanmaz; tam-loader + fixups 0.9.0 hedefi).
   - GUVENLIlK KURALLARI (durusluk — red edilirse cikti YAZILMAZ, rc=4):
     * TLS callback'leri olan dosyalar (EP oncesi calisir, crash olur;
       metin2 geciyor — callback listesi bos)
     * DLL, .NET (CLR), imzali (Authenticode) dosyalar
     * .tls/.vmp* ve import/TLS/export/reloc/debug/loadcfg/IAT/CLR dizini
       tasiyan bolumler hic paketlenmez
   - cfg sozlesmesi 0x40 -> 0x48 (pack_rva alanı eklendi).

2) cpStripFixups: .reloc dizini silinir + ASLR bayraklari temizlenir.
3) cpStripDebugInfo: debug dizini + COFF sembol tablosu cikarilir.
4) CLI: --pack, --stripfixups, --stripdebug
   GUI: "Pack the Output File" / "Strip Fixups" / "Strip Debug Information"
   satirlari artik motor tarafina bagli (0.4.x'te beri gorunuyordu).

KANITLAR (wine64, kanit/ klasoru)
---------------------------------
T1  pack-only     : mini42c.exe -> mini42c_final.exe ... exit 42 (gercek:
                    .text diskte LZMA akisi; unpack calismadan kod yok)
T2  pack+CRC      : --pack --loadercrc --filecrc ... exit 42
                    (sira: text-CRC PRE-pack, file-CRC FINAL dosya, unpack
                    en once -> uc kontrol de eslesiyor)
T3  TLS red       : Test.exe (TLS callback'li) -> rc=4, cikti YOK
T4  metin2 (x86)  : 5,242,880 -> 3,810,304 bayt (%27 kazanim)
                    .text 3,881,472 -> 1,253,601 (%32.3)
                    .data 197,120 -> 69,714 | .data1 2,560 -> 375
                    XPK1: 3 kayit; python LZMA dogrulamasi TAM (uzunluk +
                    bas/son baytlar); x86 stub: call$+5/pop/sub/add (ASLR-
                    guvenli). Windows'ta calistirma testi kullanicida.
T5  SELFTEST OK   : funcs=13 (wine64 + Xvfb; sifir tolerans kapisi)
T6  strip         : Test.exe --stripfixups --stripdebug -> dir5/dir6=0,
                    DllChars 0x8160 -> 0x8100

TESLIS EDILEBILIR SINIRLAR (durusluk)
------------------------------------
- IAT korumasi hala DENEYSEL (0.7.0'da olgunlasacak).
- Resource protection 0.6.0'dan 0.7.0'a kaydi (LZMA pack "1 cok saglam
  ozellik" olarak tam gecirildi — kullanici talimati).
- Paketli + ASLR'li calisma (fixups'li unpack) 0.9.0 tam-loader scope'u.
- wine32 yok: x86 pack calistirma testi Windows'ta kullanici tarafindan.

KULLANIM
--------
GUI : XProtectUI.exe (x64) / XProtectUI32.exe (x86)
      Project > Protection Options > "Pack the Output File" = Yes
CLI : XProtectUI.exe --cli <girdi> <cikti> [--add RVA:RVA:V|M|U]
      --pack --stripfixups --stripdebug
      --loadercrc --selfcrc --textcrc --filecrc --rt-nomsg ...
Ornek: XProtectUI.exe --cli app.exe app_vmp.exe --pack --loadercrc --filecrc

DERLEME (kaynaktan)
-------------------
source/XProtectUI_Source/build_xrt.sh   # xrt blob (gcc; LZMA decoder dahil)
source/XProtectUI_Source/build_ui.sh    # zig c++ (x64 + x86; LZMA encoder dahil)
Gereksinimler: gcc, python3 + ziglang (pip)

LISANS NOTU: lzma/ klasoru LZMA SDK (Igor Pavlov) — public domain.
XProtect egitim/arastirma amacli ornektir; VMProtect ticari urunudur.
