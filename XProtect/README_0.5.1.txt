XProtect 0.5.1 — OKUMA DOSYASI
================================

BU SURUM NEDEN VAR?
-------------------
0.5.0'da senin bildirdigin ciddi gerileme duzeltildi: "3 modda da sifrelemiyor"
(metin2client.exe +D7610 fonksiyonu ciktiyla bayt-bayt ayni kaliyordu).

0.5.0'DA NE BOZUKTU? (uc ayri hata, birlesik etki)
--------------------------------------------------
1) engine_main.cpp: lift_allow_call = pe.is64
   -> x86 (32-bit) hedeflerde CALL iceren fonksiyonlar V/U modunda SESSIZCE
      RED ediliyordu. metin2client x86 oldugu icin V ve U hicbir sey yapmiyordu.
2) Manuel eklenen fonksiyonlar yanlislikla "SDK bolgesi" sayiliyordu
   (man_rva yerine sdk_rva seti yoktu) -> yanlis donusum yolu.
3) SAHTE-BASARI: hicbir fonksiyon uygulanamasa bile cikti dosyasi YINE yaziliyor,
   rc=0 donuyor, log "korundu" diyordu. En sinsi hata buydu.

0.5.1'DE NE YAPILDI?
--------------------
- lift_allow_call = true (EXPERIMENT): x86 V/U artik CALL'li fonksiyonlari
  natcall ile kaldiriyor (0.4.3 davranisi + 0.5.x duzeltmeleri).
- sdk_rva seti: yalniz SDK isaretleri bolge-kipi; manuel fonksiyon = gercek fonksiyon.
- SAHTE-BASARI KORUMASI: secili fonksiyonlarin HICBIRI uygulanamazsa
  CIKTI DOSYASI YAZILMAZ, rc=3 + [HATA]/[RED] satirlari + GUI uyari kutusu.
- xrt W sarmalayici makrolarinda KOK-DUZELTME (args-array + kayit-PIN deseni):
  eski "rm" operand + rsp-kaydirma + rbx-clobber-eksik hatalari API'ya cop
  arguman gonderiyordu -> file-CRC yanlis-pozitif (temiz dosya "bozuk" derdi).
  Artik: temiz kosum calisiyor, bozuk dosya yakalaniyor (asagidaki kanit).
- rt_fatal cikis kodlari: 0x70+hata (1=self, 2=text, 3=file, 4=k32) - teshis icin.

KANITLAR (kanit/ klasoru, wine64)
---------------------------------
1) 3mod_bayt_kaniti.txt - senin baktigin yer: metin2client +D7610
   orijinal : 55 8B EC 83 EC 08 56 8B F1 E8 ...   (push ebp/mov ebp,esp/...)
   mod V    : E9 -> jmp 0x5469A0  (VM stub, bc=194B, key=41C3AAC93F0355E3)
   mod M    : E9 -> jmp 0x5468D0  (native mutasyon govdesi 95B->210B)
   mod U    : E9 -> jmp 0x546B00  (on-mutasyon+VM, bc=545B)
   3 cikti md5 birbirinden farkli: ec9cb1d0 / e57df2f7 / 58a8449d
2) runtime_kanit.txt + rtc_final.exe - rt_chain testi:
   temiz kosum: exit 167 (orijinalle ayni), .text bozulunca exit 114,
   dosya bozulunca exit 115. (NOT: rt_chain.c yorumundaki "42 beklenir"
   hatali aritmetikti; dogru sonuc 167'dir.)
3) m2_V/M/U.exe + .log - tam metin2 ciktilari (senin manuel fonksiyonunla).

KULLANIM
--------
GUI: XProtectUI.exe (x64) / XProtectUI32.exe (x86). Dosya ac -> fonksiyon
ekle (Base/End hex RVA) -> tip sec (V/M/U) -> Protect.
CLI: XProtectUI.exe --cli <girdi> <cikti> --type V|M|U --strength 1..3
     --add BASE:END:TYPE  [--selfcrc --textcrc --filecrc] [--rt-nomsg] ...

LIMITLER / NOTLAR
-----------------
- x86 V/U natcall yolu yeniden-acildi (0.4.3 ailesi); Unicorn diferansiyel
  x86-M kanitli, x86 V/U natcall calisma-dogrulugu gercek Windows'ta
  denemende kesinlesir (wine32 bu oturumda kurulu degildi).
- IAT korumasi hala DENEYSEL.
- LZMA packing + lisans runtime/RSA: 0.6.0 hedefi (sira degismedi).
