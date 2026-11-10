// sdk_ext_test.c — 0.8.0 komut genisletme kaniti: rol/ror/bswap/xchg
// her adimdan sonra g_out'a yazar; main VM anlik goruntuleri native
// modelle karsilastirir (ilk farkli adim hangi op'ta kayiyor gosterir).
#include <stdio.h>
#include <stdlib.h>

static volatile unsigned int g_out;
static volatile unsigned int g_snap[8];
static volatile int g_n;

#define XP_SDK_BEG(t) \
    __asm__ volatile(".byte 0xB8, 0xB5, 0xE1, 0x37, 0x9C" ::: "eax", "memory"); \
    __asm__ volatile(".byte 0xB8, " #t ", 0x00, 0x00, 0x00" ::: "eax", "memory")
#define XP_SDK_END() \
    __asm__ volatile(".byte 0xB8, 0x8E, 0x2C, 0x7A, 0x3D" ::: "eax", "memory")

static unsigned int sdk_ext(unsigned int x, unsigned int y) {
    unsigned int t = x, t2 = y;
    g_n = 0;
    XP_SDK_BEG(1);
    t = (t << 8) | (t >> 24);            /* rol32 8  */
    g_snap[g_n] = t; g_n = g_n + 1;
    t = (t >> 4) | (t << 28);            /* ror32 4  */
    g_snap[g_n] = t; g_n = g_n + 1;
    __asm__ volatile("bswap %0" : "+r"(t));
    g_snap[g_n] = t; g_n = g_n + 1;
    __asm__ volatile("xchg %0, %1" : "+r"(t), "+r"(t2));
    g_snap[g_n] = t; g_n = g_n + 1;
    t2 = (t2 << 12) | (t2 >> 20);        /* rol32 12 */
    g_snap[g_n] = t2; g_n = g_n + 1;
    __asm__ volatile("xchg %0, %1" : "+r"(t), "+r"(t2));
    g_snap[g_n] = t; g_n = g_n + 1;
    g_out = t;
    XP_SDK_END();
    return g_out;
}

static unsigned int rol32(unsigned int v, int n) { return (v << n) | (v >> (32 - n)); }
static unsigned int ror32(unsigned int v, int n) { return (v >> n) | (v << (32 - n)); }
static unsigned int bswap32(unsigned int v) { return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v>>8)&0xFF00)|(v>>24); }

int main(int argc, char** argv) {
    unsigned int x = (argc > 1) ? (unsigned)strtoul(argv[1], 0, 16) : 0x1234;
    unsigned int y = (argc > 2) ? (unsigned)strtoul(argv[2], 0, 16) : 0xA5A5A5A5;
    unsigned int r = sdk_ext(x, y);
    unsigned int t = x, t2 = y, tmp;
    t = rol32(t, 8);
    t = ror32(t, 4);
    t = bswap32(t);
    tmp = t; t = t2; t2 = tmp;
    t2 = rol32(t2, 12);
    tmp = t; t = t2; t2 = tmp;
    int ok = 1;
    printf("OK sonuc r=%08x beklenen=%08x %s\n", r, t, r == t ? "ESLESTI" : "FARKLI");
    printf("snap: ");
    for (int i = 0; i < 6; i++) printf("%08x ", (unsigned)g_snap[i]);
    printf("| model: %08x %08x %08x %08x %08x %08x\n",
           rol32(x,8), ror32(rol32(x,8),4), bswap32(ror32(rol32(x,8),4)),
           y, rol32(bswap32(ror32(rol32(x,8),4)),12), rol32(bswap32(ror32(rol32(x,8),4)),12));
    return ok && r == t ? 42 : 3;
}
