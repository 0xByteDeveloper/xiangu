# XProtect 0.8.5 — CE VEH debugger hwbp yakalama (K3: DRx)

## Ozet
0.8.4 (kullanici dogrulamasi: temiz oyun calisiyor, CE **Windows debugger**
F5 duruyor) uzerine **yalnizca bir ozellik** eklendi: CE **VEH debugger**
donanim-kirici (hwbp) yakalamasi. 0.8.2/0.8.3'un cokme sebebi olan
istisna-dispatch makinasi HIC devreye sokulmadi.

## Kullanici raporu (0.8.4)
- metin2 d7610 ULTRA + Check Debugger Yes: temiz oyunda damage NORMAL ✓
- CE Windows debugger F5: damage DURDU ✓ (Check Debugger calisti)
- CE **VEH debugger**: damage CALISMAYA DEVAM — yakalanamadi ✗  ← bu surumde duzeltildi

## Kok neden
CE VEH debugger OS debug API'si kullanmaz: PEB.BeingDebugged=0,
DebugPort=0 — 0.8.4'un PEB yolu gormez. hwbp (F5) ise **DR0-3'te kalir**
(check debugger'in bakmadigi tek yer).

## Cozum: K3 (0.8.4'un K1/K2'sine ek)
- `GetThreadContext(GetCurrentThread(), CONTEXT_DEBUG_REGISTERS)`
  ile DR0|DR1|DR2|DR3 okunur; herhangi biri 0 degilse -> zehir
  (0xDEADBEEF donus, fonksiyon hasar vermez).
- UniversalObfuscator `runtime/core.cc:778` semantigi **1:1**
  (Dr0|Dr1|Dr2|Dr3 != 0 -> debugger) — fakat orijinalin istisna-baglamindan
  okuma formu (core.cc:771, TF-probu + `__except`) yerine **senkron API**
  ile: 0.8.2/0.8.3 dersi — TF-probu + VEH baglam-yazimi yorumlayici
  cerceveleriyle carpisir (wine'da yigit-degistirme/is-parcacigi
  senaryosunda yazim YITIRILIR -> temiz oyunda cokme). K3 istisna
  uretmez; maliyeti 1 syscall.
- Isim cozumleri PEB gezinmesi + FNV-1a (derleme sabiti hash; rodata yok,
  tam PIC). CONTEXT ofsetleri winnt.h birebir (x64 FLAGS=0x30 DR0=0x48;
  x86 FLAGS=0x00 DR0=0x04 — 0.8.2'nin bozuk EFLAGS/RAX ofsetlerinden farkli
  bolge, iki mimaride de dogru).

## Bilincli kapsam kararlari
- **DRx semantigi**: oyun KENDI gozetim noktasi icin DRx kurarsa Check
  Debugger'li fonksiyon zehirlenir — UniversalObfuscator'in da ayni
  semantigi (core.cc:778). metin2 istemcisi DRx kullanmiyor.
- **Bos VEH baglama** (kirici/tek-adim yok) bilincli yakalanmaz
  (0.8.4'teki gibi — 0.8.2'nin temiz-oyun FP yuzeyi).
- QIP, guard-page, ntdll-0xCC taramalari yine YOK (0.8.2 FP yuzeyleri).

## Kanitlar (kanit_085/)
- x86 Unicorn (oyun mimarisi PE32) **8/8**: clean TEMIZ;
  beingdbg/ntglobal/kapicc/tf POISON; **drx (DR0 set) POISON**;
  drx0 (DR0=0) TEMIZ; drxfail (GTC basarisiz) TEMIZ.
- x64 wine 8-modlu matris: clean/ceveh/gameveh TEMIZ;
  sw/tfstep/peb ZEHIR. DRx satirlari: wine'in SetThreadContext/resume
  DRx-apply emulasyonu flak (server kopyasi ARMALI gorunur, canli thread
  bazen 0 gorur) — ARMALI yakalandiginda K3 ZEHIR uretir
  (k3_drx_kanit.txt: worker-try1 dr0=0x3FFD9268 -> r=-559038737).
  Gercel Windows'ta debugger askiya-alinmis thread'e DRx kurar; OS
  deterministik uygular (CE/x64dbg bunun uzerine kurulu).
- R5 ULTIMATE (V+M+U + pack + sectcrypt + loadercrc + filecrc +
  stripfixups): **RCODE=42** (a=78 b=0x16f9 c=0x1221 d=81).
- R5-tamper (paketli cikti tek-bayt): **RCODE=117** duru-ret.
- **SELFTEST OK** (funcs=0 baseline).
- metin2 d7610 statik: seed=0xD7610 bc_len=546 bc[0]=0x0A->0x1E=ANTIDBG
  (0.8.1 ciktisiyla bc_len parite).

## Degisen dosyalar
- `vm_core.c` (XProtectUI_Source + CPlusPort_Source): xa_fnv_a/fnv_w,
  xa_module, xa_export, xa_call4/xa_call2_86 yardimcilari + OP_ANTIDBG
  icine K3 blogu (K1/K2'den sonra).
- `xprotect_ui.cpp`: surum 0.8.5.
