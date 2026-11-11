XProtect 0.8.5 — CE VEH debugger hwbp yakalama (K3)
====================================================

0.8.4 TEK BASINA EK: CE VEH debugger'in donanim-kiricisi (F5/hwbp)
artik yakalanir. Diger hicbir davranis degismedi (0.8.1/0.8.4 ayni).

DEGISIM (K3):
  GetThreadContext(GetCurrentThread) ile DR0-3 okunur; DR0|DR1|DR2|DR3
  != 0 -> zehir (fonksiyon 0xDEADBEEF dondurur). UniversalObfuscator
  core.cc:778 semantigi 1:1 — ama istisna-dispatch'siz (0.8.2/0.8.3
  cokme dersi): tek maliyet 1 syscall.

KULLANIM: 0.8.1/0.8.4 ile birebir ayni:
  1) Fonksiyonu ekle (orn. 0xd7610), Koruma = Ultra
  2) metin2client.exe koruma ozellikleri: Check Debugger = Yes
  3) Compile

CE KAPSAM TABLOSU (0.8.5):
  Windows debugger + F5 (swbp)          -> YAKALANIR (PEB + K1)
  Windows debugger + hwbp               -> YAKALANIR (PEB + K3)
  VEH debugger + F5/hwbp                -> YAKALANIR (K3 — YENI)
  VEH debugger + girise 0xCC            -> YAKALANIR (K1)
  VEH/Windows debugger tek-adim izleme  -> YAKALANIR (K2)
  VEH debugger bos bagli (bp/izleme yok)-> YAKALANMAZ (bilincli)

NOTLAR:
- DRx semantigi: DR0-3'te gecerli her gozetim noktasi debugger sayilir
  (oyunun kendi DRx'i varsa fonksiyon zehirlenir — UniversalObfuscator
  ayni sekilde). metin2 istemcisi DRx kullanmiyor.
- Temiz oyunda K3'un maliyeti: korunan fonksiyon basina 1 syscall.

KANIT (kanit_085/):
  x86 Unicorn 8/8 (oyun mimarisi): drx->ZEHIR, drx0->TEMIZ, drxfail->TEMIZ
  x64 wine matris: clean/ceveh/gameveh TEMIZ; sw/tfstep/peb ZEHIR
  R5 ULTIMATE RCODE=42 | tamper RCODE=117 | SELFTEST OK
  metin2 d7610: bc_len=546, bc[0]=0x0A->0x1E=ANTIDBG
