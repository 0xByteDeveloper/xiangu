XPROTECT 0.4.3 — DEGISIKLIK NOTLARI (kullanici 0.4.2 raporu: x1, x2, x4)
========================================================================

x1) Tarama bitince BEYAZ EKRAM (paneller resize'a kadar görünmüyor)
    KOK NEDEN (2 katman):
    - 0.4.1 x1 "boot izolasyonu" ile panel görünürlüğünün TEK kaynağı layout()
      yapıldı; layout() yalnizca WM_SIZE'da çağrıldığı için dosya açılışında
      welcome gizleniyor ama agac/ozellikler/log HIC gosterilmiyordu. İlk
      WM_SIZE (maximize/restore) gelene kadar ekran beyaz kaliyordu.
    - buyuk exelerde merge O(N^2) ana thread'i saniyelerce bloklyordu.
    FIX:
    - update_all() artik layout() uygular (boot->workspace gecisi birebir).
    - open_target sonunda tam boya zorlanir (InvalidateRect + UpdateWindow).
    - merge_funcs rva->index hash map ile O(N+M) (metin2: 39.078 fonksiyon
      saniyeler icinde; wine kanitli log + goruntu).

x2) Fonksyon ekle penceresi ~10 sn aciliyor + listeden secim Base/End'e
    yazmiyor
    KOK NEDEN (3 katman):
    - 39k+ satirin tek-tek InsertItem+SetItemText akisi Windows'ta ~10 sn
      suruyor (0.4.2'nin "568 satir 50ms" olcumu mini-test'e dayaniyordu).
    - WM_SETREDRAW toplu doldurmasi ise wine'da listeyi HIC boyamiyor
      (0.4.1'in ayni bozuklugu; satirlar mantiksal, piksel yok).
    - secim->Base/End dolumu pump (mesaj dongusu) seviyesindeydi; bazi
      mesaj siniflandirma durumlarinda bildirim kaciriliyordu.
    FIX:
    - AF listesi + ana Fonksiyonlar listesi SANAL LISTVIEW (LVS_OWNERDATA)
      oldu: satir objesi uretilmez, metin LVN_GETDISPINFO ile saglanir.
      39.078 satir ANINDA acilir (wine goruntu kanitli). Details sekmesi de
      sanala cevrildi.
    - secim dolumu diyalog WNDPROC seviyesine tasindi; format artik
      kullanici istegi gibi 0x onekli hex: Base=0x1050, End=Base+boyut
      (kanit: entry_00001050 satiri -> Base=0x1050, End=0x2000).
    - acilis taramasi surerken Add Function/GoTo kilitlenir (tarama bitmeden
      acilan diyalogda liste bayat kaliyordu — "bos liste" raporunun kok
      nedenlerinden biri). Durum satiri + log uyarisi verir.

x4) 3 mod (V/M/U) ciktisi AYNI dosya / ayni boyut
    KOK NEDEN: motorun Mutation yeniden-uretim gecisinde allow_call
    parametresi YANLISLIKLA pe.is64 yollaniyordu. x86'da (is64=false)
    ikinci gecis "call desteklenmiyor" diyerek dusuyor -> [HATA] + rc=1 ->
    CIKTI DOSYASI HIC YAZILMIYOR. Diskteki ESKI cikti (V ciktisi) "mutation"
    sanilarak kullaniliyordu. (Kanit: kullanicinin virtualization ve mutation
    dosyalari bayt-bayt ayni md5 39a0a3db...; motor V ciktisiyla birebir
    ayni — yani kullanici hicbir zaman Mutation ciktisi uretememisti.)
    FIX:
    - ikinci gecis allow_call=true (olcum gecisiyle ayni; 0.4.1 x86 rel32
      dis-cagri fixup'i destekli).
    - derleme BASARISIZ olursa GUI artik acikca HATA mesaji gosterir
      ("eski cikti dosyasini bu denemenin sonucu SANMAYIN").
    KANIT (metin2client 0xD7610-0xD766C, 92 bayt):
      V: [YAMA] bc=193B bytecode+stub, .vmp0=0x4CA6  md5 39a0a3db...
      M: [MUT]  ikame=6 junk=13 kapi=5, 92B->207B    md5 4a45369e... (YENI)
      U: [YAMA] bc=540B + [ULTRA] on-mutasyon        md5 75fdb567...
    uc mod artik GERCEKTEN farkli cikti uretir.
    (VMP'de Virtualization: govde VM bytecode'una cevrilir, .vmp0'da
    yorumlanir; Mutation: govde native kalir, ikame+junk ile yeniden
    yazilir; Ultra: once mutation, sonra virtualization. XProtect artik
    ayni sirayi uygular.)

DIGER
- Derleme hatasinda (rc>0) uyarı mesaj kutusu.
- surum define'i 0.4.0'da kalmisti -> 0.4.3 (About/title).
- --uistate tanı araci genisletildi (iki dokum + scr koordinatlari).

DOGRULAMA
- SELFTEST OK (wine64+Xvfb) — motor regresyonu temiz.
- CLI regresyon: --add D7610:D766C:V|:M|:U -> 3 farkli md5, rc=0.
- GUI kanitlari (XTEST gercek tiklama + ekran goruntusu):
  * acilis sonrasi resize YOK: agac + Core Properties + log + status boyali.
  * AF diyaloqu 39.078 satir ANINDA dolu (sub_1000...sub_15fa gorunur).
  * satir secimi -> Base=0x1050, End=0x2000 dolumu.
