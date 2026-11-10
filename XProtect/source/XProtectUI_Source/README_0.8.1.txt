XProtect 0.8.1 — duzeltme surumu (0.8.0 uzerine)
================================================

KISA: 0.8.0'da Cheat Engine ile fonksiyonu debug ettiginizde fonksiyon
calismaya devam ediyordu (0.6.2/0.7.0'da duruyordu). Kok neden bulundu,
kanitlandi ve duzeltildi. Ayarlarda "Check Debugger" isaretli oldugu
surece anti-debug artik 0.7.0 ile AYNI sekilde calisir.

TEKNIK (kisa):
1) 0.8.0 polimorfik opcode permutasyonu, Options tarafindan ham basilan
   ANTIDBG/STRDEC op'larini cozemiyordu (bc[0]=0x1E -> CQO'ya dusuyor,
   kontrol hic calismiyordu). Yayim artik lifter ile ayni perm tablosundan
   geciyor. Kanit: winedbg altinda fonksiyon 0xDEADBEEF zehir degerine
   donuyor (duruyor), debug'siz 42; metin2 d7610 statik dogrulandi.
2) STRDEC (strenc) 0.3.0'dan beri format uyumsuzdu (VM 4xu32, encoder
   2xu32). Tam yeniden yapildi: sifreli string kopyasi .vmp0'da, orijinal
   string diske dokunulmaz (M/native kod guvenli), state kopya basina
   paylasilir. Runtime ilk erisimde cozer.

KULLANIM FARKI YOK: onceki surumlerle ayni CLI/GUI. "Check Debugger" =
Cheat Engine/winedbg tespiti. "Debug Mode" ve "Check Kernel Debugger"
kutulari henuz motora bagli degildir (0.9.0 anti-debug paketi).

KANIT DOSYALARI: kanit/ klasoru (selftest, ULTIMATE RCODE=42, tamper
RCODE=113, diferansiyel winedbg loglari, statik cozum araci).
