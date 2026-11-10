/* vmharness.c — vm_core'u HOST'ta gerçek exe görüntüsü üzerinde koşturur.
 * Amaç: sdk_ext (movslq/SIB/NCALL'lu bölge) wild-write teşhisi — op/op iz.
 * Derleme: gcc -O0 -g -DVM_IS_X64 -DVM_TRACE -I. vm_core.c vmharness.c -o vmh
 * Koşu:    ./vmh <orijinal.exe> <korumali.exe> <stubRVA hex>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "vm_opcodes.h"

void vm_entry(void* c);   /* vm_core.c'den (SysV) */

#define IMG_BASE 0x140000000ULL
#define IMG_SIZE 0x100000ULL
#define STK_BASE 0x70000000ULL
#define STK_SIZE 0x100000ULL

typedef struct {
    unsigned long long bc, bc_len, key, base, regs[16], flags,
        push_area, init_rsp, extra_pop, done, scratch[8];
} CTX;

static unsigned int rd32(const unsigned char* b, unsigned long o) {
    return b[o] | (b[o+1] << 8) | (b[o+2] << 16) | ((unsigned)b[o+3] << 24);
}
static unsigned short rd16(const unsigned char* b, unsigned long o) {
    return (unsigned short)(b[o] | (b[o+1] << 8));
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "kullanim: vmh <orijinal.exe> <korumali.exe> <stubRVAhex>\n"); return 1; }
    /* 1) orijinal exe'yi 0x140000000'e BOLUM-BOLUM map et (VA=VA) */
    void* img = mmap((void*)IMG_BASE, IMG_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (img == MAP_FAILED) { perror("mmap img"); return 1; }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror("exe"); return 1; }
    fseek(f, 0, SEEK_END); long fsz = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char* buf = malloc(fsz); fread(buf, 1, fsz, f); fclose(f);
    unsigned long pe = rd32(buf, 0x3C);
    unsigned nsec = rd16(buf, pe + 6);
    unsigned optsz = rd16(buf, pe + 20);
    unsigned long sh = pe + 4 + 20 + optsz;
    memcpy(img, buf, sh + (unsigned long)nsec * 40 < IMG_SIZE ? sh + nsec * 40 : IMG_SIZE);
    for (unsigned i = 0; i < nsec; i++) {
        unsigned long o = sh + (unsigned long)i * 40;
        unsigned vsz = rd32(buf, o + 8), va = rd32(buf, o + 12),
                 rsz = rd32(buf, o + 16), ro = rd32(buf, o + 20);
        if (!rsz || !va) continue;
        unsigned long n = (unsigned long)vsz < rsz ? vsz : rsz;
        if ((unsigned long long)va + n > IMG_SIZE) n = IMG_SIZE - va;
        memcpy((unsigned char*)img + va, buf + ro, n);
        fprintf(stderr, "[harness] sec va=%#x rsz=%#x -> VA\n", va, rsz);
    }
    /* 2) korumali exe'den stub alanlarini coz */
    FILE* fg = fopen(argv[2], "rb");
    if (!fg) { perror("korunmus"); return 1; }
    fseek(fg, 0, SEEK_END); long gsz = ftell(fg); fseek(fg, 0, SEEK_SET);
    unsigned char* gb = malloc(gsz); fread(gb, 1, gsz, fg); fclose(fg);
    unsigned long pe2 = rd32(gb, 0x3C);
    unsigned nsec2 = rd16(gb, pe2 + 6);
    unsigned optsz2 = rd16(gb, pe2 + 20);
    unsigned long sh2 = pe2 + 4 + 20 + optsz2;
    unsigned stub_rva = strtoul(argv[3], 0, 16);
    long sraw = -1;
    for (unsigned i = 0; i < nsec2; i++) {
        unsigned long o = sh2 + (unsigned long)i * 40;
        unsigned vsz = rd32(gb, o + 8), va = rd32(gb, o + 12),
                 rsz = rd32(gb, o + 16), ro = rd32(gb, o + 20);
        if (va <= stub_rva && stub_rva < va + (vsz ? vsz : rsz)) {
            sraw = (long)ro + (stub_rva - va); break;
        }
    }
    if (sraw < 0) { fprintf(stderr, "stub raw yok\n"); return 1; }
    unsigned char* st = gb + sraw;
    unsigned long i1 = 0; while (memcmp(st + i1, "\x48\xB8", 2) && i1 < 0x80) i1++;
    unsigned long long key2; memcpy(&key2, st + i1 + 2, 8);
    unsigned long j1 = i1; while (memcmp(st + j1, "\x48\x05", 2) && j1 < 0x100) j1++;
    unsigned bc_rva; memcpy(&bc_rva, st + j1 + 2, 4);
    unsigned long k1 = i1 + 2; while (memcmp(st + k1, "\x48\xB8", 2) && k1 < 0x100) k1++;
    unsigned long long bc_len; memcpy(&bc_len, st + k1 + 2, 8);
    unsigned long m1 = 0; unsigned seed = 0;
    while (m1 < 0x100 && memcmp(st + m1, "\xC7\x84\x24\xE0", 4)) m1++;
    if (m1 < 0x100) memcpy(&seed, st + m1 + 7, 4);
    fprintf(stderr, "[harness] key2=%#llx bc_rva=%#x bc_len=%llu seed=%#x\n",
            key2, bc_rva, bc_len, seed);
    /* 3) bc'yi korumali imajdan VA'ya kopyala (görüntüde yazılabilir olsun) */
    long braw = -1;
    for (unsigned i = 0; i < nsec2; i++) {
        unsigned long o = sh2 + (unsigned long)i * 40;
        unsigned vsz = rd32(gb, o + 8), va = rd32(gb, o + 12),
                 rsz = rd32(gb, o + 16), ro = rd32(gb, o + 20);
        if (va <= bc_rva && bc_rva < va + (vsz ? vsz : rsz)) {
            braw = (long)ro + (bc_rva - va); break;
        }
    }
    if (braw < 0) { fprintf(stderr, "bc raw yok\n"); return 1; }
    memcpy((unsigned char*)img + bc_rva, gb + braw, bc_len);
    /* 4) yigin */
    void* stk = mmap((void*)STK_BASE, STK_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (stk == MAP_FAILED) { perror("mmap stk"); return 1; }
    /* 5) ctx */
    static CTX ctx; memset(&ctx, 0, sizeof ctx);
    static unsigned long long pa[16];
    unsigned long long argv_fake[2] = { 0, 0 };
    pa[0] = 0;                              /* rax */
    pa[1] = 1;                              /* rcx = argc */
    pa[2] = (unsigned long long)argv_fake;  /* rdx */
    pa[3] = 0x11111111;                     /* rbx */
    pa[4] = 0x22222222;                     /* rbp */
    pa[5] = 0x33333333;                     /* rsi */
    pa[6] = 0x44444444;                     /* rdi */
    for (int i = 7; i < 15; i++) pa[i] = 0x5555000000ULL + i;
    ctx.bc = IMG_BASE + bc_rva;
    ctx.bc_len = bc_len;
    ctx.key = key2;                          /* stub imm (key^K2) */
    ctx.base = IMG_BASE;
    ctx.push_area = (unsigned long long)pa;
    ctx.init_rsp = STK_BASE + STK_SIZE - 0x1000;
    ctx.scratch[3] = seed;
    ctx.extra_pop = 0;
    *(unsigned long long*)(ctx.init_rsp - 8) = 0xDEADC0DE;
    fprintf(stderr, "[harness] koşu başlıyor: init_rsp=%#llx\n", ctx.init_rsp);
    vm_entry(&ctx);
    fprintf(stderr, "[harness] done=%llu rax=%#llx rcx=%#llx\n",
            ctx.done, ctx.regs[0], ctx.regs[1]);
    return 0;
}
