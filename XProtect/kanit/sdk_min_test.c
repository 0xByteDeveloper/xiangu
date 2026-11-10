// sdk_min_test.c — yalniz rol/ror/bswap/xchg (bellek erisimi YOK)
#include <stdio.h>
#include <stdlib.h>
static volatile unsigned int g_out;
static unsigned int sdk_min(unsigned int x, unsigned int y) {
    unsigned int t = x, t2 = y;
    __asm__ volatile(".byte 0xB8, 0xB5, 0xE1, 0x37, 0x9C" ::: "eax", "memory");
    __asm__ volatile(".byte 0xB8, 0x01, 0x00, 0x00, 0x00" ::: "eax", "memory");
    t = (t << 8) | (t >> 24);                 /* rol 8  */
    t = (t >> 4) | (t << 28);                 /* ror 4  */
    __asm__ volatile("bswap %0" : "+r"(t));
    __asm__ volatile("xchg %0, %1" : "+r"(t), "+r"(t2));
    t2 = (t2 << 12) | (t2 >> 20);             /* rol 12 */
    __asm__ volatile("xchg %0, %1" : "+r"(t), "+r"(t2));
    g_out = t;
    __asm__ volatile(".byte 0xB8, 0x8E, 0x2C, 0x7A, 0x3D" ::: "eax", "memory");
    return g_out;
}
static unsigned int rol32(unsigned int v, int n) { return (v << n) | (v >> (32 - n)); }
static unsigned int ror32(unsigned int v, int n) { return (v >> n) | (v << (32 - n)); }
static unsigned int bswap32(unsigned int v) { return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v>>8)&0xFF00)|(v>>24); }
int main(int argc, char** argv) {
    unsigned int x = (argc > 1) ? (unsigned)strtoul(argv[1], 0, 16) : 0x1234;
    unsigned int y = (argc > 2) ? (unsigned)strtoul(argv[2], 0, 16) : 0xA5A5A5A5;
    unsigned int r = sdk_min(x, y);
    /* app akisi: t=rol8,ror4,bswap; xchg(t,t2); t2=rol12(t2=x-chain); xchg -> t=rol12(x-chain) */
    unsigned int expect = rol32(bswap32(ror32(rol32(x, 8), 4)), 12);
    printf("OK min r=%08x beklenen=%08x %s\n", r, expect, r == expect ? "ESLESTI" : "FARKLI");
    return r == expect ? 42 : 3;
}
