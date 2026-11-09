# XProtect 0.5.1 — Degisiklik Gunlugu

## Hedef
0.5.0 geri-cagrisi: "3 modda da sifrelemiyor" (metin2client +D7610 ciktiyla
bayt-bayt ayni). Kok nedenler bulundu, duzeltildi, kanitlandi.

## Kok Nedenler (0.5.0)
1. `engine_main.cpp`: `lift_allow_call = pe.is64` — x86 hedeflerde CALL'li
   fonksiyonlar V/U'da sessiz RED. metin2client (x86) tam bu durumdaydi.
2. Manuel fonksiyonlar `man_rva` uzerinden "bolge-kipi"ne dusuyordu; SDK
   isaretleri ile kullanici fonksiyonu ayristirilmiyordu (`sdk_rva` seti yok).
3. Sahte-basari: `comps.empty()` olsa bile cikti yaziliyor, rc=0.

## Duzeltmeler (0.5.1)
- `lift_allow_call = true` (EXPERIMENT) — x86 V/U natcall'li kaldirim geri acildi.
- `sdk_rva` seti: yalniz SDK isaretleri bolge-kipinde (5 kullanim noktasi).
- Sahte-basari korumasi: hicbir secili fonksiyon uygulanamazsa cikti YOK,
  rc=3, [HATA]/[RED] + GUI uyari kutusu (`g_last_reds`).
- xrt W sarmalayicilari kok-duzeltme (args-array + kayit-PIN):
  * eski: `"rm"` operandlar rsp kaydirmasindan sonra yanlis adresten okunuyor,
    `rbx` clobber-disi oldugundan operand rbx'te kalabilir (`movq rsp,rbx` ezer).
  * yeni: argumanlar C'de yerel diziye kopyalanir; x64'te r14 (dizi) + r15 (fn)
    kayit-degiskeni PIN, rbp cipasi; x86'da esi uzerinden sabit ofset pushl.
  * x86'daki hatali `xchgl esi,(esp)+popl` kuyrugu kaldirildi (cagiran yigini bozuyordu).
- `rt_fatal` cikis kodlari: 0x70+hata (1=self, 2=text, 3=file, 4=k32).
- Olcu: XRT_ERRCODE debug problari XRT_PROBE bayragi altinda korundu.

## Dogrulama (wine64)
- selftest: `SELFTEST OK` rc=0 (x64; x86 wine32 bu oturumda yok).
- rt_chain V + self/text/file CRC:
  temiz=167 (orijinalle ayni), text-bozuk=114, file-bozuk=115.
  (0.5.0'da temiz dosya bile 115 veriyordu — yanlis-pozitif.)
- metin2client +D7610 (kullanicinin rapor ettigi fonksiyon):
  V: E9->0x5469A0 (bc=194B), M: E9->0x5468D0 (210B govde), U: E9->0x546B00 (bc=545B);
  md5 ayrisma: ec9cb1d0 / e57df2f7 / 58a8449d.

## Not
- rt_chain.c icindeki "0x2A=42 beklenir" yorumu hatali; dogru sonuc 167
  (157+1+9). Korumali cikti da 167 urettigi icin davranis esligi KANITLANDI.
