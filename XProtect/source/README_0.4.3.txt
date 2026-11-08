XProtect 0.4.3 — universalObfusPublicSrc (VMProtect 3.5.1 kaynak agaci) tabanli
C++ koruma motoru + saf Win32 API GUI portu. Qt YOK.

PAKET ICERIGI
  XProtectUI.exe      x64 arayuz + motor (tek exe)
  XProtectUI32.exe    x86 arayuz + motor
  Test.exe / Test32.exe          ornek hedef (x64/x86)
  Test_vmp.exe / Test32_vmp.exe  ornek korumali ciktilar
  Test_exe_Source/               ornek hedef kaynagi
  XProtectUI_Source/             tam kaynak (build_ui.sh ile yeniden derlenir)
  PDBScope/                      gomulu fonksiyon tarama araci (pdb + pdbless)
  tools/nssetup.c                wine headless kurulum yardimcisi
  examples_test_proj.vmp         ornek VMProtect proje dosyasi
  README.txt / CHANGELOG.md

0.4.3 OZETI
  x1 acilis beyaz ekrani (layout tek-kaynak + zorla boya + O(N) merge)
  x2 Fonksyon ekle: 10 sn kasilma (SANAL LISTVIEW) + secim->Base/End
     0x-hex dolumu (wndproc seviyesi) + tarama kilidi
  x4 Mutation x86 yeniden-uretim allow_call fix -> 3 mod GERCEKTEN farkli
     (V bytecode / M native-mutant / U mutant+bytecode; kanit md5 uclusu)
  derleme hatasinda acik mesaj; surum define 0.4.3

HIZLI BASLANGIC
  1) XProtectUI.exe'yi calistirin (x64) veya XProtectUI32.exe (x86).
  2) Boot sayfasindan Open File -> hedef exe. PDB ayni klasordeyse sembollu,
     yoksa pdbless tarama yapilir (ilerleme diyalogu % gosterir).
  3) Project > Add Function (Insert): listeden secin veya Manual Address
     Base/End girin; Options sekmesinden tip (Virt/Mut/Ultra) secin.
  4) Compile (F9) -> <isim>_vmp.exe uretilir.
  5) Execute menüsünden orijinal/korumali calistirin.

CLI (otomasyon)
  XProtectUI.exe --cli <girdi> <cikti> [--type V|M|U] [--strength 1..3]
      [--all | --funcs Isim:V,Isim2:U | --func Isim:M | --add BASE:END:TYPE]
      [--antidebug] [--strenc] [--memprot] [--novmenc]
  Ornek: XProtectUI.exe --cli Test.exe out.exe --add 1000:1040:U

SELFTEST
  XProtectUI.exe --selftest --open Test.exe  ->  SELFTEST OK funcs=N

NOTLAR
- Korunan cikti <hedef>_vmp.exe olarak hedefin yanina yazilir. Derleme
  BASARISIZ olursa ESKI cikti diskte kalir; program bunu acikca bildirir.
- x64 derlemelerde mutation'li fonksiyonlarda istisna isleme (unwind) eski
  govdeye isaret eder — VMProtect davranisi.
- PDBScope ayni klasorde bulunursa daha iyi sembol sonucu verir.

LISANS: egitim/arastirma amacli ornek; VMProtect ticari urunudur.
