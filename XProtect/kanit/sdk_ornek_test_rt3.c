// sdk_ornek_test_rt3.c — 0.8.0 CFB (crypt_command) gercek-VM kosu testi
// Ders (rt2'den): volatile'i silmek YETMEZ — derleyici register hesabini
// marker'larin DISINA tasir (bölge 10B kirinti kalir -> yanlis sonuc).
// Cozum: hesap GLOBAL volatile uzerinden (riprel) — memory clobber
// tasmayi engeller, bolge tum govdeyi kapsar, rsp-goreli erisim YOK.
// Beklenen: a=78 b=0x16f9 c=0x1221 d=81, RCODE=42 (hepsi VM/mutasyondan).
#include <stdio.h>
#include <stdlib.h>

#define XP_SDK_BEG(t) \
    __asm__ volatile(".byte 0xB8, 0xB5, 0xE1, 0x37, 0x9C" ::: "eax", "memory"); \
    __asm__ volatile(".byte 0xB8, " #t ", 0x00, 0x00, 0x00" ::: "eax", "memory")
#define XP_SDK_END() \
    __asm__ volatile(".byte 0xB8, 0x8E, 0x2C, 0x7A, 0x3D" ::: "eax", "memory")

static volatile int g_r;

static int sdk_v(int a, int b) {
    XP_SDK_BEG(1);                       /* Virtualization (bytecode + CFB) */
    g_r = a + b;
    g_r = g_r * 3 + 7;
    if (g_r > 100) g_r -= 100;
    XP_SDK_END();
    return g_r;
}

static int sdk_m(int a) {
    XP_SDK_BEG(2);                       /* Mutation (native mutasyon) */
    g_r = a ^ 0x55;
    g_r = g_r + (g_r >> 2);
    XP_SDK_END();
    return g_r;
}

static int sdk_u(int a) {
    XP_SDK_BEG(3);                       /* Ultra (mutasyon+VM) */
    g_r = a * 2;
    g_r = g_r ^ 0x58;
    g_r = (unsigned)g_r >> 1;
    g_r = g_r + 9;
    XP_SDK_END();
    return g_r;
}

static int sdk_print(int a, int b, int c, int d) {
    printf("OK sdk_print %d\n", a);
    if (a == 78 && b == 0x16f9 && c == 0x1221 && d == 81) {
        printf("OK hepsi dogru\n"); return 42;
    }
    printf("OK HATALI SONUC a=%d b=%#x c=%#x d=%d\n", a, b, c, d);
    return 3;
}

int main(int argc, char** argv) {
    int k = (argc > 1) ? atoi(argv[1]) : 0;   /* sabit-katlama engeli */
    int a = sdk_v(19 + k, 38);                /* V: bytecode (CFB) */
    int b = sdk_m(0x1234 + k);                /* M: native mutasyon */
    int c = sdk_u(0x1234);                    /* U */
    int d = sdk_v(19, 39);                    /* V: bytecode (CFB) */
    printf("OK main a=%d b=%#x c=%#x d=%d\n", a, b, c, d);
    return sdk_print(a, b, c, d);
}
