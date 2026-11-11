# XProtect 1.0.0-UO — UniversalObfuscator birebir kapı katmanı + oyun-çökmesi kök-düzeltmesi

**Sürüm kararı:** 0.9.5-DeepVM ve 0.10.0-FazC **geri çekildi** (kullanıcı kanıtı:
korunan oyun fonksiyonu çalışınca oyun çöküyor). Bu sürüm **0.9.0 tabanına**
döner ve UniversalObfuscator'ın (VMProtect pub1) gerçek derleme şemalarını
**birebir** port eder — ama 0.9.5/0.10.0'ın tehlikeli bulguları olmadan.

## Kök-neden analizi: 0.9.5/0.10.0 neden oyunu çökertti?

Kanıtlı zincir (bu sürümün geliştirme kanıtı, kanit_100/):

1. **Bölge-siniri kesigi:** `--add D7610:D766C` gibi manuel aralık son
   `ret 8` talimatını (0xD766C) **diyarıda** bırakır. Lifter izi `pop ebp`'de
   biter — bytecode **RET'siz** kalır.
2. VM yorumlayıcısı akış sonunu kontrol etmez: `pop ebp` sonrası `bc[bc_len]`
   **OOB baytını** opcode diye çözer (`itab[çöp]` = stack-OOB okuma) ve
   rastgele bir VM op'u çalıştırır.
3. 0.9.0'da OOB baytı benign çözüldü (şans). 0.9.5/0.10.0'da IR-junk + kapı
   zincirleri bytecode boyutunu değiştirdi → **OOB bayt değişti** → çöp op →
   oyun çökmesi. Kapılar asla suçlu değildi; kesik akış her zaman hastaydı.

**1.0.0 düzeltmeleri:**
- **Lifter:** akış sonu `OP_RET` ile **garantilenir** (son op RET değilse yapay
  RET eklenir; stub retimm ile temiz döner).
- **Yorumlayıcı:** `ic >= bc_len` → **duru-çıkış** (OOB okuma imkânsız; yanlış
  değer mümkün ama çökme asla).

## Yeni: cmNor/cmNand yığın-kapıları (intel.cc birebir)

VMProtect VM'i yığın-makinesidir: `cmAnd/cmOr/cmXor` doğrudan komut DEĞİLDİR,
hepsi `cmNor/cmNand` kapılarından geçer (intel.cc `CompileToVM` case
cmAnd/cmOr/cmXor + AddRegistrAndValueSection/OrValueSection). 1.0.0 bu şemayı
XProtect register-VM'ine port eder:

- `OP_NOR_ST=0x3B` / `OP_NAND_ST=0x3C`: pop b, pop a, push `~(a|b)` / `~(a&b)`
  (sz-maskeli; mantık-bayrakları yerel op ile bayt-bayt aynı: CF=OF=0,
  SF/ZF/PF=sonuç).
- `OP_POP_RP=0x3D`: kismi-yazım pop (`cmPop otRegistr size_` karşılığı) —
  sz<8'de üst baytlar korunur (x64 `and eax,ebx` üst-32-koruma semantiği).
- `VM_OPN 0x3B→0x3E` (perm-tablosu genişledi — bytecode bilinçli kırılım).
- Dekompozisyon (lifter, tüm sz 1/2/4/8; reg-reg + reg-imm):
  - AND: `inv(b); inv(a); NOR` → a&b (cmAnd dal-1)
  - OR: `inv(a); inv(b); NAND` → a|b (cmOr dal-2)
  - XOR: `inv(b); inv(a); NOR; push b; push a; NOR; NOR` → a^b (cmXor dal-1)
  - `inv(x)` = `[PUSH x][PUSH x][NOR_ST]` → ~x (VMP'nin çift-push kapı öngörüsü)
- **0.9.5/0.10.0 dersi uygulandı:** temp-reg havuzu (r9/r10/r11) YOK, IR-junk
  YOK, nonce YOK. Kapılar saf yığın ops: temp'siz, dengeli (2 push + kapı +
  1 pop), düğüm-sınırı içinde — JCC hedefleri op-sınırlarıdır; zincire
  atlanırsa tam yürütülür, atlanırsa hiç başlamaz. Yığın/RSP dengesi her iki
  durumda da bozulmaz.
- mem-formlu and/or/xor yerel ALU ile (PUSH_M/POP_M gerekirdi — konservatif
  kaplama; semantik zaten aynı).

## Kanıtlar (kanit_100/, regress_100.sh PASS=14 FAIL=0)

| Test | Sonuç |
| --- | --- |
| T1 SELFTEST | SELFTEST OK |
| T2 x64 test_deepvm64 V-korumalı | **bayt-bayt** (6 yeni GATE testi dahil) |
| T3 x86 Unicorn diferansiyeli (oyun mimarisi PE32) | V 14/14 + U 14/14 eşleşme |
| T4 R5 ULTIMATE (sdkonly+pack+sectcrypt+loadercrc+filecrc+stripfixups) | rc=42 "OK hepsi doğru" |
| T5 tamper (.vmp1 tek-bayt) | rc=117 duru-ret |
| T6 md5 determinizmi | aynı girdi → aynı çıktı |
| T7 metin2 d7610 ULTRA+antidebug | KAYIT + ANTIDBG 0x1E + **bc-census uçtan-uca** |

**GATE testleri (yeni):** t_gate32 (and/or/xor sz4 + jz/js/jp bayrak-tüketimi),
t_gate816 (sz1/sz2), t_gate_zr (xor r,r + ZF), t_gate_loop (geri-dalış
**doğrudan zincir başına** — yığın-dengesi kaniti), t_gate_stack (misafir
yığın basıncı altında kapı zinciri — transient yazmalar misafir push'larını
ezmemeli), t_gate_imm (sz1/2/4 sınır-imm: 0x80/0x8000/0x80000000).

**bc-census (bc_census_100.py — yeni araç):** korumalı PE'nin VM bytecode'unu
CFB+perm ile çözer, op-uzunluklarıyla **uçtan-uca yürür**. Metin2 d7610:
`bc_len=215, 51 op, son-op=RET, NOR_ST=5 POP_RP=1` — kapılar gerçek oyun
fonksiyonunda aktif ve yapı sağlam. (0.9.0 referansı: bc_len=194, son-op
POP_R = kesik akış → kök-nedenin bağımsız doğrulaması.)

## Sürüm notları

- 0.9.5/0.10.0 özellikleri (IR-mutasyon, build-nonce, IR-junk) **geri alındı**;
  bu sürümde yoklar. NOR/NAND kapıları onların yerine, kanıtlanabilir-güvenli
  şemayla geldi.
- Windows debugger + VEH debugger koruması (K1/K2/K3) 0.8.5'ten beri
  değişmedi — 0.9.0 kullanıcı kanıtıyla yerinde.
- UI versiyon göstergesi 0.8.5'ten beri güncellenmemişti → 1.0.0 olarak düzeltildi.
