# XProtect 0.8.0 — VM Derinleştirme (CFB + Polimorfik + SEH + SectionCryptor + Komut Genişletme)

**Tarih:** 2026-11-10 · **Kapı:** wine self-test `SELFTEST OK` + kanıt matrisi `RCODE=42`

0.8.0, 0.7.0-final'in üzerine VM/tersmjene dayanıklılık katmanlarını ekler:

## 1) crypt_command / crypt_key — CFB şifre-geribesleme (0.8.0 çekirdeği)
- Bytecode baytları artık **CFB-64** ile şifreli: her baytın akışı bir önceki
  **şifreli** bayta bağlı (saf `(key, prev, pos)` fonksiyonu).
- JMP/JCC konum değişikliği zinciri bozmaz (O(1) atlama, sınır bilgisi gerekmez).
- **Tek bayt tamper → sonraki TÜM akış bozulur** (yayılım kanıtı: bc+0 tek bayt
  → yanlış sonuç + hızlı `ExitProcess`; pack-akışında tek bayt → `rt_fatal`).
- `crypt_key`: stub artık düz anahtar taşımaz — `key^K2` yazar; `K2 =
  KEYXOR(bc_rva, bc_len)` purely fonksiyonu (splitmix64 finalizer). `vm_entry`
  girişte `key ^= K2` ile geri çözer.
- Stub ayrıca `(bc_rva, bc_len)`'i doğrudan taşımaz (rel32 hesap + K2).

## 2) Polimorfik işleyiciler — anahtar-türetilmiş opcode permütasyonu
- Her korumalı bölge için `π = Fisher-Yates(seed = bölge RVA'si)`; encoder
  gerçek→raw, interpreter raw→gerçek (AYNI makro, tablo taşınmaz).
- Stub tohumu `ctx->scratch[3]`'e yazar (`disp32` formu zorunlu — `disp8`
  imzalı okuma bug'ı düzeltildi: `C7 44 24 E0` = −0x20 yazımı ctx dışına!).
- Kanıt: aynı kaynak, farklı tohum → farklı bytecode (05→0C, 03→2B remap).

## 3) Komut genişletme (rol / ror / bswap / xchg)
- `SHI_ROL/SHI_ROR` mevcut shift op'larıyla bedavaya geldi (`do_shift` genişletmesi).
- Yeni op'lar: `OP_BSWAP_R (0x2A)`, `OP_XCHG_RR (0x2B)`.
- Kanıt: `sdk_min_test` (yalnız rol/ror/bswap/xchg) korumalı = 42, r aynı;
  `sdk_ext_test` (rol/ror/bswap/xchg + movslq + SIB + NCALL) korumalı = 42.

## 4) SEH / .pdata — dinamik kayıt (wine-güvenli tasarım)
- dd[3] (EXCEPTION) repoint wine'da istisna dağıtımını bozuyor (bayt-özdeş
  kopyayla bile — üçlü ölçümle kanıtlandı). Nihai tasarım: **dd[3]'e
  dokunma**; unwind tablosu `.vmp0`'a taşınır ve XRT giriş katmanı
  **RtlAddFunctionTable** ile dinamik kaydeder (VMProtect runtime-yaklaşımı).
- Mut gövdeleri için mutator'ın yığın disiplinini izleyen **sadık
  UNWIND_INFO** üretimi (uw haritası: prolog içi yığın ayarlarının
  çıkış-ofsetleri).
- cfg 0x44/0x48: `unw_rva` / `unw_cnt`.
- Kanıt: C++ throw (SEH tabanlı unwinder) korumalı mut gövdeden main'e
  düşer, `YAKALANDI e=7`, 42; A/B: kayıt kapalıyken de 42 (wine'da
  leaf-assume şansı yetiyor) — mimari gerçek Windows için gerekli.

## 5) SectionCryptor — pack'li akışların CFB-64 şifrelenmesi
- `--sectcrypt`: XPK1 LZMA akışları CFB-64 şifrelenir (kayıt-bağı nonce =
  `f(k0, dst_rva)` — akış yer değiştirme/çapraz-splice imkânsız).
- XPK1 kaydı 12B→**16B** (`srcsz` alanı eklendi); header +12 (eski rezerv)
  = flag. cfg 0x4C/0x50/0x58: flag + maskeli anahtarlar (`XRC_KEYMASK`).
- Runtime `xrt_unpack`: RWX mapli `.vmp1` üzerinde **yerinde çözme** → LZMA.
- Kanıtlar: pack+sectcrypt (x64 koşu 42; x86 statik: cfg flag=1, tüm akışlar
  python-CFB ile çözülüp LZMA'dan geçiyor), **tek bayt tamper → RCODE=117
  (hızlı duru ret)**.

## 6) Dayanıklılık sertleştirmeleri (0.8.0 tamamlayıcıları)
- **Adım bütçesi**: interpreter döngüsünde 1<<24 adım sınırı — CFB tamper'ının
  ürettiği çöp zıplama sonsuz askıya çeviremez (DoS sınıfı zayıflık kapandı).
- **STRDEC sertlik üst sınırı**: çöp `len` ile 4G yineleme engellendi.
- **movslq (MOVSXD) interpreter fix**: `ssz==4` artık 32-bit işaret
  genişletme (eski kod 16-bit'e düşürüyordu).
- **0.8.0 KRİTİK FIX — encoder perm atlaması**: `XP_MOVZX/XP_MOVSX` emission
  ham `e()` kullanıyordu → perm tablosu atlanıyor → akış desync →
  `itab[0xC0]` tablo-dışı okuma → wild write (sdk_ext crash'i). `eop()` ile
  düzeltildi. (Teşhis: host harness + op-op iz — `vmharness.c`, `bcwalk.py`.)
- **in-place CFB yön fix'i**: geri besleme çözme tarafında XOR-öncesi
  (şifreli) baytlardan alınmalı; tek fonksiyon `dec` parametresiyle
  ayrıştırıldı (ilk 8 bayt özdeş, sonrası sapma bulgusuyla kanıtlandı).
- **novmenc + CFB etkileşimi belgelendi**: `--novmenc` encoder key=0 ile
  şifreler, `vm_entry` koşulsuz K2 XOR'lar → novmenc çıktıları koşmaz
  (bilinen sınırlama; testlerde vmenc=ACIK kullanılır).

## 7) Kanıt matrisi (final motor + final blob)
| # | Test | Sonuç |
|---|------|-------|
| R1 | sdk_min V-only (rol/ror/bswap/xchg bytecode) | 42 ✅ |
| R2 | pack + sectcrypt + V | 42 ✅ |
| R3 | pack (şifresiz, 16B kayıt formatı) | 42 ✅ |
| R4 | sectcrypt tek-bayt tamper | 117 (duru ret) ✅ |
| R5 | w1 ultimate: V+M+pack+sectcrypt+loadercrc+filecrc+stripfixups | 42 ✅ (a=78 b=0x16f9 c=0x1221 d=81 altın değerler) |
| R6 | sdk_ext V (movslq/SIB/NCALL + yeni op'lar) | 42 ✅ |
| R7 | Lisans paketi (ok/badsum/badhwid/expire/recovery + GetData cache) | 42 ✅ |
| R8 | SELFTEST (funcs=34) | OK ✅ |
| R9 | metin2 d7610 U + pack + sectcrypt + loadercrc + stripfixups | statik: e9 kapı ✓, TEXTCRC ESLEŞİYOR ✓, 3 akış CFB→LZMA ✓, RELOC 0/0 ✓ |
| R10 | x86 (fake32 PE32) pack+sectcrypt statik | flag=1 + akışlar çözülüyor ✅ |

## Bilinen sınırlamalar
- `--novmenc` CFB ile uyumsuz (yukarıda) — vmenc her zaman açık kullanılmalı.
- 32-bit wine yok: x86 koşu kanıtı yerine statik kanıt (bilinen sınırlama).
- Bölge-kipi rsp-göreceli RED devam ediyor (fonksiyon-kipi serbest);
  gerçek-yığın VM kipi 0.9.0 planında.

## Sürüm dosyaları
- `XProtect_0.8.0.zip` — motor (x64+x86) + SDK + kanıtlar + tam kaynak.
- Kaynak: `XProtect/source/XProtectUI_Source/` (vm_opcodes.h ortak sözleşme,
  xr_cipher.h ortak şifre, engine_pack.cpp SectionCryptor).
