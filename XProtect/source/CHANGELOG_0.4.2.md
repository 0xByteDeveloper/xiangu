# XProtect 0.4.2 — Changelog

## Fix: fonksiyon listesi penceresinde hicbir nesne gozukmuyor (kullanici raporu)

### Kok neden (2 katman, ikisi de 0.4.1'de girildi)

1. **mk_ctl toplu WS_CLIPSIBLINGS (ana neden)**
   0.4.1, x1 (boot sayfasi duzeltmesi) kapsaminda TUM alt kontrollere toplu
   `WS_CLIPSIBLINGS` eklemisti. Wine/X11'de bu stili tasiyan her cocuk ayri bir
   X alt-penceresine donusur; X yigin sirasi Win32 z-sirasini izlemedigi icin
   Add Function penceresinde ONCE yaratilan TAB kontrolunun beyaz sayfasi,
   SONRA yaratilan filtre / Tara / LISTVIEW / durum kontrollerinin uzerine
   boyaniyordu. Liste bellekte dolu ve gorunurdu (LVM_GETITEMCOUNT = 568,
   IsWindowVisible = TRUE) ama ekranda hicbir piksel yoktu — kullanici
   gozleminin ("hicbir nesne gozukmuyor") tam aciklamasi.
   Duzeltme: mk_ctl 0.4.0 davranisina donduruldu (toplu clip yok). Boot sayfasi
   izolasyonu (x1) layout gizlemesiyle saglanmaya devam eder; g_welcome'in
   kendi WS_CLIPSIBLINGS'i ve tab'in acik clip'i korunur. Kanit: boot sayfasi
   render + Quick Start tiklamasi 0.4.2'de yeniden dogrulandi.

2. **Haritalanmadan once doldurulan listview (ikincil)**
   0.4.1 listeyi pencere gosterilmeden ONCE dolduruyor ve af_layout ile
   tasiiyordu; wine X11'de bu, satirlarin sayfanin altina kaymasi ve kolon
   basliklarinin boyanmamasi seklinde bozuluyordu.
   Duzeltme: ilk doldurma ShowWindow + af_layout sonrasina tasindi.

3. **af_fill_list WM_SETREDRAW toplu doldurma (ikincil)**
   Wine'da kaydirma/boyama tutarsizligi yaratmisti. 0.4.0'in kanitlanmis duz
   doldurmasina donuldu (568 satir ~50 ms).

4. **af_layout st.chk_lock atanmamisti (yan bug)**
   AFState.chk_lock doldurulmadan MoveWindow'a giriyordu (gecersiz HWND).
   Artik pencere yakalanip atanıyor.

## Dogrulama (wine64 + Xvfb, otomatik tani: --uistate)

- Add Function penceresi: items=568, 5 kolon, satirlar ustten:
  WrapPos/MovePlayer/GhostStep/CheckCollision/GameTick/entry_point...
  (Adres/Boyut/Export/Source kolonlari dolu; header boyaniyor.)
- Boot sayfasi (x1 regresyon): render + Quick Start "Open File" tiklamasi canli.
- SELFTEST OK funcs=568 proj=0 lics=0
- CLI regresyon: --cli Test.exe --funcs WrapPos:V,GameTick:U -> V=1 U=1 rc=0.
- Ikililer: XProtectUI.exe 1020928 B (a2bd5d5c), XProtectUI32.exe 1072128 B.

## Tani araci

`--uistate`: normal kullanimda etkisiz; wine otomasyonunda diyalog ic-durumunu
(geometri, gorunurluk, listview icerigi) dosyaya dokar. Kalici tani imkani.
