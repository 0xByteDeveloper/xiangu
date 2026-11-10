// ============================================================
// vm_core.c — MiniVM: bayt-kod yorumlayici (korumali PE'ye gomulur)
// DERLEME (pozisyon-bagimsiz duz ikili):
//   x64: gcc -O2 -ffreestanding -fno-stack-protector -fno-jump-tables
//        -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident
//        -DVM_IS_X64 [-DVM_WINDOWS] -c vm_core.c
//        ld -Ttext=0x100000 -e vm_entry vm_core.o -o vm.elf
//        objcopy -O binary vm.elf vm.bin
//   x86: aynisi -DVM_IS_X86 (i386 hedefi)
// KURAL: libc YOK, switch/jump-table YOK, string literal YOK.
// Bayt-kod akis formati: vm_opcodes.h (encoder ile birebir ayna)
// ============================================================
#include "vm_opcodes.h"
#ifdef VM_TRACE
/* 0.5.0 hata ayiklama izi — tputn typedef'lerden sonra tanimlanir */
static int vm_trace_on = 1;
#define VM_TRACE_CHECK()
#else
#define VM_TRACE_CHECK()
#define tputn(p, a, b)
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;
typedef long long i64;

#ifdef VM_TRACE
static void tputn(const char* pre, u64 v, u64 v2) {
    char b[128]; int n = 0; int k;
    while (*pre && n < 40) b[n++] = *pre++;
    b[n++] = ' ';
    b[n++] = '0'; b[n++] = 'x';
    for (k = 15; k >= 0; k--) { char c = (char)((v >> (4 * k)) & 0xF); b[n++] = (char)(c < 10 ? '0' + c : 'a' + c - 10); }
    b[n++] = ' ';
    b[n++] = '0'; b[n++] = 'x';
    for (k = 15; k >= 0; k--) { char c = (char)((v2 >> (4 * k)) & 0xF); b[n++] = (char)(c < 10 ? '0' + c : 'a' + c - 10); }
    b[n++] = '\n';
    __asm__ volatile("syscall" : : "a"((u64)1), "D"((u64)2), "S"((u64)(unsigned long)b), "d"((u64)n) : "rcx", "r11", "memory");
}
#endif

#ifdef VM_TRACE
static int open2(const char* path) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"((u64)2), "D"(path),
                     "S"((u64)(0x41 /*O_WRONLY|O_CREAT*/)), "d"((u64)0644) : "rcx", "r11", "memory");
    return (int)r;
}
static void wr2(int fd, const void* b, unsigned n) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"((u64)1), "D"((u64)fd),
                     "S"(b), "d"((u64)n) : "rcx", "r11", "memory");
}
static void close2(int fd) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"((u64)3), "D"((u64)fd) : "rcx", "r11", "memory");
}
#endif

typedef struct {
    u64 bc;         // +0x00 bayt-kod gostergesi
    u64 bc_len;     // +0x08
    u64 key;        // +0x10 rolling anahtar
    u64 base;       // +0x18 modul image base
    u64 regs[16];   // +0x20
    u64 flags;      // +0xA0
    u64 push_area;  // +0xA8 stub push alani
    u64 init_rsp;   // +0xB0
    u64 extra_pop;  // +0xB8 (x86 stdcall)
    u64 done;       // +0xC0  0=devam 1=ret 2=zehir
    u64 scratch[8]; // +0xC8
} VMContext;

#if defined(VM_IS_X64)
  #define PUSHSZ 8
  #define NMAP 15
  #define VM_DELTA VM_DELTA64
#elif defined(VM_IS_X86)
  #define PUSHSZ 4
  #define NMAP 8
  #define VM_DELTA VM_DELTA86
#endif
// pushmap fonksiyon icinde kurulur (statik veri yok -> tam PIC)

static void* u2p(u64 v) { return (void*)(unsigned long)v; }

// --- sifreli bayt akisi okuyucu ---
typedef struct { u8* bc; u64 key; u64 ic; } STRM;

static u8 sf8(STRM* s) {
    /* 0.8.0 crypt_command: CFB geribesleme — akis bir onceki SIFRELI
       bayta bagli (saf (key, prev, pos) fonksiyonu; JMP guvenli). */
    u8 v = s->bc[s->ic] ^ VM_KSC(s->key, s->ic ? s->bc[s->ic - 1] : 0, s->ic);
    s->ic++;
    return v;
}
static u32 sf32(STRM* s) {
    u32 v = 0; int i;
    for (i = 0; i < 4; i++) v |= ((u32)sf8(s)) << (8*i);
    return v;
}
static u64 sf64(STRM* s) {
    u64 v = 0; int i;
    for (i = 0; i < 8; i++) v |= ((u64)sf8(s)) << (8*i);
    return v;
}

// --- bellek ---
static u64 mrd(u64 a, int sz) {
    u8* p = (u8*)u2p(a); u64 v = 0; int i;
    for (i = 0; i < sz; i++) v |= ((u64)p[i]) << (8*i);
    return v;
}
static void mwr(u64 a, int sz, u64 v) {
    u8* p = (u8*)u2p(a); int i;
    for (i = 0; i < sz; i++) p[i] = (u8)(v >> (8*i));
}

// --- register ---
static u64 rget(VMContext* c, int enc, int sz) {
    if (enc >= 16) return (c->regs[enc-16] >> 8) & 0xFF;
    if (sz == 1) return c->regs[enc] & 0xFF;
    if (sz == 2) return c->regs[enc] & 0xFFFF;
    if (sz == 4) return c->regs[enc] & 0xFFFFFFFF;
    return c->regs[enc];
}
static void rset(VMContext* c, int enc, int sz, u64 v) {
    if (enc >= 16) {
        c->regs[enc-16] = (c->regs[enc-16] & ~0xFF00ULL) | ((v & 0xFF) << 8);
        return;
    }
    if (sz == 1)      c->regs[enc] = (c->regs[enc] & ~0xFFULL) | (v & 0xFF);
    else if (sz == 2) c->regs[enc] = (c->regs[enc] & ~0xFFFFULL) | (v & 0xFFFF);
    else if (sz == 4) c->regs[enc] = v & 0xFFFFFFFF;   /* x64: 32-bit yazim ust 32 biti SIFIRLAR */
    else              c->regs[enc] = v;
}

static u64 szmask(int sz) {
    if (sz == 1) return 0xFFULL;
    if (sz == 2) return 0xFFFFULL;
    if (sz == 4) return 0xFFFFFFFFULL;
    return ~0ULL;
}

// MEM operandini akistan oku ve gercek adresi coz.
// Encoder sirasi: form:1 -> DISP: disp32 | SIB: base,idx,scale,disp32
static u64 mem_addr(STRM* s, VMContext* c) {
    int form = sf8(s);
    if (form == MEMF_DISP) {
        return c->base + (u64)(u32)sf32(s);
    } else {
        int b = sf8(s), ix = sf8(s), sc = sf8(s);
        u32 d = sf32(s);
        /* MEMF_SIB_IMG: disp image-RVA'dir (x86 mutlak adresleme) ->
           c->base eklenir. MEMF_SIB (eski): x64 yigin SIB'i -> disp ham. */
        u64 img = (form == MEMF_SIB_IMG) ? c->base : 0;
        u64 a;
        if (b == SIB_NOBASE)      a = img + (u64)(i64)(int)d;
        else if (b == REG_RSP)    a = c->regs[REG_RSP] - (u64)VM_DELTA + (u64)(i64)(int)d;
        else                      a = img + c->regs[b] + (u64)(i64)(int)d;
        if (ix != SIB_NOBASE) a += c->regs[ix] << sc;
#ifdef VM_TRACE
        if (vm_trace_on) tputn("[mem] a=", a, ((u64)(b & 0xFF) << 8) | (ix & 0xFF));
#endif
        return a;
    }
}

static int sext32(i64* out, u32 v) { *out = (i64)(int)v; return 0; }

// --- ALU + bayraklar ---
static u64 do_alu(int alu, u64 a, u64 b, int sz, u64* fl) {
    u64 m = szmask(sz);
    int sb = sz * 8 - 1;
    u64 r = 0, f = 0;
    if (alu == ALU_ADD) {
        u64 s = a + b;
        r = s & m;
        if (sz == 8) { if (s < a) f |= FCF; }
        else         { if (s & ~m) f |= FCF; }
        if (((a ^ r) & (b ^ r)) >> sb & 1) f |= FOF;
    } else if (alu == ALU_SUB || alu == ALU_CMP) {
        r = (a - b) & m;
        if (a < b) f |= FCF;
        if (((a ^ b) & (a ^ r)) >> sb & 1) f |= FOF;
    } else if (alu == ALU_AND || alu == ALU_TEST) {
        r = a & b;
    } else if (alu == ALU_OR) {
        r = a | b;
    } else {
        r = a ^ b;
    }
    if (r == 0) f |= FZF;
    if ((r >> sb) & 1) f |= FSF;
    *fl = f;
    return r;
}

static int cond_eval(VMContext* c, int cc) {
    u64 f = c->flags;
    int C = (f & FCF) != 0, Z = (f & FZF) != 0, S = (f & FSF) != 0, O = (f & FOF) != 0;
    if (cc == CC_O)  return O;
    if (cc == CC_NO) return !O;
    if (cc == CC_B)  return C;
    if (cc == CC_AE) return !C;
    if (cc == CC_E)  return Z;
    if (cc == CC_NE) return !Z;
    if (cc == CC_BE) return (C | Z);
    if (cc == CC_A)  return !(C | Z);
    if (cc == CC_S)  return S;
    if (cc == CC_NS) return !S;
    if (cc == CC_P)  return 0;    /* PF izlenmiyor (float sonrasi kullanilmaz) */
    if (cc == CC_NP) return 1;
    if (cc == CC_L)  return (S != O);
    if (cc == CC_GE) return (S == O);
    if (cc == CC_LE) return (Z | (S != O));
    if (cc == CC_G)  return (!Z) && (S == O);
    return 0;
}

static u64 do_shift(int shi, u64 a, int n, int sz, u64* fl) {
    int sbits = sz * 8;
    u64 r = a, f = 0;
    /* x86: sayac 8/16/32-bitte &31, 64-bitte &63 ile maskelenir */
    n &= (sbits >= 64) ? 63 : 31;
    if (n) {
        if (shi == SHI_ROL) {
            r = ((a << n) | (a >> ((sbits - n) & (sbits - 1)))) & szmask(sz);
            f |= r & 1;                      /* ROL: yalniz CF (x86) */
            *fl = f;
            return r;
        }
        if (shi == SHI_ROR) {
            r = ((a >> n) | (a << ((sbits - n) & (sbits - 1)))) & szmask(sz);
            f |= (r >> (sbits - 1)) & 1;     /* ROR: yalniz CF */
            *fl = f;
            return r;
        }
        if (shi == SHI_SHL) {
            if (n <= sbits) { if ((a >> (sbits - n)) & 1) f |= FCF; }
            r = (a << n) & szmask(sz);
        } else if (shi == SHI_SHR) {
            if (n <= sbits) { if ((a >> (n - 1)) & 1) f |= FCF; }
            r = (a >> n) & szmask(sz);
        } else {
            i64 sv = (i64)(a << (64 - sbits)) >> (64 - sbits);
            if (n <= sbits) { if (((u64)sv >> (n-1)) & 1) f |= FCF; }
            r = ((u64)(sv >> n)) & szmask(sz);
        }
        if (r == 0) f |= FZF;
        if ((r >> (sbits-1)) & 1) f |= FSF;
    }
    *fl = f;
    return r;
}

#if defined(VM_IS_X86)
// 0.5.1 — x86 dogal cagri, iki kok-duzeltme ile:
// (1) 0.5.0 "yigin bozumu": esp_after "=m" BELLEK operandiydi. -O2'de GCC/zig
//     esp-tabanli cerceve kullanir -> "movl %%esp, -N(%%esp)" yazimi o anda
//     esp = VM yigini OLDUGU ICIN VM yiginina yazar, gercek degisken 0 kalir
//     -> regs[RSP] = 0 + VM_DELTA = 0x420 -> ilk push 0xFFFFFFFC'ye yazar.
//     artik esp_after kayit cikisi (esi uzerinden).
// (2) ARGUMAN-KAYITCI AKTARIMI (0.5.0'da hic yoktu): uyumlu callee gelen
//     ecx/edx'i OKUR (thiscall ecx=this, fastcall ecx,edx). VM kayitcilari
//     bellekte oldugu icin gercek CPU ecx/edx coputti -> callee cop okuyor
//     -> wild ret. Cagridan ONCE VM eax/ecx/edx gercek CPU'ya yuklenir;
//     cagridan SONRA callee'nin eax/ecx/edx'i VM'e geri yazilir (gercek CPU
//     aynasi). ebx/esi/edi/ebp bilincli YUKLENMEZ: callee-saved'dir, uyumlu
//     callee gelen degerini OKUYAMAZ (Win32 ABI); natcall onlari korur.
static void vm_natcall86(u64* regs, u32 t, u32 vm_rsp, u64* out_ret, u64* out_rsp) {
    u32 ret = 0, c_ecx = 0, c_edx = 0, s_out = t;   /* s_out girisi = hedef */
    register u32 vrsp __asm__("edi") = (u32)vm_rsp; /* VM yigini (in-out)   */
    __asm__ volatile(
        "movl 0x00(%[rg]), %%eax  \n\t"   /* eax = VM eax (regs[0].lo) */
        "movl 0x08(%[rg]), %%ecx  \n\t"   /* ecx = VM ecx (regs[1].lo) */
        "movl 0x10(%[rg]), %%edx  \n\t"   /* edx = VM edx (regs[2].lo) */
        "pushl %%ebx              \n\t"   /* ebx kaydet (C-caller callee-saved) */
        "movl %%esp, %%ebx        \n\t"   /* ebx = gercek esp */
        "movl %[vr], %%esp        \n\t"   /* esp = VM yigini (arglar burada) */
        "call  *%[sp]             \n\t"   /* esi = hedef (s_out girdisi) */
        "movl %%esp, %[sp]        \n\t"   /* callee sonrasi esp (stdcall) */
        "movl %%ebx, %%esp        \n\t"   /* gercek esp geri */
        "popl %%ebx               \n\t"   /* ebx geri (callee zaten korudu) */
        : "=&a"(ret), "=&c"(c_ecx), "=&d"(c_edx), [sp] "+S"(s_out), [vr] "+D"(vrsp)
        : [rg] "r"(regs)
        : "memory", "cc");
    regs[0] = (u64)ret;                /* callee eax = donus degeri        */
    regs[1] = (u64)c_ecx;              /* callee ecx (gercek CPU aynasi)   */
    regs[2] = (u64)c_edx;              /* callee edx                       */
    *out_ret = (u64)ret;
    *out_rsp = (u64)s_out;             /* callee sonrasi esp               */
}
#endif

#ifdef VM_IS_X64
// Native cagri. VM_WINDOWS: Win64 ABI (rcx,rdx,r8,r9). VM_SYSV_NATCALL:
// Linux test duzenegi (rdi,rsi,rdx,rcx,r8,r9).
#ifdef VM_SYSV_NATCALL
static u64 vm_natcall(u64 t, const u64* regs) {
    u64 ret;
    register u64 va7 __asm__("rdi") = regs[7];
    register u64 va6 __asm__("rsi") = regs[6];
    register u64 va2 __asm__("rdx") = regs[2];
    register u64 va1 __asm__("rcx") = regs[1];
    register u64 vr8 __asm__("r8")  = regs[8];
    register u64 vr9 __asm__("r9")  = regs[9];
    register u64 vt  __asm__("r11") = t;
    /* KRITIK: callee (SysV) rax,rcx,rdx,rsi,rdi,r8-r11'i EZER. "+X" bildirimi
       olmadan derleyici canli degerleri bu register'larda tutar -> callee
       ezer -> wild pointer. Ayrica cagri oncesi rsp 16-hizali OLMALI (SysV +
       Win64 ayni kural): hizali degilse callee'deki movaps/sse cerceveleri
       SIGSEGV verir. rsp eski degerine r10 uzerinden geri getirilir. */
    __asm__ volatile(
        "mov %%rsp, %%rbx \n\t"
        "and $-16, %%rsp \n\t"
        "sub $0x20, %%rsp \n\t"
        "call *%%r11 \n\t"
        "mov %%rbx, %%rsp \n\t"
        : "=a"(ret), "+D"(va7), "+S"(va6), "+d"(va2), "+c"(va1),
          "+r"(vr8), "+r"(vr9), "+r"(vt)
        :
        : "memory", "cc", "rbx", "r10");
    return ret;
}
#else
static u64 vm_natcall(u64 t, const u64* regs) {
    u64 ret;
    register u64 va0 __asm__("rax") = regs[0];
    register u64 va1 __asm__("rcx") = regs[1];
    register u64 va2 __asm__("rdx") = regs[2];
    register u64 vr8 __asm__("r8")  = regs[8];
    register u64 vr9 __asm__("r9")  = regs[9];
    register u64 vt  __asm__("r11") = t;
    /* KRITIK: callee (Win64) rax,rcx,rdx,r8,r9,r10,r11'i EZER (volatile).
       "+X" bildirimi yoksa derleyici canli degerleri bu register'larda
       tutabilir ve callee ezer -> bc isaretcisi bozulur -> wild read.
       Ayrica rsp 16-hizalanmali: hizasizsa callee'nin movaps'i SIGSEGV. */
    __asm__ volatile(
        "mov %%rsp, %%rbx \n\t"
        "and $-16, %%rsp \n\t"
        "sub $0x20, %%rsp \n\t"
        "call *%%r11 \n\t"
        "mov %%rbx, %%rsp \n\t"
        : "+a"(ret), "+c"(va1), "+d"(va2), "+r"(vr8), "+r"(vr9), "+r"(vt)
        :
        : "memory", "cc", "rbx", "r10");
    (void)va0;
    return ret;
}
#endif
#endif

// --- yorumlayici ---
static void vm_run(VMContext* c) {
    STRM s;
    s.bc  = (u8*)u2p(c->bc);
    s.key = c->key;
    s.ic  = 0;

    VM_TRACE_CHECK();
    /* 0.8.0 polimorfik isleyiciler: fonksiyon-bazina opcode permütasyonu.
       Stub tohumu ctx->scratch[3]'e yazdi (fonksiyon RVA'si); ayni
       vm_perm_tab ile raw->gercek tablosu turetilir. */
    unsigned char itab[VM_OPN];
    vm_perm_tab((u32)c->scratch[3], itab, 1);
#ifdef VM_TRACE
    if (vm_trace_on) {
        tputn("[bc] len=", c->bc_len, (u64)(unsigned long)c->bc);
        {
            /* tam PLAIN akis (sf8 ile birebir cozum) + itab -> dosya */
            int fd = open2("/tmp/plain.bin");
            if (fd >= 0) {
                unsigned di;
                for (di = 0; di < (unsigned)c->bc_len; di++) {
                    unsigned char raw = ((const unsigned char*)u2p(c->bc))[di];
                    unsigned char pv = di ? ((const unsigned char*)u2p(c->bc))[di - 1] : 0;
                    unsigned char p1 = (unsigned char)(raw ^ VM_KSC(c->key, pv, di));
                    wr2(fd, &p1, 1);
                }
                close2(fd);
            }
            fd = open2("/tmp/itab.bin");
            if (fd >= 0) { wr2(fd, itab, VM_OPN); close2(fd); }
        }
    }
#endif
    /* 0.8.0 adim butcesi: CFB tamper-yayilimi bozuk akisi JUMP dongusune
       cevirebilir -> butcesiz while sonsuz aski (DoS sinifi) demek.
       Butce 1<<24 (16.7M adim): meşru lifted donguler (kucuk fonksiyon
       govdesi) asla ulasamaz; tamper-derailment sinirli surede durur. */
    {
        u64 budget = (u64)1 << 24;
        while (!c->done) {
            if (budget-- == 0) { c->done = 1; break; }
        u8 op = itab[sf8(&s)];
        int alu, shi, cc, ud, sz, ssz, r1, r2, form, n;
        u32 v32;
        u64 a, b, r, addr, fl;
        i64 sv;
            c->scratch[0] = op;        /* hata ayiklama izi */
            c->scratch[1] = s.ic;
            c->scratch[2] = s.key;
#ifdef VM_TRACE
        if (vm_trace_on) tputn("[vm] ic=", s.ic, ((u64)op << 56) | s.key);
#endif

        if (op == OP_NOP) {
            /* junk */

        } else if (op == OP_MOV_RR) {
            sz = sf8(&s); r1 = sf8(&s); r2 = sf8(&s);
            rset(c, r1, sz, rget(c, r2, sz));

        } else if (op == OP_MOV_RI) {
            sz = sf8(&s); r1 = sf8(&s); sext32(&sv, sf32(&s));
            rset(c, r1, sz, (u64)sv);

        } else if (op == OP_MOV_RI64) {
            r1 = sf8(&s);
            rset(c, r1, 8, sf64(&s));

        } else if (op == OP_CMOV) {
            cc = sf8(&s); sz = sf8(&s); r1 = sf8(&s); form = sf8(&s);
            if (form == 0) a = rget(c, sf8(&s), sz);
            else           a = mrd(mem_addr(&s, c), sz);
            if (cond_eval(c, cc)) rset(c, r1, sz, a);

        } else if (op == OP_MOV_RM) {
            sz = sf8(&s); r1 = sf8(&s);
            addr = mem_addr(&s, c);
            rset(c, r1, sz, mrd(addr, sz));

        } else if (op == OP_MOV_MR) {
            sz = sf8(&s);
            addr = mem_addr(&s, c);
            r1 = sf8(&s);
            mwr(addr, sz, rget(c, r1, sz));

        } else if (op == OP_MOV_MI) {
            sz = sf8(&s);
            addr = mem_addr(&s, c);
            sext32(&sv, sf32(&s));
            mwr(addr, sz, (u64)sv);

        } else if (op == OP_LEA_RM) {
            r1 = sf8(&s);
            rset(c, r1, 8, mem_addr(&s, c));

        } else if (op == OP_ALU_RR) {
            alu = sf8(&s); sz = sf8(&s); r1 = sf8(&s); r2 = sf8(&s);
            r = do_alu(alu, rget(c, r1, sz), rget(c, r2, sz), sz, &fl);
            c->flags = fl;
            if (alu != ALU_CMP && alu != ALU_TEST) rset(c, r1, sz, r);

        } else if (op == OP_ALU_RI) {
            alu = sf8(&s); sz = sf8(&s); r1 = sf8(&s); sext32(&sv, sf32(&s));
            b = (u64)sv;
            if (sz == 4) b &= 0xFFFFFFFFULL;
            if (sz == 2) b &= 0xFFFFULL;
            if (sz == 1) b &= 0xFFULL;
            r = do_alu(alu, rget(c, r1, sz), b, sz, &fl);
            c->flags = fl;
            if (alu != ALU_CMP && alu != ALU_TEST) rset(c, r1, sz, r);

        } else if (op == OP_ALU_RM) {
            alu = sf8(&s); sz = sf8(&s); r1 = sf8(&s);
            addr = mem_addr(&s, c);
            r = do_alu(alu, rget(c, r1, sz), mrd(addr, sz), sz, &fl);
            c->flags = fl;
            if (alu != ALU_CMP && alu != ALU_TEST) rset(c, r1, sz, r);

        } else if (op == OP_ALU_MR) {
            alu = sf8(&s); sz = sf8(&s);
            addr = mem_addr(&s, c);
            r2 = sf8(&s);
            r = do_alu(alu, mrd(addr, sz), rget(c, r2, sz), sz, &fl);
            c->flags = fl;
            if (alu != ALU_CMP && alu != ALU_TEST) mwr(addr, sz, r);

        } else if (op == OP_ALU_MI) {
            alu = sf8(&s); sz = sf8(&s);
            addr = mem_addr(&s, c);
            sext32(&sv, sf32(&s));
            b = (u64)sv;
            if (sz == 4) b &= 0xFFFFFFFFULL;
            if (sz == 2) b &= 0xFFFFULL;
            if (sz == 1) b &= 0xFFULL;
            r = do_alu(alu, mrd(addr, sz), b, sz, &fl);
            c->flags = fl;
            if (alu != ALU_CMP && alu != ALU_TEST) mwr(addr, sz, r);

        } else if (op == OP_SHI_RI) {
            shi = sf8(&s); sz = sf8(&s); r1 = sf8(&s); n = sf8(&s);
            r = do_shift(shi, rget(c, r1, sz), n, sz, &fl);
            c->flags = fl;
            rset(c, r1, sz, r);

        } else if (op == OP_SHI_CL) {
            shi = sf8(&s); sz = sf8(&s); r1 = sf8(&s);
            n = (int)(c->regs[REG_RCX] & 0x3F);
            r = do_shift(shi, rget(c, r1, sz), n, sz, &fl);
            c->flags = fl;
            rset(c, r1, sz, r);

        } else if (op == OP_SHI_RM_RI || op == OP_SHI_RM_CL) {
            shi = sf8(&s); sz = sf8(&s);
            addr = mem_addr(&s, c);
            if (op == OP_SHI_RM_RI) n = sf8(&s);
            else                    n = (int)(c->regs[REG_RCX] & 0x3F);
            r = do_shift(shi, mrd(addr, sz), n, sz, &fl);
            c->flags = fl;
            mwr(addr, sz, r);

        } else if (op == OP_IMUL_RRI) {
            sz = sf8(&s); r1 = sf8(&s); r2 = sf8(&s); sext32(&sv, sf32(&s));
            a = (u64)(i64)rget(c, r2, sz);
            if (sz == 4) a = (u64)(i64)(int)(a & 0xFFFFFFFF);
            r = (u64)(sv * (i64)a);
            c->flags = 0;
            rset(c, r1, sz, r);

        } else if (op == OP_IMUL_RMRI) {
            sz = sf8(&s); r1 = sf8(&s);
            addr = mem_addr(&s, c);
            sext32(&sv, sf32(&s));
            a = (u64)(i64)rget(c, 0, 0);           /* kullanilmaz */
            b = (u64)(i64)rget(c, 0, 0);           /* yer tutici */
            (void)a; (void)b;
            {
                u64 mv = mrd(addr, sz);
                i64 mvx = (sz == 8) ? (i64)mv : (i64)(int)(mv & 0xFFFFFFFF);
                r = (u64)(sv * mvx);
            }
            c->flags = 0;
            rset(c, r1, sz, r);

        } else if (op == OP_IMUL_RR) {
            sz = sf8(&s); r1 = sf8(&s); r2 = sf8(&s);
            a = (u64)(i64)rget(c, r1, sz);
            b = (u64)(i64)rget(c, r2, sz);
            if (sz == 4) {
                a = (u64)(i64)(int)(a & 0xFFFFFFFF);
                b = (u64)(i64)(int)(b & 0xFFFFFFFF);
            }
            r = (u64)((i64)a * (i64)b);
            c->flags = 0;
            rset(c, r1, sz, r);

        } else if (op == OP_INCDEC_R) {
            ud = sf8(&s); sz = sf8(&s); r1 = sf8(&s);
            r = do_alu(ud ? ALU_SUB : ALU_ADD, rget(c, r1, sz), 1, sz, &fl);
            c->flags = fl;
            rset(c, r1, sz, r);

        } else if (op == OP_INCDEC_M) {
            ud = sf8(&s); sz = sf8(&s);
            addr = mem_addr(&s, c);
            r = do_alu(ud ? ALU_SUB : ALU_ADD, mrd(addr, sz), 1, sz, &fl);
            c->flags = fl;
            mwr(addr, sz, r);

        } else if (op == OP_NEG_R) {
            sz = sf8(&s); r1 = sf8(&s);
            r = do_alu(ALU_SUB, 0, rget(c, r1, sz), sz, &fl);
            c->flags = fl;
            rset(c, r1, sz, r);

        } else if (op == OP_NOT_R) {
            sz = sf8(&s); r1 = sf8(&s);
            rset(c, r1, sz, ~rget(c, r1, sz) & szmask(sz));

        } else if (op == OP_NEG_M) {
            sz = sf8(&s); addr = mem_addr(&s, c);
            r = do_alu(ALU_SUB, 0, mrd(addr, sz), sz, &fl);
            c->flags = fl;
            mwr(addr, sz, r);

        } else if (op == OP_NOT_M) {
            sz = sf8(&s); addr = mem_addr(&s, c);
            mwr(addr, sz, ~mrd(addr, sz) & szmask(sz));

        } else if (op == OP_MOVZX || op == OP_MOVSX) {
            sz = sf8(&s); ssz = sf8(&s); r1 = sf8(&s); form = sf8(&s);
            if (form == 0) {
                a = rget(c, sf8(&s), ssz);
            } else {
                addr = mem_addr(&s, c);
                a = mrd(addr, ssz);
            }
            if (op == OP_MOVZX) r = a & szmask(ssz);
            else {
                /* 0.8.0 fix: ssz==4 (movslq/MOVSXD) 16-bit degil 32-bit
                 * isaret genisletme olmali — eski kod (i64)(short) ile
                 * ssz==4'u 16-bit'e dusuruyordu (sdk_ext kanit bug'i). */
                if (ssz == 1)      r = (u64)(i64)(signed char)(a & 0xFF);
                else if (ssz == 2) r = (u64)(i64)(short)(a & 0xFFFF);
                else               r = (u64)(i64)(int)(a & 0xFFFFFFFF);
            }
            rset(c, r1, sz, r);

        } else if (op == OP_PUSH_R) {
            r1 = sf8(&s);
            c->regs[REG_RSP] -= PUSHSZ;
            mwr(c->regs[REG_RSP] - (u64)VM_DELTA, PUSHSZ, rget(c, r1, PUSHSZ));

        } else if (op == OP_PUSH_I) {
            sext32(&sv, sf32(&s));
            c->regs[REG_RSP] -= PUSHSZ;
            mwr(c->regs[REG_RSP] - (u64)VM_DELTA, PUSHSZ, (u64)sv);

        } else if (op == OP_POP_R) {
            r1 = sf8(&s);
            rset(c, r1, PUSHSZ, mrd(c->regs[REG_RSP] - (u64)VM_DELTA, PUSHSZ));
            c->regs[REG_RSP] += PUSHSZ;

        } else if (op == OP_JMP) {
            sext32(&sv, sf32(&s));
            s.ic = s.ic + (u64)sv;

        } else if (op == OP_JCC) {
            cc = sf8(&s); sext32(&sv, sf32(&s));
            if (cond_eval(c, cc)) s.ic = s.ic + (u64)sv;

        } else if (op == OP_NCALL) {
            v32 = sf32(&s);
#ifdef VM_TRACE
            /* harness modu: native cagri YOK — sahte donus (rax=gumus),
               akis devam eder (mem/SIB teshisi icin). */
            if (vm_trace_on) tputn("[ncall] hedef=", c->base + (u64)v32, c->regs[REG_RSP]);
            c->regs[0] = 0;
#else
#ifdef VM_IS_X64
            c->regs[0] = vm_natcall(c->base + (u64)v32, c->regs);
#else
            {
                /* 0.5.1: x86 dogal cagri — arg kayitcilari (eax/ecx/edx)
                gercek CPU'ya tasınır, callee sonrasi esp + eax/ecx/edx VM'e
                yansitilir (0.5.0'da ikisi de yoktu -> yigin bozumu + cop
                thiscall). stdcall temizlemesi esp senkronu ile yansitilir. */
                u64 nret = 0, nesp = 0;
                vm_natcall86(c->regs, (u32)(c->base + (u64)v32),
                             (u32)(c->regs[REG_RSP] - (u64)VM_DELTA), &nret, &nesp);
                c->regs[REG_RSP] = nesp + (u64)VM_DELTA;
            }
#endif
#endif /* VM_TRACE */

        } else if (op == OP_RET) {
            c->done = 1;

        } else if (op == OP_SETCC) {
            cc = sf8(&s); r1 = sf8(&s);
            rset(c, r1, 1, cond_eval(c, cc) ? 1 : 0);

        } else if (op == OP_CQO) {
#if defined(VM_IS_X64)
            c->regs[REG_RDX] = (c->regs[0] >> 63) ? ~0ULL : 0ULL;
#else
            c->regs[REG_RDX] = (c->regs[0] & 0x80000000ULL) ? 0xFFFFFFFFULL : 0ULL;
#endif

        } else if (op == OP_IDIV_R) {
            sz = sf8(&s); r1 = sf8(&s);
#if defined(VM_IS_X64)
            if (sz == 4) {
                i64 dd = ((i64)(int)(c->regs[REG_RDX] & 0xFFFFFFFF) << 32)
                       |  (i64)(int)(c->regs[0] & 0xFFFFFFFF);
                i64 dv = (i64)(int)(c->regs[r1] & 0xFFFFFFFF);
                if (dv == 0) { c->regs[0] = 0; c->regs[REG_RDX] = 0; }
                else { c->regs[0] = (u64)(int)(dd / dv); c->regs[REG_RDX] = (u64)(int)(dd % dv); }
            } else {
                i64 dd = (i64)c->regs[REG_RDX];
                i64 dv = (i64)c->regs[r1];
                if (dv == 0) { c->regs[0] = 0; c->regs[REG_RDX] = 0; }
                else { c->regs[0] = (u64)(dd / dv); c->regs[REG_RDX] = (u64)(dd % dv); }
            }
#else
            if (sz == 4) {
                int dv = (int)(c->regs[r1] & 0xFFFFFFFF);
                if (dv == 0) { c->regs[0] = 0; c->regs[REG_RDX] = 0; }
                else {
                    int q = (int)(c->regs[0] & 0xFFFFFFFF) / dv;
                    int rm = (int)(c->regs[0] & 0xFFFFFFFF) % dv;
                    c->regs[0] = (u32)q; c->regs[REG_RDX] = (u32)rm;
                }
            } else { c->done = 1; }
#endif

        } else if (op == OP_ANTIDBG) {
#ifdef VM_WINDOWS
#if defined(VM_IS_X64)
            { u64 peb; __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb));
              u8* p = (u8*)u2p(peb);
              if (p[2] || (p[0xBC] & 0x70)) c->done = 2; }
#else
            { u32 peb; __asm__ volatile("movl %%fs:0x30, %0" : "=r"(peb));
              u8* p = (u8*)u2p((u64)peb);
              if (p[2] || (p[0x68] & 0x70)) c->done = 2; }
#endif
#endif

                } else if (op == OP_BSWAP_R) {
            /* 0.8.0 komut genisletme: bswap r (yalniz 32/64) */
            sz = sf8(&s); r1 = sf8(&s);
            u64 v = rget(c, r1, sz), w = 0;
            int k2;
            for (k2 = 0; k2 < sz; k2++) w = (w << 8) | ((v >> (8 * k2)) & 0xFF);
            rset(c, r1, sz, w);

        } else if (op == OP_XCHG_RR) {
            /* 0.8.0 komut genisletme: xchg r1, r2 (bayrak etkisi yok) */
            sz = sf8(&s); r1 = sf8(&s); r2 = sf8(&s);
            u64 va = rget(c, r1, sz), vb = rget(c, r2, sz);
            rset(c, r1, sz, vb);
            rset(c, r2, sz, va);

} else if (op == OP_STRDEC) {
            u32 strva = sf32(&s), stva = sf32(&s), k = sf32(&s), len = sf32(&s);
            u8* st = (u8*)u2p(c->base + (u64)stva);
            /* 0.8.0 sertlik: CFB tamper len'i 4G'ye cevirebilir -> handler
               ici 4G yineleme (bütce tek komutu sayar, ici döngüyü saymaz).
               Meşru strenc dizgesi bolge buyuklugu sinifinda (<1MB). */
            if (len > 0x100000u) len = 0;
            if (*st == 0) {
                u8* p = (u8*)u2p(c->base + (u64)strva);
                u32 i2;
                for (i2 = 0; i2 < len; i2++) p[i2] ^= (u8)(k + i2 * 31);
                *st = 1;
            }

        } else {
            c->done = 1;   // bilinmeyen opcode -> guvenli dur
        }
    }
    }
}

void vm_entry(VMContext* c) {
    int k;
    int map[NMAP];
#if defined(VM_IS_X64)
    /* 0.5.0 KOK-NEEN Duzeltmesi: eski {0,1,2,3,5,...} tablosunu GCC 14
    .rodata+memcpy'ye ceviriyordu -> blob PIC olmaktan cikiyordu (korunmis
    exe'de 0x103000 mutlak okuma = SIGSEGV). Tablo ARITMETIK yazilir:
    reg 4 (RSP) push edilmez -> atlanir. */
    for (k = 0; k < 15; k++) map[k] = (k < 4) ? k : (k + 1);
#else
    /* x86 pushad sirasi: EAX once push edilir -> EN YUKSEK adreste; EDI enson
       -> EN ALCAK adreste. push_area[k] ARTAN okundugu icin siralama ters:
       [0]=EDI(7), [1]=ESI(6), [2]=EBP(5), [3]=ESP_orig(atla), [4]=EBX(3),
       [5]=EDX(2), [6]=ECX(1), [7]=EAX(0). Eski map {0,1,2,3,..} reg'leri
       tamamen karistiriyordu (dir=copuk deger -> yanlis hareket). */
    for (k = 0; k < 8; k++) map[k] = (k == 3) ? (int)0xFFFFFFFFu : (7 - k);
#endif
#if defined(VM_IS_X64)
    u64* src = (u64*)u2p(c->push_area);          /* x64: 8B slot (push'lar) */
    #define SRC_LOAD(k)  src[k]
    #define SRC_STORE(k, v) (src[k] = (v))
#else
    u32* src = (u32*)u2p(c->push_area);          /* x86 pushad: 4B slot! */
    #define SRC_LOAD(k)  (u64)src[k]
    #define SRC_STORE(k, v) (src[k] = (u32)(v))
#endif

    for (k = 0; k < NMAP; k++) {
        if (map[k] != (int)0xFFFFFFFFu) c->regs[map[k]] = SRC_LOAD(k);
    }
    c->regs[4] = c->init_rsp;
    c->flags = 0;
    c->done = 0;

    /* 0.8.0 crypt_key: stub key^K2 tasiydi — gercek rolling anahtari geri
       coz. K2 = (bc_rva, bc_len) fonksiyonu (engine_stub ile ayni makro). */
    {
        u32 bc_rva = (u32)(((const u8*)u2p(c->bc)) - (const u8*)u2p(c->base));
        c->key ^= VM_KEYXOR(bc_rva, c->bc_len);
    }

    vm_run(c);

    if (c->done == 2) {
        c->regs[0] = 0xDEADBEEFDEADBEEFULL;   // debugger -> zehirli donus
        c->done = 1;
    }
    for (k = 0; k < NMAP; k++) {
        if (map[k] != (int)0xFFFFFFFFu) SRC_STORE(k, c->regs[map[k]]);
    }
    (void)c->extra_pop;
}
