// test_deepvm.c — 0.9.0-DeepVM kapsam testi (x86 PE32 + x64)
// Her fonksiyon yeni opcode siniflarini ham __asm__ ile KULLANIR;
// deterministik checksum dondurur. Korunmus cikti == orijinal cikti
// (bayt-bayt) = lifter+interpreter kaniti.
// Derleme: zig cc -target x86-windows-gnu -O1 -fno-omit-frame-pointer
#include <stdio.h>
#include <windows.h>

/* 1) ADC/SBB zinciri — CF tasiyan coklu-duyarlik toplama (bayrak derinligi) */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_adc(unsigned int a, unsigned int b) {
    unsigned int r;
    __asm__ volatile(
        "add %2, %0          \n\t"   /* a += b (CF kurulur) */
        "adc $0x22222222, %0 \n\t"   /* a += 0x22222222 + CF */
        "sbb %1, %1          \n\t"   /* r = -CF (0 veya -1) */
        : "+r"(a), "=&r"(r) : "r"(b) : "cc");
    return a ^ r;
}

/* 2) PUSHF/POPF + LAHF/SAHF — bayrak imaji yuvarlama (x86: lahf/sahf; x64: pushfq) */
#ifdef __i386__
__attribute__((noinline)) __declspec(dllexport) unsigned int t_pushf(unsigned int a) {
    unsigned int f1, ah;
    __asm__ volatile(
        "cmp $0x41, %2   \n\t"      /* CF/ZF/SF/PF/AF kur (0x41: parite cift) */
        "pushf           \n\t"
        "pop  %0         \n\t"      /* f1 = EFLAGS imaji */
        "lahf            \n\t"
        "movzbl %%ah, %1 \n\t"      /* ah = SF:ZF:0:AF:0:PF:1:CF */
        "add  $0x7F, %1  \n\t"      /* yeni bayrak degeri */
        "movb %b1, %%ah  \n\t"
        "sahf            \n\t"      /* AH'dan bayraklara */
        "pushf           \n\t"
        "pop  %0         \n\t"      /* f1 = yeni imaj */
        : "=&r"(f1), "=&r"(ah) : "r"(a) : "cc", "ah");
    /* Unicorn pushf imaji IF(0x200)/IOPL normalize eder — maskeli eslesme */
    return (f1 & 0xFFFFFDFFu) ^ (ah << 16);
}
#else
__attribute__((noinline)) __declspec(dllexport) unsigned int t_pushf(unsigned long long a) {
    unsigned long long f1, f2;
    __asm__ volatile(
        "cmp $0x41, %1   \n\t"
        "pushfq          \n\t"
        "pop  %0         \n\t"
        "add $0x7F, %1   \n\t"
        "pushfq          \n\t"
        "pop  %0         \n\t"
        : "=&r"(f1), "+r"(a) : : "cc");
    (void)f2;
    return (unsigned int)(f1 ^ (f1 >> 32));
}
#endif

/* 3) CLC/STC/CMC + JC/JNC akisi */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_flagops(unsigned int a) {
    unsigned int r = 0;
    __asm__ volatile(
        "clc             \n\t"
        "jc 1f           \n\t"
        "add $0x10, %0   \n\t"
        "1: stc          \n\t"
        "jnc 2f          \n\t"
        "add $0x20, %0   \n\t"
        "2: clc          \n\t"
        "cmc             \n\t"
        "jnc 3f          \n\t"
        "add $0x40, %0   \n\t"
        "3:              \n\t"
        : "+r"(r) : "r"(a) : "cc");
    return r + a;
}

/* 4) MUL/DIV (dword) — RDX:EAX genis carpim/bolum */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_muldiv(unsigned int a, unsigned int b) {
    unsigned int lo, hi, q, rm;
    __asm__ volatile(
        "mov %2, %%eax   \n\t"
        "mul %3          \n\t"      /* EDX:EAX = a*b */
        : "=&a"(lo), "=&d"(hi) : "r"(a), "r"(b) : "cc");
    __asm__ volatile(
        "mov %2, %%eax   \n\t"
        "xor %%edx, %%edx\n\t"
        "div %3          \n\t"      /* EAX = b/0:EDX=0 / a */
        : "=&a"(q), "=&d"(rm) : "r"(b), "r"(a) : "cc");
    return (lo ^ hi ^ q ^ rm);
}

/* 5) BT ailesi — CF okuma + bit yazma */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_bt(unsigned int a) {
    unsigned int r = 0, v = 0x00000240;
    __asm__ volatile(
        "bt  %2, %1      \n\t"      /* bit 9 (0x200) -> CF=1 */
        "jc  1f          \n\t"
        "add $0x01, %0   \n\t"
        "1: bt  $5, %1   \n\t"      /* imm8 bit 5 (0x20) -> CF=1 */
        "jnc 2f          \n\t"
        "add $0x02, %0   \n\t"
        "2: bts %2, %1   \n\t"      /* bit9 zaten 1 */
        "btr $6, %1      \n\t"      /* bit6 (0x40) temizle */
        "btc %2, %1      \n\t"      /* bit9 temizle */
        "bt  %2, %1      \n\t"      /* simdi CF=0 */
        "jc  3f          \n\t"
        "add $0x04, %0   \n\t"
        "3:              \n\t"
        : "+r"(r), "+r"(v) : "r"(9u) : "cc");
    return r ^ v;
}

/* 6) SHLD/SHRD — imm8 ve CL formlari */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_shld(unsigned int a, unsigned int b) {
    unsigned int r1, r2;
    __asm__ volatile(
        "mov %2, %0      \n\t"
        "shld $7, %3, %0 \n\t"      /* r1 = a<<7 | b>>25 */
        "mov %3, %1      \n\t"
        "shrd $9, %2, %1 \n\t"      /* r2 = b>>9 | a<<23 */
        "mov $4, %%ecx   \n\t"
        "shld %%cl, %3, %0 \n\t"    /* cl formu */
        : "=&r"(r1), "=&r"(r2) : "r"(a), "r"(b) : "ecx", "cc");
    return r1 ^ r2;
}

/* 7) LOOP/JECXZ — sayac dongusu */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_loop(unsigned int n) {
    unsigned int r = 0;
    __asm__ volatile(
        "mov %1, %%ecx   \n\t"
        "xor %0, %0      \n\t"
        "1: add $3, %0   \n\t"
        "loop 1b         \n\t"
        "jecxz 2f        \n\t"
        "add $0x77, %0   \n\t"      /* ecx!=0 (n>0) ise ek */
        "2:              \n\t"
        : "=&r"(r) : "r"(n) : "ecx", "cc");
    return r;
}

/* 8) XADD — atomik toplama semantigi */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_xadd(unsigned int a, unsigned int b) {
    unsigned int r = 0;
    __asm__ volatile(
        "xadd %2, %0     \n\t"      /* r = a; a = a + b (b degismez... r=eski a) */
        : "+r"(a) : "r"(0), "r"(b) : "cc");
    r = a;
    return r;
}

/* 9) REP string — movsb/stosb/scasb + STD/CLD (yalniz x86; x64'te edi-kesme) */
#ifdef __i386__
__attribute__((noinline)) __declspec(dllexport) unsigned int t_str(unsigned int seed) {
    char buf[32];
    unsigned int r = 0;
    __asm__ volatile(
        "cld                 \n\t"
        "lea 4(%1), %%edi    \n\t"
        "mov $8, %%ecx       \n\t"
        "mov $0xAB, %%eax    \n\t"
        "rep stosb           \n\t"      /* buf[4..11] = 0xAB */
        "lea 4(%1), %%esi    \n\t"
        "lea 12(%1), %%edi   \n\t"
        "mov $8, %%ecx       \n\t"
        "rep movsb           \n\t"      /* buf[12..19] = buf[4..11] */
        "mov $0xAB, %%eax    \n\t"
        "lea 12(%1), %%edi   \n\t"
        "mov $8, %%ecx       \n\t"
        "repe scasb          \n\t"      /* 8 eslesme -> ecx=0, ZF=1 */
        "jne 1f              \n\t"
        "add $0x11, %0       \n\t"
        "1: std              \n\t"
        "mov $0x5C, %%eax    \n\t"
        "lea 19(%1), %%edi   \n\t"
        "mov $8, %%ecx       \n\t"
        "rep stosb           \n\t"      /* geriye dogru 0x5C */
        "cld                 \n\t"
        : "+r"(r) : "r"(buf) : "eax", "ecx", "esi", "edi", "cc", "memory");
    { int i; for (i = 4; i < 12; i++) r = r * 31 + (unsigned char)buf[i]; }
    { int i; for (i = 12; i < 20; i++) r = r * 31 + (unsigned char)buf[i]; }
    return r ^ seed;
}
#else
__attribute__((noinline)) __declspec(dllexport) unsigned int t_str(unsigned long long seed) {
    return (unsigned int)(seed * 0x9E3779B97F4A7C15ULL >> 32) ^ 0x5E5E5E5Eu; /* x64: yer tutucu (movs yolu x86 Unicorn'da kanitlanir) */
}
#endif

/* 10) FS/GS:[self] — TEB self-isaretcisi (segment semantigi) */
#ifdef __i386__
__attribute__((noinline)) __declspec(dllexport) unsigned int t_fs(void) {
    unsigned int teb = 0;
    __asm__ volatile("mov %%fs:0x18, %0" : "=r"(teb));
    return (teb == 0) ? 0xDEADBEEF : (teb & 0xFFFF);
}
#else
__attribute__((noinline)) __declspec(dllexport) unsigned int t_fs(void) {
    unsigned long long teb = 0;
    __asm__ volatile("mov %%gs:0x30, %0" : "=r"(teb));
    return (unsigned int)((teb == 0) ? 0xDEADBEEFULL : (teb & 0xFFFF));
}
#endif

/* 11) PF/JNP — parite bayragi GERCEK kullanım (0.8.5'te hep "not parity") */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_parity(unsigned int a) {
    unsigned int r = 0;
    __asm__ volatile(
        "xor %1, %1      \n\t"
        "cmp $0x000000FF, %1 \n\t"  /* 8 bit -> parite CIFt (PF=1) */
        "jnp 1f          \n\t"
        "add $0xAA, %0   \n\t"      /* PF=1 yolu */
        "1: cmp $0x3, %1 \n\t"      /* 2 bit -> parite TEK (PF=0) */
        "jp  2f          \n\t"
        "add $0x55, %0   \n\t"      /* PF=0 yolu */
        "2:              \n\t"
        : "+r"(r) : "r"(a) : "cc");
    return r;
}

/* 12) IMUL bellek formu (0F AF) + negatif degerler */
__attribute__((noinline)) __declspec(dllexport) unsigned int t_imulmem(unsigned int a) {
    unsigned int arr[2] = { (unsigned int)(-(int)a), 7u };
    unsigned int r;
    __asm__ volatile(
        "imul 4(%1), %0  \n\t"      /* r = r * arr[1] */
        "imul (%1), %0   \n\t"      /* r = r * -a */
        : "+r"(a) : "r"(arr) : "cc");
    r = a;
    return r;
}

/* 13) 8-bit MUL/DIV (AX=AL*r8) — 0.9.0 genis aritmetik (yalniz x86: cl formlari) */
#ifdef __i386__
__attribute__((noinline)) __declspec(dllexport) unsigned int t_mul8(unsigned int a) {
    unsigned int r1, r2;
    __asm__ volatile(
        "mov $0x12, %%eax \n\t"
        "mov $0x33, %%ecx \n\t"
        "mul %%cl         \n\t"     /* AX = 0x12*0x33 = 0x396 */
        "movzwl %%ax, %0  \n\t"
        "mov $0x0807, %%ax\n\t"     /* AL=7 AH=8 */
        "mov $0x10, %%cl  \n\t"
        "div %%cl         \n\t"     /* AL = 0x0807/0x10, AH = kalan */
        "movzwl %%ax, %1  \n\t"
        : "=&r"(r1), "=&r"(r2) : "r"(a) : "eax", "ecx", "cc");
    return r1 ^ r2;
}
#else
__attribute__((noinline)) __declspec(dllexport) unsigned int t_mul8(unsigned long long a) {
    unsigned int r;
    __asm__ volatile(
        "mov $0x1234, %%eax \n\t"
        "mov $0x5678, %%ecx \n\t"
        "mul %%cx           \n\t"   /* x64: 16-bit mul gecerli (AX = AX*CX) */
        : "=&a"(r) : : "ecx", "cc");
    (void)a;
    return r;
}
#endif

__attribute__((noinline)) __declspec(dllexport) unsigned int t_mulonly(unsigned int a);
__attribute__((noinline)) __declspec(dllexport) unsigned int t_divonly(unsigned int a);

int main(void) {
    FILE* f = fopen("Z:\\home\\z\\my-project\\scripts\\xb090\\deepvm_sonuc.txt", "w");
    if (!f) f = fopen("C:\\deepvm_sonuc.txt", "w");
    if (!f) return 1;
#ifdef __i386__
    fprintf(f, "ARCH x86\n");
#else
    fprintf(f, "ARCH x64\n");
#endif
    fprintf(f, "DEEPVM %08X\n", t_adc(0xFFFFFF01u, 2u));
    fprintf(f, "DEEPVM %08X\n", t_pushf(0x41u));
    fprintf(f, "DEEPVM %08X\n", t_flagops(7u));
    fprintf(f, "DEEPVM %08X\n", t_muldiv(0x12345u, 0x6789u));
    fprintf(f, "DEEPVM %08X\n", t_bt(0u));
    fprintf(f, "DEEPVM %08X\n", t_shld(0x12345678u, 0x9ABCDEF0u));
    fprintf(f, "DEEPVM %08X\n", t_loop(5u));
    fprintf(f, "DEEPVM %08X\n", t_xadd(0x40u, 0x22u));
    fprintf(f, "DEEPVM %08X\n", t_str(0x5Eu));
    fprintf(f, "DEEPVM %08X\n", t_fs());
    fprintf(f, "DEEPVM %08X\n", t_parity(0u));
    fprintf(f, "DEEPVM %08X\n", t_imulmem(3u));
    fprintf(f, "DEEPVM %08X\n", t_mul8(0u));
    fprintf(f, "IX %08X\n", t_mulonly(0u)); fprintf(f, "IX %08X\n", t_divonly(0u)); fprintf(f, "SON\n");
    fclose(f);
    return 42;
}

/* izole: yalniz 8-bit mul */
__attribute__((noinline)) unsigned int t_mulonly(unsigned int a) {
    unsigned int r1;
    __asm__ volatile(
        "mov $0x12, %%eax \n\t"
        "mov $0x33, %%ecx \n\t"
        "mul %%cl         \n\t"
        "movzwl %%ax, %0  \n\t"
        : "=&r"(r1) : "r"(a) : "eax", "ecx", "cc");
    return r1;
}
/* izole: yalniz 8-bit div */
__attribute__((noinline)) unsigned int t_divonly(unsigned int a) {
    unsigned int r1;
    __asm__ volatile(
        "mov $0x0807, %%ax\n\t"
        "mov $0x10, %%cl  \n\t"
        "div %%cl         \n\t"
        "movzwl %%ax, %0  \n\t"
        : "=&r"(r1) : "r"(a) : "eax", "ecx", "cc");
    return r1;
}
