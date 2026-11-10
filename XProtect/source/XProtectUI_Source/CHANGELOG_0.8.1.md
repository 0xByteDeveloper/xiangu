# XProtect 0.8.1 — Anti-Debug Regresyon Düzeltmesi + STRDEC Tamiri

## Özet
0.8.0'da kullanıcı tarafından bildirilen **anti-debug ölümü** (Cheat Engine F5 ile
fonksiyon debug edilirken çalışmaya devam ediyordu; 0.6.2/0.7.0'da duruyordu)
kök nedenine kadar bulundu, iki ayrı encoder hatası olarak kanıtlandı ve
düzeltildi. 0.8.0 bir "downgrade" değildi — 0.8.0'da eklenen polimorfik opcode
permütasyonunun encoder tarafındaki iki kör noktası, anti-debug dahil tüm
ham-push edilen op'ları etkisizleştiriyordu.

## 1) KRİTİK: Anti-debug ölümü (ham opcode push'u)
**Belirti:** `Check Debugger=Yes` ile derlenen ULTRA fonksiyon, CE/winedbg
altında çalışmaya devam ediyordu.

**Kök neden (kullanıcının kendi 0.8.0 çıktısından kanıtlandı):**
- 0.8.0 fonksiyon-bazlı opcode permütasyonu ekledi (tohum = fonksiyon RVA'sı;
  interpreter `op = itab[sf8()]` ile her opcode baytını tablodan geçiriyor).
- Lifter tüm op'ları `eop()` (perm-tablosundan) basarken, **Options motoru**
  ANTIDBG/STRDEC op'larını `plain.push_back(OP_x)` ile **ham** basıyordu.
- Kullanıcının `metin2client_vmp.exe`'si (d7610 U): `bc[0]=0x1E` → interpreter
  `itab[0x1E]=0x1C=CQO` çözüyordu → **kontrol hiç çalışmıyordu** (CQO zararsız).
- 0.7.0'da perm olmadığı için ham 0x1E == ANTIDBG idi ve çalışıyordu.

**Düzeltme:** Yayım artık lifter ile AYNI perm tablosunu kullanıyor
(`vm_perm_tab(f.rva, ptab, 0)` → `ptab[OP_ANTIDBG]`).

**Kanıt (wine + winedbg, 3-yönlü diferansiyel):**
| Test | Yapı | Debugger | r | bd |
|---|---|---|---|---|
| T1 | antidebug'lı | yok | 3812 | 0 |
| T2 | antidebug'lı | winedbg | **0xDEADBEEF (zehir)** | 1 → DURDU |
| T3 | antidebug'sız | winedbg | 3812 | 1 (VM normal) |

metin2 d7610 U statik: `bc[0]=0x0A → itab[0x0A]=0x1E=ANTIDBG` ✓
(0.8.0 çıktısında aynı analiz: 0x1E → 0x1C CQO — bug birebir teyit.)

## 2) KRİTİK: STRDEC format uyumsuzluğu (0.3.0'dan beri latent)
**Kök neden:** VM handler 4×u32 okuyordu (strRva, stateRva, key, len); encoder
yalnız 2×u32 yazıyordu → bytecode akışı 8 bayt kayıyor, gövde op'ları yanlış
çözülüyor, ANTIDBG de yutuluyordu.

**Düzeltme (tam yeniden tasarım — VMProtect tarzı):**
- Encoder 4×u32 yazıyor; opcode perm tablosundan geçiyor.
- **Özel kopya şifreleme:** string'in şifreli kopyası .vmp0 içinde tutulur
  (orijinal string diske DOKUNULMAZ → M/native kodun referansları her zaman
  doğru). Lifter .rdata referans siteleri kaydediyor (str_sites); bytecode
  referansları kopyaya yönlendiriliyor (string-içi delta korunur).
- **Paylaşılan state:** state kopya başına TEK byte (aynı stringi kullanan
  ikinci fonksiyon tekrar çözmüyordu — string bozuluyordu).
- **M-bölge çakışma koruması:** kopya alanı M blokları/bc/stub'ların
  bitişiğinde, ayrı hizalı bölgede.
- .rdata artık yazılabilir yapılmıyor (runtime yazma .vmp0'da; read-only
  kalmak korumayı güçlendirir).

**Kanıt:** runtime çözümü ✓ (`str=XP-gizli-mesaj-0.8.1 ok=1 RCODE=42`),
diskte .vmp0 içinde PLAIN string YOK ✓, şifreli kopya VAR ✓.

## 3) Regresyon paketi (hepsi son derlemeyle)
- R1 anti-debug diferansiyel: T1/T2/T3 (yukarıda) ✓
- R2 metin2 d7610 U + antidebug + memprot statik ✓
- R3 strenc runtime + disk analizi ✓
- R4 tamper (paketli+şifreli akışta tek bayt) → RCODE=113 duru-ret ✓
- R5 ULTIMATE (V+M+pack+sectcrypt+loadercrc+filecrc+stripfixups) → RCODE=42 ✓
- R6 x86 (PE32) statik ANTIDBG ✓ (bc[0]=0x00 → 0x1E)
- R7 SELFTEST OK (x64 wine + Xvfb) ✓

## Notlar
- `Debug Mode` / `Check Kernel Debugger` kutuları hâlâ motor tarafına bağlı
  DEĞİL (0.6.2'den beri görüntülük); CE tespiti "Check Debugger" ile sağlanır.
  Kernel debugger tespiti 0.9.0 anti-debug paketindedir.
- Paketleme kuyruğu: XProtect_0.8.1.zip
