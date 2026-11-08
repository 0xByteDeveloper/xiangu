XProtect 0.4.2
==============

0.4.1'in beklenmedik regresyonu giderildi: "Fonksiyon Ekle" (Add Function)
penceresinde hicbir nesne gozukmuyordu.

Ne oldi (kisa):
- 0.4.1'deki toplu WS_CLIPSIBLINGS, wine/X11'de fonksiyon listesi, filtre ve
  Tara dugmesinin TAB sayfasinin ALTINDA kalmasina yol aciyordu (liste dolu
  ama piksel boyanmiyordu). Kaldirildi; boot sayfasi izolasyonu aynen durur.
- Liste artik pencere son geometrisine oturduktan SONRA dolduruluyor;
  kolon basliklari ve satirlar ustten dogru basiyor.
- WM_SETREDRAW toplu doldurma 0.4.0'in kanitlanmis duz doldurmasina dondu.
- af_layout'un tasidigi Lock To Serial Number kutusunun handle ataması
  tamamlandi (boyutlandirmada gecersiz HWND erisimi engellendi).

Icerik (0.4.1 ile ayni + yukaridaki fixler):
- XProtectUI.exe (x64) + XProtectUI32.exe (x86) — saf Win32 arayuz
- Motor: mutation (M), virtualization (V), ULTRA (M+V); x86 natcall
- PDBScope gomulu (PDB varsa sembollerle, yoksa pdbless tarama)
- Test.exe / Test32.exe + ULTRA ornek ciktilar, kaynak agac, nssetup.c
- --selftest / --uistate / --open / --cli otomasyon arabirimleri

Hizli dogrulama (kendi makinenizde):
- XProtectUI.exe'yi acin -> Open File ile Test.exe'yi secin
- Project > Add Function... -> listede 568 fonksiyon ustten listelenir
- listeden secim Base/End kutularina adres+yazilir (0.4.0 davranisi)

Surum notlari: 0.4.1'deki tum ozellikler (x1-x6 fixleri, thread'li tarama,
yuzde ilerleme, boyutlandirilabilir pencere, sekme/filtre) korundu.
