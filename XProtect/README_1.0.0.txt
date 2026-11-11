XProtect 1.0.0-UO — UniversalObfuscator kapı katmanı + oyun-çökmesi kök-düzeltmesi
==================================================================================

ONEM: 0.9.5 ve 0.10.0 paketlerini KULLANMAYIN (geri cekildi — korunan oyun
fonksiyonu calisinca oyunu cokertiyor). Bu surum 0.9.0 tabanina doner ve
kok-nedeni kapatir.

KOK-NUDEN (kisa):
Manuel fonksiyon araligi (--add BASE:END) son ret'i diyarida birakabiliyordu.
Bytecode RET'siz kaliyor, VM akis sonunu kontrol etmedigi icin bytecode
sonrasi OOB bayti opcode diye calistiriyordu. 0.9.0 sansliydi (OOB bayt
benzin cozuldu); 0.9.5/0.10.0'da bytecode boyutu degisince OOB bayt degisti
ve oyun coktu. 1.0.0: (1) lifter akis sonunu RET ile garantiler, (2) VM
ic >= bc_len'de duru-cikis yapar (cokme imkansiz).

YENI — UniversalObfuscator (VMProtect) birebir kapi katmani:
VMProtect VM'inde and/or/xor dogrudan komut degildir; hepsi cmNor/cmNand
yigin kapilarindan gecer. 1.0.0 ayni semayi uygular:
  - OP_NOR_ST / OP_NAND_ST / OP_POP_RP (yigin kapilari, sz-maskeli)
  - AND -> inv+inv+NOR | OR -> inv+inv+NAND | XOR -> 7-adim Nor zinciri
  - Tum boylar (1/2/4/8), reg-reg ve reg-imm formlari
  - Temp-register YOK, IR-junk YOK, nonce YOK (0.9.5/0.10.0 dersleri)
  - Bayraklar yerel and/or/xor ile bayt-bayt ayni (CF=OF=0; SF/ZF/PF=sonuc)

KANIT (regress_100.sh — PASS=14 FAIL=0):
  T1  SELFTEST OK
  T2  x64 test_deepvm64 V-korumali: BAYT-BAYT eslesme (6 yeni GATE testi)
  T3  x86 Unicorn diferansiyeli (oyun mimarisi): V ve U modu TAM eslesme
  T4  R5 ULTIMATE (pack+sectcrypt+loadercrc+filecrc+stripfixups): rc=42
  T5  tamper: rc=117 duru-ret
  T6  md5 determinizmi: ayni girdi -> ayni cikti
  T7  metin2 d7610 ULTRA+antidebug: KAYIT + ANTIDBG 0x1E +
      bc-census UCTAN-UCA (bc_len=215, 51 op, son-op=RET, NOR_ST=5)
      -> kapilar gercek oyun fonksiyonunda aktif, yapi saglam

KULLANIM (CLI):
  XProtectUI.exe --cli <giris.exe> <cikis.exe> --type V --all
  XProtectUI.exe --cli <giris.exe> <cikis.exe> --add D7610:D7670:V --antidebug
  (ipucu: bolge sonunu son ret'in OTESINE koyun, orn. D7670 — 1.0.0 zaten
   kesigi guvenli yapiyor ama tam govde her zaman en iyisi)

TAM KAPASITE MATRISI (0.8.5'ten beri degismeyen taban + 1.0.0 kapilari):
  [OK] ULTRA sanallastirma (V) — tam EFLAGS (CF/PF/AF/ZF/SF/OF+DF), 15+
       VM op (MUL/DIV 128-bit, BT ailesi, SHLD/SHRD, XADD, LOOP/JECXZ,
       REP STR, FS/GS TEB) + NOR/NAND kapi dekompozisyonu
  [OK] Mutation (M) + Ultra (U) modlari
  [OK] Anti-debug: K1 (giris 0xCC) + K2 (TF) + K3 (DRx hwbp — CE VEH)
  [OK] Pack (XPK1/LZMA), SectionCryptor (CFB-64), LoaderCRC, FileCRC,
       StripFixups, StripDebugInfo
  [OK] RSA lisans + HWID + watermark + SDK-40
  [OK] Polimorfik opcode perm (Fisher-Yates, seed=RVA) + crypt_command
       (iki-katmanli keystream + CFB) + crypt_key
  [OK] x86 PE32 + x64 PE32+ (oyun mimarisi Unicorn ile fark-dozede)

0.9.5/0.10.0'DAN FARK:
  - IR-katmani mutasyonu YOK (geri alindi)
  - B4 build-nonce YOK (geri alindi)
  - r9/r10/r11 host-reg havuzu YOK (yigin kapilari temp'siz)
  - Akis-sonu RET garantisi + VM ic-sinir kontrolu VAR (yeni)

XProtectUI.exe (x64) / XProtectUI32.exe (x86) — koruyucu arayuz.
kaynak/: tam motor kaynagi (engine_*.cpp, vm_core.c, vm_opcodes.h...).
kanit_100/: regress_100.sh ciktilari + bc_census_100.py araci.
