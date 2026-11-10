XProtect 0.8.3 — oyun-guvenli anti-debug (0.8.2 oyun-cokmesi fix)

KULLANIM (kullanici senaryosu):
  XProtectUI32.exe (PE32 oyunlar icin) → fonksiyon sec (orn. d7610)
  → ULTRA → Check Debugger=Yes → Compile.
  - Debugger bagliyken fonksiyon CALISMAZ (donus 0) — oyun cokmeden.
  - Debugger yokken fonksiyon NORMAL calisir (0.8.2'deki cokme giderildi).

0.8.2 → 0.8.3 OZET:
  1) TF-probu yanlis-pozitifi (temiz oyunda cokme) -> GOLGE politikasi
  2) x86 NtQueryInformationProcess/NtProtectVirtualMemory handle fix
     (0xFFFFFFFE -> 0xFFFFFFFF) — x86'da DebugPort/NtPVM artik calisir
  3) TF-probu baglam-DRx (core.cc:771 1:1) — GetThreadContext'siz DRx
  4) Zehirli donus 0xDEADBEEF -> 0 (oyun-guvenli)
  5) CONTEXT tamponlari tam boyut + VM_DELTA 0x1400 + xa_module saglamlik

KANIT: kanit/veh_matrix_x86_083.txt (14/14 TAM — ilk gercek x86 runtime
dogrulamasi), kanit/veh_matrix_x64.txt (5/5), kanit/m2_083_U_ad.exe
(metin2 d7610 statik ANTIDBG kaniti).
