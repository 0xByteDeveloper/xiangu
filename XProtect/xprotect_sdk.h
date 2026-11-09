// ============================================================
// xprotect_sdk.h — XProtect SDK (0.5.0)
// Kaynak koda isaret (marker) koyarak KORUNACAK BOLGELERI secin.
// Derleyin, XProtect ile acin — isaretli bolgeler otomatik bulunur.
//
// KULLANIM (GCC / Clang / zig cc):
//   #include "xprotect_sdk.h"
//   int hesapla(int a, int b) {
//       XProtectBeginVirtualization();   // sanallastirma (VM)
//       int r = (a + b) * 3;
//       XProtectEnd();
//       return r;
//   }
//
// MODLER:
//   XProtectBeginVirtualization()  -> V  : bayt-kod + sifreleme (en guclu analiz)
//   XProtectBeginMutation()        -> M  : native yeniden-uretim (en hizli)
//   XProtectBeginUltra()           -> U  : mutasyon + sanallastirma (en guclu)
//
// KURALLAR:
//   1) Isaretleri FONKSIYON BASINA koyun (prolog/epilog disinda).
//      Bolge sonunda donus degeri RAX/EAX'te hazir olmali.
//   2) Isaret makrolari EAX'i gecici bozar (mov eax, imm) — bolge
//      basinda/sonunda canli EAX degeri tutmayin.
//   3) x86 hedeflerde V/U icin bolge ICINDE cagri (call) KULLANMAYIN —
//      0.5.0'da x86 VM dogal cagri desteklemez (duru RED). Cagirili
//      fonksiyonlar icin M (mutasyon) kullanin. x64'te uc mod da cagri
//      destekler.
//   4) Bolge icindeki degiskenler volatile olmali (sabit-katlamayi ve
//      kod tasinmasini onler).
//   5) Isaretler calisma zamaninda zararsizdir: korumasiz calistirmak
//      isterseniz XPROTECT_NO_SDK tanimlayin — makrolar bosalir.
//
// SOZLESME (motor tarafinda birebir ayni sabitler):
//   B8 9C37E1B5  B8 <tip 1..3>  ...bolge...  B8 3D7A2C8E
// ============================================================
#ifndef XPROTECT_SDK_H
#define XPROTECT_SDK_H

#if defined(_MSC_VER)
  // MSVC: __asm bloklari (x86); x64 MSVC satir-ici asm desteklemez —
  // x64 icin derleyici intrinsic gereklidir; simdilik GCC/Clang kullanin.
  #define XProtectBeginVirtualization() do { __asm { mov eax, 0x9C37E1B5h } __asm { mov eax, 1 } } while (0)
  #define XProtectBeginMutation()       do { __asm { mov eax, 0x9C37E1B5h } __asm { mov eax, 2 } } while (0)
  #define XProtectBeginUltra()          do { __asm { mov eax, 0x9C37E1B5h } __asm { mov eax, 3 } } while (0)
  #define XProtectEnd()                 do { __asm { mov eax, 0x3D7A2C8Eh } } while (0)
#elif defined(XPROTECT_NO_SDK)
  #define XProtectBeginVirtualization() ((void)0)
  #define XProtectBeginMutation()       ((void)0)
  #define XProtectBeginUltra()          ((void)0)
  #define XProtectEnd()                 ((void)0)
#else
  // GCC / Clang / zig cc: ham bayt yayini (sozdiziminden bagimsiz).
  // "memory" clobber = derleyici bariyeri (kodun isaret disina tasinmasini engeller).
  #define XP_SDK_BEG(t) \
      __asm__ volatile(".byte 0xB8, 0xB5, 0xE1, 0x37, 0x9C" ::: "eax", "memory"); \
      __asm__ volatile(".byte 0xB8, " #t ", 0x00, 0x00, 0x00" ::: "eax", "memory")
  #define XP_SDK_END() \
      __asm__ volatile(".byte 0xB8, 0x8E, 0x2C, 0x7A, 0x3D" ::: "eax", "memory")

  #define XProtectBeginVirtualization() XP_SDK_BEG(1)
  #define XProtectBeginMutation()       XP_SDK_BEG(2)
  #define XProtectBeginUltra()          XP_SDK_BEG(3)
  #define XProtectEnd()                 XP_SDK_END()
#endif

#endif // XPROTECT_SDK_H
