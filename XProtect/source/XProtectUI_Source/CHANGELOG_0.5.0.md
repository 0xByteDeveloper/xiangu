# XProtect 0.5.0 (2026-11-09)

## Eklenti: Runtime katmani (0.4.3 kapsam raporundaki #1 oncelik)
- **xrt_core.c**: korunmus PE'ye gomulen yerel koruma katmani (3.1KB x86 / 3.0KB x64,
  freestanding, tamamen PIC — uretim kapisi: yalniz .text).
  - PEB gezintisi (fs:0x30 / gs:0x60) + kernel32 tabani bulma — IAT bagimsiz.
  - Ihracat tablosu cozumleme FNV-1a isim hash'i ile (derleme-sabit hash'ler;
    rodata tablosu YASAK — GCC stack-string'i memcpy'ye cevirip PIC'i bozuyordu).
  - Win32 API sarmalayicilari (api1/3/4/5/7): stdcall (x86) ve Win64 ABI
    (x64: rcx/rdx/r8/r9 + golge alani + 16-bayt hizalama).
  - **Loader CRC**: (a) self-CRC — vm+xrt motor kodu birligi; (b) .text CRC —
    sayfa sayfa, .reloc yamali baytlar ATLANARAK (ASLR altinda da dogru).
  - cfg (64B) .vmp0 icinde: magic/flags/state/rva'lar/crc'ler — motor tarafinda
    Python aynasi ile dogrulandi (xrt_crc_check.py).
  - Her korumali giris BIR KEZ kontrol (state kilidi); hata: MessageBoxA +
    ExitProcess(1) (bayraklarla yonetilir).
  - Yerlesim: stub'larda ve M-govde kapilarinda "gate" — hizalama duzeltmeli
    (bolgeye E9/jmp ile giriste donus adresi yoktur; movaps icin rsp hizasi
    acikca saglanir).
- GUI: cpLoaderCRC -> self+text CRC; yeni cpFileCRC (disk, DENEYSEL) satiri.
- CLI: --loadercrc --selfcrc --textcrc --filecrc --rt-nomsg --rt-noexit.

## Eklenti: SDK isaretleri (kapsam #4)
- **xprotect_sdk.h**: XProtectBegin{Virtualization,Mutation,Ultra}()/XProtectEnd().
  Sozlesme: B8 9C37E1B5 + B8 <tip> ... B8 3D7A2C8E (mov eax, imm — calisma
  zamaninda zararsiz; memory-clobber derleyici bariyeri).
- Motor: SDK taramasi -> ManFn listesi (SDK_V_0x... isimleri) — otomatik secim.
- Bolge kipi: lifter (yapay OP_RET + stub region sonuna jmp) ve mutator
  (govde sonuna E9) icin "ret yok" destegi. VCube/Unicorn testi: cmovge'li,
  printf'li, orta-fonksiyon bolgeleri DOGRU calisiyor (exit=42).
- CLI: --sdkonly (yalniz SDK/manual bolgeleri), --nosdk.

## Eklenti: IAT korumasi (kapsam #2 — DENEYSEL)
- parse_imports + disasm tabanli guvenli site tarayici (cozulmeyen bayt =
  fonksiyon atlanir — yanlis yama riski sifir; kanit: 44/44 site dogru).
- Thunk'lar (.vmp0): hash cozumleme + cache + orijinal-IAT geriucusu.
- IAT yamalari derleme ONCESI yapilir -> FF15 goran fonksiyonlar
  sanallastirilabilir olur (VMProtect davranisi). GRP5 decode (XP_CALLI/JMPI).

## VM cekirdek duzeltmeleri
- x64 pushmap: {0,1,2,3,5..} tablosu GCC 14'te .rodata+memcpy olurdu ->
  blob mutlak 0x103000 okur -> SIGSEGV. ARITMETIK formule cevrildi.
  (Eski blob'da yok; yeniden derlemede ortaya cikti.)
- PIC kapisi: gen_blob.py / gen_xrt_blob.py — .text disi bolum = HATA.
- x86: LDPTR (isaretci yuku 4B), BaseDllName.Length +0x2C (eskiden +0x28
  yanlis alandi), Win64 sarmalayici hizalamalari (sub 0x28/0x38).
- x86 V/U + call: DURU RED (vm_natcall86 kirik — Unicorn'ta yigin bozumu;
  0.4.x'te sessiz erken-cikis yani YANLIS SONUC vardi, artik reddedilir).
  M-modu cagrilarla TAM calisir (diferansiyel kanit).

## Bilinen aciklar (0.5.1)
- filecrc + iat: wine altinda uctan uca calismiyor (motor+altyapi dogrulandi:
  Unicorn thunk zinciri + gercek kernel32.dll find_export kaniti). DENEYSEL.
- x86 vm_natcall86.

## Testler (kanit klasoru + scripts/)
- wine64: test_rt64 SDK-only V/M/U -> exit=42; +LoaderCRC -> 42;
  selfcrc=42 textcrc=42 (filecrc acik); SELFTEST OK funcs=542.
- Unicorn: x86 M diferansiyel ESLESIYOR (EAX/ESP/bellek); x86 V call=RED;
  thunk+PEB+find_export sahte ve GERCEK kernel32.dll ile BASARILI.
- metin2: V/M/U md5'leri birbirinden farkli (yeni VM blob — 0.4.3'ten
  farkli md5'ler BEKLENDI: pushmap/natcall degisiklikleri).
