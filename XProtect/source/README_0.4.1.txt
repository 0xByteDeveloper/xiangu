XProtect 0.4.1 — Hata Düzeltmeleri + x86 ULTRA (M+V) Gerçekleşti
==================================================================

Bu surumde ne var (0.4.0 -> 0.4.1)
-----------------------------------
x6) ULTRA (M+V) MANUEL FONKSIYONLARDA GERCEKLESTI:
   - HATA: Manuel eklenen fonksiyon (Base/End) ULTRA secilse bile
     HIC DERLENMIYORDU — motor yalniz kendi tarayici adaylariyla
     eslestiriyordu. Kanit: metin2client 0xD7610-0xD766C ULTRA ->
     cikti .vmp0 yalnizca VM blob (18257 bayt), fonksiyon baytlari
     AYNEN kalmisti.
   - DUZELTME: "End" bilinen fonksiyonlar artik KESIN {rva, boyut,
     tip} olarak motora gider (Options::manual); ayni RVA'daki
     tarayici adayi kesin boyut lehine cikarilir.
   - x86 NATCALL: x86 VM'inde dis cagri destegi YOKTU (call iceren
     fonksiyonlar reddediliyordu). Eklendi: yigin-gecisli cagri
     (stdcall/cdecl/thiscall/fastcall; ECX/EDX register argmanlari
     yuklenir, callee'nin kendi ret imm'si yigini temizler).
   - x86 MUTASYON: dis-cagri rel32 fixup zaten destekliydi; x86
     siniri kaldirildi.
   - 0.3.0 GIZLI BUG: lifter yerel retimm'i F.retimm'e aktarmiyordu
     -> tum x86 stub'lar "ret" (C3) uretiyordu; stdcall temizligi
     yanlis. Simdi ret imm16 dogru.
   - MUTASYON JUNK: artik ortada NOP uretmez; lifter "ret sonrasi
     dolgu = govde bitti" sezgisi mutant govdeyi yarida kesmesin.
     Lifter'a pad_terminates bayragi (mutant girdide dolgu akisi
     kesmez, bilinmeyen bayt sesli hata verir).
   - KANITLAR:
     * metin2client 0xD7610 (thiscall, 4 call, ret 8) ULTRA:
       [YAMA] stub E9 kapisi + [ULTRA] ikame=6 junk=13 kapi=5
       | 92B->207B (x2.25) — Unicorn diferansiyel 5/5 ESIT
       (orijinal == korunan; EAX + ESP + bellek yazmalari).
     * mini32 klon fonksiyon: 4/4 ESIT.
     * Test32.exe U regresyonu: 5/5 ESIT (0.3.0 davranisi bozulmadi).
     * Test_vmp.exe (x64 ULTRA) wine'da oynaniyor: MiniPac baslik +
       skor + EOF'ta temiz cikis.

x1) BOOT SAYFASI (ilk acilim) TIKLANABILIRLIKI:
   - Calisma-alani panelleri (agac/liste/prop/log/split) boot sayfasi
     gorunurken TAMAMEN gizlenir (VMP boot_frame davranisi) — kardeş
     pencerelerin "arkadakiyle buga girmesi" bitti.
   - Tum cocuk kontrollere WS_CLIPSIBLINGS.
   - Kanit: wine icinde gercek XTEST tiklamasi: Quick Start "Open
     File" -> GetOpenFileName diyalogu aciliyor.

x2) TARAMA IS PARCACIGI + YUZDE + PDB KONTROLU:
   - Dosya acilisi taramasi ARTIK THREAD'de: arayuz donmuyor.
   - Ilerleme diyalogu: yuzde (%0-100) + "N fonksiyon bulundu" +
     tarama modu.
   - Ayni klasorde <hedef>.pdb VARSA "semboller" ile (PDB/MAP),
     YOKSA "pdbless" ile taranir (pdbscope_scan_ex).
   - CSV parser iki bicim okur (pdbless 10 kolon / semboller 13 kolon)
     ve basligi ICERIKTEN tanir (borudaki stderr gurultusu listeyi
     bosaltabiliyordu).
   - 90 sn sonunda askida kalan arac guvenle sonlandirilir.

x3) FONKSIYON LISTESI PENCERESI:
   - Boyutlandirilabilir (WS_THICKFRAME + WM_SIZE yeniden yerlesim +
     en az 640x500) ve daha buyuk varsayilan (840x640).
   - Liste toplu doldurma (WM_SETREDRAW) — binlerce satirda donme azaldi.

x4) LISTE SECIMI -> MANUAL BASE/END DOLUMU:
   - Listedeki fonksiyona tiklayinca Base = adres, End = adres+boyut
     (boyut yoksa +16). (0.4.0'da vardi; x3/x5 ile birlikte saglamdi.)

x5) FUNKSIYONLAR/SECENEKLER SEKME HATALARI:
   - "Tara" dugmesi Secenekler sekmesinde de gorunuyordu — sekme
     gecisi artik Tum sayfa kontrollerini gizler/gosterir.
   - Filtre HIC CALISMIYORDU (EN_CHANGE yanlislikla WM_NOTIFY icinde
     aranmisti) — "sub"/"pointer" yazinca liste bosalmiyordu. Artik
     WM_COMMAND EN_CHANGE ile canli filtre.
   - "Tara" isi parcacigiyla: diyalog donmez, durum satiri yuzde +
     bulunan sayisini gosterir; bitince liste tazelenir.

CLI: XProtectUI.exe --cli ... --add BASE:END:TYPE  (manuel fonksiyon,
     ornek: --add D7610:D766C:U)

Dogrulama ozeti
---------------
- XProtectUI.exe (x64) wine selftest: SELFTEST OK funcs=568 proj=0 lics=0
- Test32.exe U regresyonu (Unicorn): 5/5 ESIT
- metin2client manuel ULTRA diferansiyel: 5/5 ESIT
- Test_vmp.exe wine oyun ciktisi: temiz (MiniPac)
- Ikililer: XProtectUI.exe 1017344B (fbb2a577) /
  XProtectUI32.exe 1070080B (0bc14657)

Kullanim notu (ULTRA denemek icin, kullanici senaryosu):
1) Dosya ac -> Add Function -> fonksiyon listesinden sec (Base/End dolar)
   veya Base=0xD7610 End=0xD766C yaz.
2) Derleme turu = ULTRA (M+V) -> OK.
3) Compile. Log'da "[MANUEL] ... tip=U" + "[ULTRA] on-mutasyon ..." +
   "[YAMA] ... (trampoline)" satirlarini gorursun.
4) Disassembler'da fonksiyon basi artik E9 (VM kapisi) + CC dolgudur.
