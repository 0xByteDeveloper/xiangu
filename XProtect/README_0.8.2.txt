XProtect 0.8.2 (Ultimate) — 2026-11-10
========================================

BU SÜRÜMDEKİ BAŞLIK: CE VEH debugger yakalama
----------------------------------------------
0.8.1'de Cheat Engine "Windows debugger" F5 fonksiyonu durduruyordu ama
"VEH debugger" durdurmuyordu. 0.8.2 VMProtect'in (UniversalObfuscator
public kaynağı) loader anti-debug paketini 1:1 port eder:

1) PEB.BeingDebugged + NtGlobalFlag        (0.8.1'de vardı)
2) ProcessDebugPort + DebugObjectHandle    (gerçek debugger)
3) GetThreadContext DR0-DR3                (CE VEH donanım-bp)
4) Kapı kontrolü: fonksiyon girişi 0xCC    (CE VEH yazılım-bp — XProtect'e
                                            özgü, orijinalde yok)
5) ntdll/kernel32 kritik API girişleri 0xCC taraması (loader.cc 1:1)
6) NtProtectVirtualMemory PAGE_GUARD/NOACCESS (loader.cc 1:1)
7) TF + RDTSC problaması (kuyruk-VEH)      (tek-adım yutan debugger —
                                            CE VEH'in asıl yakalandığı yer)

KULLANIM (kullanıcı senaryosu değişmedi):
XProtectUI → fonksiyon seç → ULTRA → Check Debugger=Yes → Compile
CE'de Windows debugger VEYA VEH debugger ile F5 → korunan fonksiyon artık
İKİSİNDE DE durur (zehirli dönüş 0xDEADBEEF + süreç sonlanır).

Komut satırı:
  XProtectUI.exe --cli <giris.exe> <cikti.exe> --add RVA:SON:U --antidebug
  (+ --strenc --memprot --pack --sectcrypt --loadercrc --filecrc --stripfixups)

KANITLAR (kanit/ dizini):
  veh_matrix_x64.txt       5-modlu CE-VEH simülatör diferansiyel matrisi
  static_ad_check.py       statik bytecode çözümleyici (stub→bc→itab)
  m2_082f_U_ad.exe         metin2client d7610 U+antidebg derlemesi
  ad_sonuc.txt / str_sonuc.txt

DEĞİŞİKLİKLER (0.8.1 → 0.8.2):
+ vm_core.c: VMProtect anti-debug bataryası (x64 ms_abi + x86 stdcall VEH
  işleyicisi, PC-göreli işleyici adresi, PEB-gezgini + FNV-1a export)
+ vm_opcodes.h: VM_DELTA64/86 0x600→0xC00 (derin C çağrı zinciri emüle
  yığını ezmesin)
+ scratch[3] u32 cast (stub 4 bayt yazar — üst 32 bit yığın çöpü)
+ x86 NTSTATUS maskesi düzeltmesi
+ DebugObjectHandle yanlış-pozitif koruması (orijinalin `|| 0` tuhaflığı
  alınmadı)
+ Sürüm: 0.7.0 → 0.8.2 (About/başlık)

SINIRLAR (değişmedi):
- x86 PE32 çalıştırma kanıtı wine ortamında üretilemez (syswow64 yok);
  statik kanıt + x64 runtime kanıtı geçerli
- hw-bp wine'da DR round-trip yapmadığı için (D) DRx kontrolü kod-düzeyinde
  doğrulanmıştır (winnt.h ofsetleri birebir)

LİSANS/SDK: 0.7.0'daki gibi (SDK 40 API, RSA lisans, HWID, filigran).

XProtect Ultimate — VMProtect tarzı koruma (C++ + Win32, saf port)
