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
