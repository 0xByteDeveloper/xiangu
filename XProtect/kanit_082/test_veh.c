// test_veh.c — 0.8.2 CE-VEH simülatör diferansiyel test gövdesi (x64+x86)
// Kullanım: test_veh.exe <mod>  → mod: clean | sw | hw | vehonly
//   clean  : VEH yok, kesinlik temiz — damage() NORMAL çalışmalı
//   sw     : CE VEH simülatörü + damage girişine 0xCC (yazılım-bp, CE F5)
//   hw     : CE VEH simülatörü + DR0=damage (donanım-bp)
//   vehonly: VEH kurulu, tek-adım istisnalarını YUTUYOR (bp yok) — TF probu
// Sonuç: veh_sonuc.txt ("mode=<> r=<> dr0=<>") — dosyaya (stdout winedbg'de
// güvenilmez). CORRECT=3812, POISON=0xDEADBEEF(sinirsiz ekranda -559038737
// veya 3735928559).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>



static void*  g_veh   = NULL;
static BYTE*  g_bp    = NULL;
static BYTE   g_orig  = 0;
static int    g_eatss = 1;

static LONG CALLBACK ce_sim_veh(void* expp) {
    // EXCEPTION_POINTERS { PEXCEPTION_RECORD; PCONTEXT; }
    void** ep = (void**)expp;
    unsigned char* rec = (unsigned char*)ep[0];
    void*           ctx = ep[1];
    unsigned long code = *(unsigned long*)(rec + 0);
    void* addr = *(void**)(rec + 0x10);   // ExceptionAddress (x64 +0x10, x86 +0x0C)
    if (code == 0x80000003 /*STATUS_BREAKPOINT*/ && addr == (void*)g_bp) {
        *g_bp = g_orig;                       // CE: baytı geri koy
#if defined(_M_X64) || defined(__x86_64__)
        *(unsigned long*)((char*)ctx + 0x44) |= 0x100;  // EFlags |= TF
        *(unsigned long long**)((char*)ctx + 0xF8) = 0,         *(unsigned long long*)((char*)ctx + 0xF8) = (unsigned long long)g_bp;  // Rip = bp (yeniden yürüt)
#else
        *(unsigned long*)((char*)ctx + 0xB8) |= 0x100;
        *(unsigned long*)((char*)ctx + 0xB0) = (unsigned long)g_bp;  // Eip = bp
#endif
        return -1;                            // EXCEPTION_CONTINUE_EXECUTION
    }
    if (code == 0x80000004 /*STATUS_SINGLE_STEP*/) {
        if (g_bp) *g_bp = 0xCC;               // CE: bp'yi tek-adımda geri kur
        return g_eatss ? -1 : 0;
    }
    return 0;                                 // CONTINUE_SEARCH
}

__attribute__((noinline)) static int damage(int a, int b) {
    __asm__ volatile(".byte 0x90,0x90,0xB8,0x5A,0x6B,0x7C,0x11");  // iz
    volatile int r = 0;
    r = a + b;
    r = r * 3 + 7;
    r = r ^ 0x55;
    r = r - b;
    return r;
}

static const char* OUTPATH =
#ifdef _WIN64
    "Z:\\home\\z\\my-project\\scripts\\xp081\\test\\veh_sonuc64.txt";
#else
    "Z:\\home\\z\\my-project\\scripts\\xp081\\test\\veh_sonuc32.txt";
#endif

int main(int argc, char** argv) {
    const char* mode = (argc > 1) ? argv[1] : "clean";
    g_veh = AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)ce_sim_veh);  // CE: kafa

    if (!strcmp(mode, "sw")) {
        g_bp = (BYTE*)damage;                 // yalnız sw: yazılım-bp hedefi
        DWORD oldp = 0;
        VirtualProtect(g_bp, 16, PAGE_EXECUTE_READWRITE, &oldp);  // CE: sayfayı yazılabilir yap
        g_orig = *g_bp;
        *g_bp = 0xCC;                          // CE F5: girişe yazılım-bp
    } else if (!strcmp(mode, "hw")) {
        CONTEXT c2; memset(&c2, 0, sizeof c2);
        c2.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        GetThreadContext(GetCurrentThread(), &c2);
        c2.Dr0 = (DWORD_PTR)damage;            // CE VEH donanım-bp
        c2.Dr7 = 1;
        SetThreadContext(GetCurrentThread(), &c2);
    } else if (!strcmp(mode, "peb")) {
        RemoveVectoredExceptionHandler(g_veh);
        g_veh = NULL;
        // 0.8.1 T2 kanit metodu: PEB.BeingDebugged kendiliinden-yaz (gerçek
        // debugger yolunun dogrudan testi — DebugPort olmadan)
#ifdef _WIN64
        unsigned long long peb2; __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb2));
#else
        unsigned int peb2; __asm__ volatile("movl %%fs:0x30, %0" : "=r"(peb2));
#endif
        *(unsigned char*)(peb2 + 2) = 1;
    } else if (strcmp(mode, "vehonly") != 0) {
        RemoveVectoredExceptionHandler(g_veh); // clean: simülatörü kaldır
        g_veh = NULL;
    }

    int r = damage(1234, 77);

    unsigned long dr0 = 0;
    {
        CONTEXT c3; memset(&c3, 0, sizeof c3);
        c3.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &c3)) dr0 = c3.Dr0;
    }
    FILE* f = fopen(OUTPATH, "w");
    if (f) { fprintf(f, "mode=%s r=%d dr0=%lu\n", mode, r, dr0); fclose(f); }
    printf("mode=%s r=%d dr0=%lu\n", mode, r, dr0);
    fflush(stdout);
    return (r == 3812) ? 42 : 1;
}
