# XProtect 0.4.1 — Hata Düzeltmeleri + x86 ULTRA gerçek

Tarih: 2026-11-08
Kapsam: Kullanıcı raporu x1–x6 düzeltmeleri; x86 natcall + manuel fonksiyon hattı; tarama thread'i + yüzde; diyalo yenilemesi

## x6 — ULTRA (M+V) manuel fonksiyonlarda gerçek (KÖK NEDEN + DÜZELTME)

**Hata:** Manuel eklenen fonksiyon (Base/End) ULTRA seçilse bile derlenmiyordu.
- `start_compile` manuel fonksiyonları yalnızca RVA-hex anahtar olarak `opt.funcs`'a yazıyordu; motor seçimi **kendi tarayıcı adaylarıyla** eşleştiriyordu (`man_rva` kavramı yok).
- Aday bulunamazsa fonksiyon sessizce düşüyordu → `.vmp0` yalnızca VM blob (x86: 18257 bayt) taşıyordu. Kanıt: kullanıcının metin2client çıktısında RVA 0xD7610 bölgesi bayt bayt AYNI, `.vmp0` vsz=0x4751 = VM_BLOBX86_LEN.
- Aday bulunsa bile: (a) boyut tarayıcı tahmini (kullanıcının End'i yok sayılıyordu), (b) x86'da `call` içeren fonksiyon lifter/mutasyon tarafından reddediliyordu ("call desteklenmiyor").

**Düzeltmeler:**
- `Options::manual {rva, size, ty, name}` + `engine_main` birleştirme: kesin {rva, boyut, tip}; aynı RVA'daki tarayıcı adayı silinir; kapsam/boyut kırpma + `[MANUEL]`/`[ULTRA] on-mutasyon RED` logları.
- **x86 natcall** (`vm_core.c`): yığın-geçişli yerli çağrı — `mov ebp,esp` ile yorumlayıcı ESP korunur, misafir yığınına geçilir (`esi`), `call *edi`; ECX/EDX misafir değerleri gerçek register'lara yüklenir (**thiscall/fastcall** argmanları!), callee'nin kendi `ret/ret imm16` yığınını temizler, EAX→regs[0], yeni misafir ESP=gerçek ESP+VM_DELTA. Blob yeniden üretildi (vmx86.bin 18257→19002).
- x86 lift/mutasyon `allow_call=true` (dış çağrı rel32 fixup her iki mimaride çalışır).
- **0.3.0 gizli bug:** `lift_function` yerel `retimm`'i `F.retimm`'e aktarmıyordu → x86 stub'lar hep C3 üretiyordu (stdcall ESP kayması). Şimdi doğru.
- **Mutasyon junk NOP'suz:** lifter'ın "ret sonrası dolgu = gövde bitti" sezgisi mutant gövdeyi yarida kesiyordu (mini32 kanıtı: bc 294B, VM erken çıkış, EAX=arg2). Lifter'a `pad_terminates` bayrağı: mutant girdide dolgu akışı kesmez, çözülemeyen bayt sesli hata.

**Kanıtlar (Unicorn diferansiyel):**
- metin2client 0xD7610–0xD766C (thiscall, 4 call, ret 8) ULTRA: `[YAMA] stubRVA retimm=8 (trampoline)` + `[ULTRA] ikame=6 junk=13 kapi=5 | 92B->207B (x2.25)`; diferansiyel **5/5 ESİT** (callee'ler eax=1 stub'lı; EAX+ESP+yazmalar birebir).
- mini32 klon (ebp frame + thiscall + push arg'lar + 4 çağrı): **4/4 ESİT**.
- Test32.exe U regresyonu: **5/5 ESİT** (0.3.0 yolları bozulmadı).
- Test_vmp.exe (x64 ULTRA) wine'da oynanıyor: MiniPac başlık + skor + EOF'ta temiz çıkış.
- Giriş kapısı: `0xD7610: e9 ...` (E9 → .vmp0 trampoline) + CC dolgu.

## x1 — boot sayfası tıklanabilirlik
- Çalışma-alanı panelleri (tree/func/prop/dir/log/split/filter) boot görünürken **tamamen gizlenir** (tek kaynak: `layout()`); `g_welcome` + tüm çocuklara `WS_CLIPSIBLINGS`.
- Kanıt: wine'da gerçek XTEST tıklaması — Quick Start "Open File" → `Open` (GetOpenFileName) penceresi.

## x2 — tarama thread'i + yüzde + PDB kontrolü
- `ScanJob` + `scan_thread`: dosya açılış taraması iş parçacığında; ilerleme diyaloğu **%0-100 + "N fonksiyon bulundu" + [PDB/pdbless] modu**; pompa 100ms'lik `WM_TIMER` ile canlı.
- Aynı klasörde `<hedef>.pdb` varsa `pdbscope semboller`, yoksa `pdbscope pdbless` (kullanıcı istediği mod ayrımı).
- CSV ayrıştırıcı iki biçim (pdbless 10 / semboller 13 kolon) ve başlığı **içerikten** tanır (boruda stderr satırları başlığın önüne geçebiliyor — "ilk satır başlıktır" varsayımı listeyi boşaltıyordu).
- 90 sn tavan: askıda araç güvenle sonlandırılır; UI yaşamaya devam eder.
- Ölçüm: Test.exe → `PDBScope[pdbless]: 568 fonksiyon (güçlü: 549) [+565 new, 0 enriched]`.

## x3 — fonksiyon listesi penceresi
- 840×640 varsayılan + `WS_THICKFRAME|WS_MAXIMIZEBOX` + `af_layout` (WM_SIZE'da tüm kontroller yeniden yerleşir) + min 640×500; liste toplu doldurma (`WM_SETREDRAW`).

## x4 — liste seçimi → Base/End
- Seçim: Base = fonksiyon RVA, End = RVA+boyut (boyut yoksa +16); çift tık = OK.

## x5 — Fonksiyonlar/Seçenekler sekmesi + Tara + filtre
- Sekme geçişi **Tara düğmesi dahil** tüm sayfa kontrollerini gizler/gösterir (düzeltme: "Tara" Seçenekler'de de görünüyordu).
- Filtre `WM_COMMAND/EN_CHANGE` ile canlı (eski kod EN_CHANGE'i WM_NOTIFY içinde arıyordu → "sub"/"pointer" yazınca hiçbir şey olmuyordu).
- "Tara" iş parçacığıyla çalışır; durum satırı yüzde + bulunan sayısı; bitince liste tazelenir.

## CLI
- `--add BASE:END:TYPE` (örn. `--add D7610:D766C:U`) — manuel fonksiyon komut satırından da verilebilir.

## İkililer
- XProtectUI.exe 1.017.344 B (md5 fbb2a577…), XProtectUI32.exe 1.070.080 B (md5 0bc14657…)
- SELFTEST: `SELFTEST OK funcs=568 proj=0 lics=0` (wine64, Xvfb)
