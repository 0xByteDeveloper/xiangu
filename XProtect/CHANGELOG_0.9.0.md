# XProtect 0.9.0-DeepVM — CHANGELOG

## Faz A: UniversalObfuscator (VMProtect 3.5.1 pub) VM derinliği portu

### Tam EFLAGS (A1)
- VM artık 9 bayrağı modeller: **CF/PF/AF/ZF/SF/OF + DF** (0.8.5: yalnız CF/ZF/SF/OF)
- **PF gerçek parite** — 0.8.5'te JP/JNP hep "not parity" döndürüyordu (sessiz yanlış-dal!)
- **AF kesin nibble-taşıyıcısı** (add/sub/adc/sbb; cin dahil)
- ADC/SBB tam zincir semantiği (0x10-0x1F + grp1 imm + acc formları)
- PUSHF/POPF gerçek EFLAGS imajı (bit1=1, IF=1); SAHF/LAHF; CLC/STC/CMC/CLD/STD
- ROL/ROR diğer bayrakları KORUR (0.8.5 hepsini sıfırlıyordu); shift'lerde OF (n==1) + PF

### Komut kapsamı (A3) — 15 yeni VM opcode
- MUL/DIV (r8/r16/r32/r64; RDX:RAX geniş formlar; #DE güvenli-sıfır)
- BT/BTS/BTR/BTC (reg + imm8 + mem hedef)
- SHLD/SHRD (imm8 + CL)
- XADD (reg + mem), IMUL r,[mem] (0F AF)
- LOOP/LOOPE/LOOPNE, JECXZ (67 prefix'li form dahil)
- REP MOVSB/STOSB/LODSB/SCASB/CMPSB (+repe/repne ZF koşulu + STD/CLD yönü)

### Adresleme (A2)
- **FS/GS segment override'ı**: x86 FS→TEB, x64 GS→TEB (vm_entry'de gerçek
  seg okuması scratch[4]'e yakalanır) — metin2 tarzı fs:[0x18] TEB erişimi artık
  sanallaştırılabilir (0.8.x bunları sessizce YANLIŞ çözüyordu)

### Kök-düzeltmeler (0.8.x'ten kalıtılan gizli buglar)
1. `66 B8 imm16` / `66 C7 imm16`: 4 yerine 2 bayt imm — akış-desync (tüm akış çöp)
2. `88/8A` byte-MOV: is8 eksik — AH/BH/CH/DH → RSP/RBP/RSI/RDI diye çözülüyordu
3. x64 IDIV: yalnız RDX bölünüyordu (RAX yok sayılıyordu) → 128/64 bit-döngüsü
4. x86 IDIV: EDX yok sayılıyordu → EDX:EAX 64/32
5. i386 freestanding `__udivmoddi4` yasağı → div64_32 bit-döngüsü
6. IMUL CF/OF artık x86 tanımlı semantiğiyle (alt-yarıya sığma)

### Mutasyon katmanı
- ud_of'a 15 yeni komutun uses/defs (0.8.5'te bunlar lifter-RED olduğu için
  mutator hiç görmüyordu; junk canlı değerleri ezebiliyordu)

### Doğrulama
- x64 wine: 13/13 test fonksiyonu birebir (V)
- x86 Unicorn (oyun mimarisi): 14/14 (V) + 14/14 (U)
- R5 ULTIMATE rc=42, tamper→117 duru-ret, SELFTEST OK
- metin2 d7610 ULTRA+antidebug statik: ANTIDBG (0x1E) doğru çözülüyor

### Bilinen açıklar (Faz B/C, sıradaki oturumlar)
- Mutator'ın yeni-komut uzunluk/fixup tablosu (t_loop U-modu duru-RED — güvenli)
- B: IR-üstü mutasyon (NOR/NAND eşdeğer-dekompozisyon), C: obfiske handler'lar
- A4: FPU (fld/fstp/fadd/fmul) — ctx genişlemesi gerekli
