XProtect 0.3.0 — C++ Koruyucu Motor (VMProtect 3.5.1 port projesi)
===================================================================

XProtect, VMProtect 3.5.1 kaynaklarindan (universalObfusPublicSrc)
portlanan, saf C++ koruma motoru ve Win32 arayuzudur. Bu surum
0.3.0 "MUTASYON CEKIRDEGI" katmanini ekler.

SURUM 0.3.0 YENILIKLERI
-----------------------
1. MUTASYON TIPI (M) — VMProtect "Mutation" davranisi:
   Fonksiyon makine kodunda KALIR (VM yok, tam native hiz); govde
   talimat ikamesi + calisan junk + JMP kapilari ile yeniden uretilir
   ve .vmp0 bolumune tasinir. Orijinal girise E9 kapisi birakilir.
2. ULTRA TIPI (U) — Mutasyon + Sanallastirma zinciri:
   Fonksiyon once mutasyona ugrar, SONRA mutasyonlu akis VM bayt-koduna
   cevrilir (M+V, VMProtect "Ultra" davranisi).
3. Canli-register analizi (liveness):
   Temel-blok CFG uzerinde 16 GPR + FLAGS sozu-register'i ile geriye
   dogru veri-akisi. Junk yalnizca KANITLANMIS olumlu (dead) ve VOLATILE
   register'lara yazilir; bayrak yazan junk yalniz bayraklar oluyken.
4. Talimat ikamesi katalogu (bayrak-semaforu-guvenli):
   add/sub<->lea, add<->sub (CF ters donumu liveness ile kanitlanir),
   mov r,imm -> push imm/pop r, xor r,r -> sub r,r, test r,r -> or r,r,
   inc/dec -> lea/add, not -> xor r,-1, mov r1,r2 -> push/pop.
5. JMP kapilari: EB rel8 + atlanan junk (anti-disasm tuzak bolgeleri),
   zincirli varyantlar; tum uretilen formlar lifter-guvenlidir (U ayni
   akistan beslenir).
6. VM NATCALL duzeltmesi (kritik): vm_natcall RSP kaydini r10'dan
   (volatile, callee ezer!) rbx'e (callee-saved) tasidi. Onceki latent
   hata: mutasyonlu govde r10 kullaninca VM yigin isaretcisi corba
   oluyordu. vm64.bin/vmx86.bin + vm_blob.cpp yeniden uretildi.
7. XProtectUI.exe --cli modu:
   XProtectUI.exe --cli <girdi> <cikti> [--type V|M|U] [--strength 1..3]
       [--all | --funcs a,b | --func Isim:TIP ...]
       [--antidebug] [--strenc] [--memprot] [--novmenc]
   Otomatik test/betik kullanimi; log stdout + <cikti>.log.
8. UI: Add Function dialogundaki V/M/U derleme tipi artik motora
   gercekten aktarilir (fonksiyon bazinda; VMProtect davranisi).
9. Dogrulama altyapisi:
   - verify_mut.py: Unicorn tam-goruntu diferansiyel dogrulayici
     (orijinal vs korumali PE; rax + yazilan bellek karsilastirmasi;
     x64 VE x86 — wine32'nin konteynerde imkansiz olmasina cozum).
   - muttest.cpp: Linux CLI test kosumu (motoru wine'siz kosturur).

DOGRULAMA (tolerans yok standardi)
----------------------------------
- Unicorn diferansiyeli: x64+x86, M/U/V: 5/5 fonksiyon ESIT (her
  fonksiyon 1-5 arguman setiyle; rax + bellek yazmalari birebir).
- wine64 uçtan uca: Test.exe -> U/M/V -> MiniPac oyun ciktisi orijinalle
  BIREBIR (7 komutluk oturum; skor/labirent/can akisi ayni).
- ASan+UBSan: 0 hata. Determinizm: ayni girdi -> ayni cikti md5.

ICERIK
------
XProtectUI.exe / XProtectUI32.exe   — ana Win32 arayuz (x64/x86)
CPlusPort.exe / CPlusPort32.exe     — mini Win32 GUI (eski arayuz, yeni motor)
Test.exe / Test32.exe               — MiniPac test oyunu (orijinal)
Test_vmp.exe / Test32_vmp.exe       — ULTRA (M+V) korumali ornek cikti
VMPHelper.exe                       — canli surec inceleme araci
XProtectUI_Source/                  — UI + motor kaynaklari (build_ui.sh)
CPlusPort_Source/                   — motor + blob kaynaklari (vm_core.c GUNCEL)
Test_exe_Source/                    — oyun kaynagi
VMPHelper_Source/                   — helper kaynagi
tools/                              — wine kosum yardimcilari (sandbox)

HIZLI BASLANGIC
---------------
GUI:   XProtectUI.exe -> File > Open -> Add Function (Ins) ->
       Derleme Turu sec (V/M/U) -> Derle (F9) -> <ad>_vmp.exe
CLI:   XProtectUI.exe --cli Test.exe Test_vmp.exe --type U --all

DERLEME
-------
Gereksinim: pip install ziglang
bash XProtectUI_Source/build_ui.sh   (x64 + x86 ikilileri uretir)
VM blob yeniden uretimi: CPlusPort_Source/build_all.sh
(vm_core.c -> gcc/ld -> vm64.bin/vmx86.bin -> gen_blob.py -> vm_blob.cpp)

SINIRLAR (bilinen)
------------------
- x86 modda call iceren fonksiyonlar derlemeye girmez (yaprak modu).
- Mutasyonlu fonksiyonlarda x64 .pdata unwind kaydi gecerliligini
  yitirir (istisna isleme kapali — VMProtect davranisi).
- wine32 konteynerde calismaz (seccomp); x86 dogrulamasi Unicorn ile
  yapilir (0.1.0'dan beri yontem budur).

XProtect surum gecmisi: 0.1.0 (CEKIRDEK) -> 0.2.0 (WIN32 UI) ->
0.3.0 (MUTASYON CEKIRDEGI).
