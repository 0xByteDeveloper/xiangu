# Worklog_2 — XProtect Projesi (devam kaydi)

> Bu dosya, worklog.md (Worklog 1) tasdigi icin acildi (54KB / ~900 satir).
> Worklog 1'in ozeti asagida; bu oturumdan itibaren kayitlar BURAYA yazilir.

## Worklog 1 ozeti (Task 1-19, surum gezbisi)

| Task | Surum | Oz |
| --- | --- | --- |
| 1-4 | pre-0.1 | VMProtect 3.5.1 kaynak analizi; Test.exe sanallastirma zinciri (zig); wine kurulumu; 13 kok-hata (BUGFIX #1-#13: retimm u16, .reloc ezme, x86 64-bit key, SIB, pushmap...) |
| 5-9 | 0.1.0-0.3.0 | Win32 UI portu, Config_info, mutasyon cekirdegi (V/M/U tip sevkı, liveness, ikame+JMP) |
| 10-14 | 0.4.0-0.4.3 | UI tam kapsam (PDBScope, Manual Address), ULTRA manuel gercek, beyaz-ekran fix, sanal listview, Mutation x86 allow_call fix |
| 15-17 | 0.5.0-0.5.1 | XRT runtime katmani (PEB gezinme + FNV-1a + Loader CRC), SDK isaretleri, IAT deneysel, PIC duzeltmeleri; pipeline regresyon fix |
| 17b | 0.6.0-0.6.2 | LZMA pack (XPK1), cpStripFixups/StripDebugInfo; XCFG rezerv tasmasi fix (#14) |
| 17c | 0.7.0 | RSA lisans + HWID + watermark + SDK-40 API + #2 loaderCRC-stripFixups fix |
| 18 | 0.8.0 | VM derinlestirme: CFB crypt_command/key, polimorfik opcode perm (Fisher-Yates seed=RVA), SEH kaydi, SectionCryptor; ANTIDBG opcode YENI (perm bug'li) |
| 18b | 0.8.1 | KRITIK fix: ANTIDBG/STRDEC ham push'lari perm tablosundan gecirme; STRDEC format birlestirme. Windows debugger karsi koruma dogrulandi (kullanici: "0.8.1 calisiyor") |
| 18c | 0.8.2/0.8.3 | CE VEH yakalama denemeleri: tam batarya (QIP/guard/ntdll-CC/TF-probe) — BASARISIZ: x86 CONTEXT ofsetleri yanlis (EFLAGS=0xB8!), TF-probe dispatch carpismasi, wine yigin-baglam kaybi -> temiz oyun coktu |
| 19 | 0.8.4 | 0.8.1 tabani + SADECE K1 (giris 0xCC) + K2 (TF bayragi). 0.8.2/0.8.3 bataryasi ATILDI. Kullanici dogruladi: temiz oyun ✓, Windows debugger ✓, VEH debugger ✗ (hwbp yakalanmiyor) |

Kalici dersler (Worklog 1'den):
- symlinks TAM YASAKLI (bind-mount kullan); Xvfb ayni komut icinde baslat
- worklog'un REPO kopyasinda PAT maskele (GitHub secret-scanner)
- encoder/interpreter format eslesmesi (perm tablosu, STRDEC 2xu32) — bayt-bayt kanitla
- temiz-oyun guvenligi: istisna-dispatch devreye sokan anti-debug kontrolleri yorumlayici cerceveleriyle carpisir

---

## Task 20 — 0.8.5: CE VEH debugger hwbp yakalama (K3: DRx)
Agent: main (Z.ai) | Tarih: 2026-11-11 (oturum 9)

KULLANICI RAPORU (0.8.4 testi): temiz oyunda damage NORMAL ✓; CE Windows
debugger F5 -> damage DURDU ✓; CE VEH debugger -> damage devam ✗.
SORULAR x1-x5: UniversalObfuscator VEH'i algiliyor mu? eksikler gecirilsin mi?
VM/M/U seviye farki? hangi surum en stabil? simdi ne yapmali?

1) x1/x2 ANALIZ (upload/universalObfusPublicSrc*.zip + scripts/uosrc2):
   runtime/core.cc:764 PEB.BeingDebugged; :771 TF+__rdtsc __try/__except
   (yutulan tek-adim=debugger; istisna-baglamindan Dr0-3 okuma = VEH hwbp
   yakalayicisi); :785 NtQueryInformationProcess ProcessDebugPort/
   DebugObjectHandle (syscall yolunda "|| debug_object==0" tuhafligi
   alinmadi — temiz surecte yanlis-pozitif); :796+ kernel debugger
   (driver). loader.cc:2116 ntdll API 0xCC, :2246 PAGE_GUARD,
   :2510 TF-probu. EKSIK IMIZ: yalniz DRx/hwbp (kullanicinin senaryosu).
2) 0.8.5 TASARIM (kullanici doktrini: "sadece o kadar"):
   K3 = GetThreadContext(GetCurrentThread, CONTEXT_DEBUG_REGISTERS)
   -> DR0|DR1|DR2|DR3 != 0 -> zehir. core.cc:778 semantigi 1:1 fakat
   orijinalin istisna-baglam formu YERINE senkron API (0.8.2/0.8.3 cokme
   dersi: TF-probe + VEH yazimi dispatch ile carpisir). Yardimcilar
   xa_fnv_a/w, xa_module (DllBase-saglamlik), xa_export, xa_call4/86
   — 0.8.3'ten birebir alindi; K3 OP_ANTIDBG'ye K1/K2'den sonra eklendi
   (XProtectUI_Source + CPlusPort_Source vm_core.c). Surum 0.8.5.
3) TESTLER:
   - vm blob yeniden derlendi (vm64.bin 9564B, vmx86.bin 13634B; PIC
     kapisi temiz) + XProtectUI.exe (x64+x86) zig ile.
   - x86 Unicorn (OYUN MIMARISI PE32) 8/8 TAMAM: clean TEMIZ;
     beingdbg/ntglobal/kapicc/tf POISON; drx (DR0=0x004D7610) POISON;
     drx0 TEMIZ; drxfail TEMIZ. (unic_drx_x86.py YENI arac; sahte
     kernel32: LDR listesi + export tablosu + GetThreadContext stdcall
     taklidi — ret 8'e +4 eip popu dahil. init_rsp = STK tepesi - 0x8000:
     natcall cerceveleri icin headroom.)
   - x64 wine 8-modlu matris: clean/ceveh/gameveh TEMIZ; sw/tfstep/peb
     ZEHIR (0xDEADBEEF). DRx satirlari: wine SetThreadContext/resume
     DRx-apply FLAK (server kopyasi ARMALI, canli thread bazen 0 —
     main-verify vs worker-pre DIAG kaniti; pipe/redirect/isinma degisken
     degil). ISCI PROB UYARISI: is-parcaciginin KENDI GetThreadContext'i
     DRx durumunu bozuyor (probe'lu 0/3 vs probesuz 3/3) — harness
     probesuz busy-spin + main hw-handle verify. ARMALI yakalandiginda
     K3 ZEHIR (k3_drx_kanit.txt + worker-try1 dr0=0x3FFD9268 ->
     r=-559038737). Gercel Windows'ta apply deterministik (OS garantisi).
   - drprobe.c: wine DRx round-trip tezgahi (self-set + suspended-child
     iki yol da calisiyor — wine DRx makinesi saglam, apply-timing flak).
   - R5 ULTIMATE (V+M+U sdkonly + pack + sectcrypt + loadercrc + filecrc
     + stripfixups): RCODE=42 (a=78 b=0x16f9 c=0x1221 d=81) ✓
     NOT: zig CRT'si TLS callback ekler -> pack TLS'li RED rc=4 (0.6.0
     kurali) — test-prep'te TLS data-dir temizlendi (uygulama TLS kullanmaz).
   - R5-tamper (.vmp1 tek-bayt): RCODE=117 duru-ret ✓
   - SELFTEST OK (funcs=0 baseline) ✓
   - metin2 d7610 statik: seed=0xD7610 bc_rva=0x545548 bc_len=546
     bc[0]=0x0A->0x1E=ANTIDBG ✓ (bc_len 0.8.1 parite)
4) PAKET: XProtect_0.8.5.zip 72 dosya (md5 c3862d6ae0a83ad9a6d2e4582c4c6c59)
   -> download/XProtect/ + download/ + repo releases/. CHANGELOG_0.8.5.md
   + README_0.8.5.txt. Yeni arac: test/unic_drx_x86.py, test/drprobe.c,
   kanit_085/ (x64_wine_matris_085_final.txt + x86_unic_drx_085.txt + ...).
5) SURUM notu: CE VEH "bos bagli" hala bilincli yakalanmaz (0.8.4 dersi);
   hwbp/giris-CC/tek-adim kapali.

Stage Summary:
- 0.8.5 teslim: 0.8.4 + K3. Kullanici testi bekleniyor: temiz oyun
  (cokme YOK) -> Windows debugger F5 (DURDU) -> CE VEH debugger F5/hwbp
  (DURDU — YENI). Ceşitli senaryolar icin kapsam tablosu README_0.8.5.txt.
- Siradaki: kullanici VEH testi; 0.9.0 (memprot paketi / TLS-packer)
  kapsam karari.

---

## Task 21 — 0.9.0-DeepVM Faz A: UniversalObfuscator VM derinligi portu (Tam EFLAGS + komut kapsamı)
Agent: main (Z.ai) | Tarih: 2026-11-11 (oturum 10)

KULLANICI RAPORU (0.8.5): UC TESTI TAM — metin2 d7610 ULTRA, Check Debugger
Windows debugger + VEH debugger ikisini de yakaladi: "Check Debugger windows
debuggeri ve Veh i başarıyla algıladı teşekkürler". TALIMAT:
"şimdi UniversalObfuscator dan bunu geçirelim" (deep VM port).

1) KAYNAK TARAMASI (scripts/uobf/src1/vmp/core/intel.cc — 31.493 satir):
   VMProtect lifter'inin (CompileToVM, intel.cc:9744) mimarisi cozuldu:
   - Yigin makinesi + NOR/NAND fonksiyonel-tam primitifler
   - `if (rand() & 1)` esdeger-form dekompozisyonu (B-fazi mutasyonu lifter
     icine gomulu — fl_ sabitleri: CF=0x0001 PF=0x0004 AF=0x0010 ZF=0x0040
     SF=0x0080 OF=0x0800, regEFX'de tasiyor)
   - AddCorrectFlagSection/AddCombineFlagsSection/AddExtractFlagSection
   XProtect karsilastirmasi: 44 VM op, 4 bayrak (CF/ZF/SF/OF) — PF/AF yok
   (cond_eval CC_P hep 0 donduruyordu!), 0F uzantilari RED.
   ONCEKI OTURUMUN YARIM KALANI: engine.hpp'ye XP_* enum tanimlari eklenmis
   (02:50) ama disasm/lift/interpreter 0 satir — implementasyon bu oturumda.

2) 0.9.0-DEEPVM FAZ A IMPLEMENTASYONU (0.8.5 tabani KORUNDU, xb090 kopyasi):
   a) vm_opcodes.h: VM_OPN 0x2C -> 0x3B; 15 yeni op (PUSHF/POPF/SAHF/LAHF/
      SETFLAG/MUL_R/DIV_R/BT_X/SHLD/SHRD/LOOP/JECXZ/XADD/STR/IMUL_RM);
      FPF/FAF/FDF bayrak bitleri; MEMF_SEG/MEMF_DISP_SEG; ALU_ADC/ALU_SBB.
   b) vm_core.c: do_alu_c (cin parametreli; AF = kesin nibble tasiyicisi,
      PF = LSB-8 paritesi, ADC/SBB carry-in); cond_eval gercek parity;
      do_shift (ROL/ROR diger bayraklari KORUR — eski kod sifirliyordu;
      OF n==1; PF); PUSHF/POPF EFLAGS imaji (bit1=1, IF=1);
      TEB yakalama scratch[4] (x86 fs:[0x18] / x64 gs:[0x30]);
      KOK-FIX: IDIV x64 dd=YALNIZ RDX (RAX yok sayiliyor — cqo sonrasi
      bolum 0/cop) -> idiv128 (128/64 bit-dongu); x86 IDIV EDX:EAX ->
      div64_32; i386 freestanding __udivmoddi4 YASAK (blob linki).
   c) engine_disasm.cpp: 0x64/0x65 seg yakalama (yutulmaz oldu), F2/F3 rep
      yakalama, GRP3 MUL/DIV, 0F A3/AB/B3/BB + 0F BA (BT ailesi),
      0F A4/A5/AC/AD (SHLD/SHRD), 0F C0/C1 (XADD), E0-E3 (LOOP/JECXZ),
      9C/9D/9E/9F (PUSHF/POPF/SAHF/LAHF), F5/F8/F9/FC/FD (SETFLAG),
      A4-A7/AA-AF (string), 0F AF mem formu.
      KOK-FIXLER (0.8.x'ten kalıtılan gizli buglar):
      - 66 B8 imm16: 4 yerine 2 bayt okunmaliydi (akis desync!)
      - 66 C7 imm16 (mem+reg formu): ayni
      - 88/8A is8=true eksik: AH/BH/CH/DH (rm kod 4-7) RSP..RDI diye
        cozuluyordu (mov %sil,%%ah -> RSP'nin dusuk baytina yaziyordu!)
      - 67 E3 jecxz: hedef pc+2 yerine pc+i (onekli formda pc+3)
   d) engine_lift.cpp: yeni XP -> OP emitleri; MEMF_SEG emem'i (segmode'da
      disp HAM — TEB-relative rva'ya cevrilemez); LOOP/JECXZ fixup.
   e) engine_mut.cpp: ud_of'a yeni komutlarin uses/defs (muhafazakar):
      MUL/DIV R(0)R(2)RW(0)RW(2), BT CF+RW(ud!=0), XADD iki taraf RW,
      STR ESI/EDI/ECX/EAX, LOOP/JECXZ RCX, PUSHF/POPF/SAHF/LAHF/SETFLAG
      FFLAG. (0.8.5'te bu komutlar lifter RED oldugu icin mutator hic
      gormuyordu — default:break junk canli degeri eziyordu.)

3) TEST KANITLARI (kanit_090/):
   - test_deepvm.c (14 test fonksiyonu, ham __asm__): ADC/SBB zinciri,
     PUSHF/POPF/LAHF/SAHF, CLC/STC/CMC+JCC, MUL/DIV dword, BT ailesi
     reg+imm, SHLD/SHRD imm+CL, LOOP/JECXZ, XADD, REP movsb/stosb/scasb
     +STD/CLD, FS/GS TEB, PF/JP/JNP, IMUL mem (0F AF), 8-bit mul/div,
     izole mul/div.
   - x64 wine (V, fonksiyon-basi izole): 13/13 BIREBIR ESIT (rc=42).
   - x86 Unicorn (oyun mimarisi, unic_deepvm_x86.py YENI: korumali PE'nin
     stub+VM'ini sahte TEB/PEB ile kostirir, fs:[abs] siteleri hook'lar;
     cdecl yigin duzeltmesi): V 14/14 ESIT + U 14/14 ESIT (1 duru-RED:
     t_loop mutasyon fixup — bilinen acik, guvenli davranis).
   - R5 ULTIMATE: rc=42 "OK sdk_print 78 OK hepsi dogru" ✓
   - R5-tamper (.vmp1 tek-bayt): rc=117 duru-ret ✓
   - SELFTEST OK (Xvfb ayni komut icinde — kalici kural) ✓
   - metin2 d7610 ULTRA+antidebug statik: seed=0xD7610 bc_rva=0x547C28
     bc_len=541, bc[0]=0x03 -> itab -> 0x1E ANTIDBG ✓ (perm tablo boyutu
     degistigi icin bc byte'lar degisti — semantik ayni)

4) BILINEN ACIKLAR (Faz B/C icin):
   - mutator'un yeni-komut uzunluk/fixup tablosu (t_loop U-modu duru-RED)
   - IR-ustu mutasyon (B1-B4) ve obfiske handler (C1-C3) — UniversalObf.
     NOR/NAND dekompozisyonu lifter'a gecirilecek
   - FPU (A4): fld/fstp/fadd/fmul — ctx st[8] genislemesi gerekli
     (VMOFF/VMSIZE + stub sabitleri degisir) — sonraki oturum

5) NOT: wine x86 calistirma hala imkansiz (wine-root wow64'suz; Debian
   wow64 sessiz oluyor) — x86 dogrulama Unicorn diferansiyeliyle (kanit).
   /tmp/my-project (wine-root + prefix) sandbox resetinde SAG KALDI.
