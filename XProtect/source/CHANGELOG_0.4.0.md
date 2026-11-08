# XProtect 0.4.0 — UI Tam Kapsam Portu + PDBScope Entegrasyonu

Tarih: 2026-11-08
Kapsam: XProtectUI (Win32) tam kapsamli UI portu + kullanici araci PDBScope entegrasyonu + .vmp proje uyumlulugu

## Eklenenler

### PDBScope entegrasyonu (kullanici istegi: "tum fonksiyonlari ceker")
- `pdbscope.exe pdbless <hedef> --format csv` subprocess entegrasyonu
  (CreatePipe + CreateProcess, CSV parser: rva,va,size,confidence,percent,class,kind,callers,sources,name)
- Otomatik kesif: exe dizini -> bin\win64 -> bin\win32 -> PATH
- Acilista otomatik tarama + birlestirme (rva bazli, isim/boyut zenginlestirme)
- Fonksiyon Ekle penceresine "Tara" dugmesi
- Paket icine gomulu: PDBScope/pdbscope.exe + bin/win64 + bin/win32 (+ kaynak)
- Olcum: Test.exe 13 -> 568 fonksiyon (CERTAIN/HIGH: 549)

### Fonksiyon Ekle penceresi
- Manual Address grubu: Base + End (kullanici istegi)
- Liste secimi -> Base = fonksiyon adresi, End = fonksiyon sonu
- Tek adres + End -> "End of Function" (brk) kaydi
- Cift tiklama = OK; Enter = OK (cok satirli kutular haric)

### Acilis (boot) sayfasi — VMProtect birebir
- Recent Files (10) + Quick Start (Open File / Open Project / Examples / Contents)
- Open File butonu wine icinde GERCEK XTEST fare tiklamasiyla dogrulandi
  (t -> GetOpenFileName diyalog penceresi aciliyor)

### Sag panel mimarisi (directory_tree_ muadili)
- Functions (N) secimi -> dizin gorunumu: Name | Address | Protection | Size | Source
- Licenses (N) secimi -> Customer Name | Email | Date | Blocked
- Cift tiklama -> oge ozellikleri; kok -> Core Properties (cp* tamami)

### .vmp proje uyumlulugu
- File > Open: .xproj / .vmp / exe otomatik yonlendirme
- Tolerant XML tarayici: Protection@InputFileName/Options + Procedures
  (MapAddress | Address, CompilationType, Options(0x2000=lock), IncludedInCompilation)

### Lisanslar
- Blocked bayragi (diyalog + izgara + sag-tik), duzenleme diyalogu
- VMP varsayilanlari: Expiration=+1 ay, MaxBuild=+1 yil, TimeLimit=30

### Derleme akisi
- Marquee ilerleme penceresi + "Korumali calistirilsin mi?" istemi
- Baslikta degisiklik yildizi; surukle-birak ile acma; VMP acma filtresi

### Duzeltmeler
- CLI: --funcs Isim:Tip artik tipi ayirir (0.3.0'da 0 eslesme bugi)
- Cok satirli EDIT'te Enter = yeni satir
- Agacta (N) sayaclari; [Blocked] etiketi; recent 10'a cikarildi

## Dogrulama (wine64)
```
SELFTEST OK funcs=0                        # bos acilis
SELFTEST OK funcs=568 proj=0               # --open Test.exe
SELFTEST OK funcs=568 proj=5               # --open test_proj.vmp
--cli Test.exe T40.exe --funcs WrapPos:V,GameTick:U -> rc=0
T40.exe (korumali) wine'da temiz cikis
```

## Ikili md5
- XProtectUI.exe  (x64): 725d61362033d588cd9314b475ae04c9
- XProtectUI32.exe (x86): 23b8a5be350289f88b3f134089de0f37
