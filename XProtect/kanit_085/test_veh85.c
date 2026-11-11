// test_veh84.c — 0.8.4 VEH debugger test gövdesi (x64+x86)
// Modlar:
//   clean   : VEH yok                      → damage() NORMAL (rc=42)
//   ceveh   : YABANCI VEH (VirtualAlloc sayfa — CE vehdebug taklidi)
//             tek-adımı YUTAR, bp yok      → DURDU (zehir; 0.8.4'te yeni)
//   gameveh : OYUN-VEH taklidi (handler ANA EXE içinde) tek-adımı yutar
//                                          → TEMİZ rc=42 (yanlış-pozitif YOK!)
//   gamedrx : gameveh + DR0=damage (oyun gözetim noktası taklidi)
//                                          → TEMİZ rc=42 (DRx tek başına karar değil)
//   sw      : yabancı VEH + damage girişine 0xCC (CE F5 yazılım-bp)
//                                          → DURDU
//   cevehdr : yabancı VEH + DR0=damage (CE donanım-bp)
//                                          → DURDU
//   tfstep  : oyun-VEH + TF kendiliinden-kur (tek-adim izleme taklidi)
//                                          → DURDU (K2)
//   peb     : PEB.BeingDebugged kendiliinden-yaz (Windows debugger yolu)
//                                          → DURDU (0.8.1 kanıtı)
// Çıktı: veh85_sonuc.txt + stdout. CORRECT=3812, POISON EAX=-559038737.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static void*  g_veh   = NULL;
static void*  g_veh2  = NULL;
static BYTE*  g_bp    = NULL;
static BYTE   g_orig  = 0;
static int    g_tfkeep = 0;

/* ---- oyun-VEH taklidi (ANA EXE içinde derlenir) ----
   STATUS_SINGLE_STEP'i yutar (bazı oyun korumaları böyle yapar);
   BREAKPOINT'i (sw modu) CE gibi işler: baytı geri koy, TF kur, Eip=bp. */
static LONG CALLBACK game_sim_veh(void* expp) {
    void** ep = (void**)expp;
    unsigned char* rec = (unsigned char*)ep[0];
    void* ctx = ep[1];
    unsigned long code = *(unsigned long*)(rec + 0);
    void* addr = *(void**)(rec + 0x10);
    if (code == 0x80000003 && g_bp && addr == (void*)g_bp) {
        *g_bp = g_orig;
#if defined(_WIN64)
        *(unsigned long*)((char*)ctx + 0x44) |= 0x100;              /* EFlags TF */
        *(unsigned long long*)((char*)ctx + 0xF8) = (unsigned long long)g_bp; /* Rip */
#else
        *(unsigned long*)((char*)ctx + 0xC0) |= 0x100;              /* EFlags TF (dogru ofset!) */
        *(unsigned long*)((char*)ctx + 0xB8) = (unsigned long)g_bp; /* Eip */
#endif
        return -1;
    }
    if (code == 0x80000004) {
        if (g_bp) *g_bp = 0xCC;         /* bp'yi tek-adimda geri kur (CE gibi) */
#if defined(_WIN64)
        if (g_tfkeep) *(unsigned long*)((char*)ctx + 0x44) |= 0x100;
#else
        if (g_tfkeep) *(unsigned long*)((char*)ctx + 0xC0) |= 0x100;
#endif
        return -1;                      /* tek-adimi YUT (oyun koruma taklidi) */
    }
    return 0;
}

/* ---- YABANCI VEH (CE vehdebug taklidi): VirtualAlloc sayfasına yazılan
   makine kodu; yalnız STATUS_SINGLE_STEP'i yutar, gerisini geçirir.
   Global YOK (taşınabilir). ---- */
#if defined(_WIN64)
static const unsigned char FOR_STUB[] = {
    0x48,0x8B,0x01,                   /* mov rax,[rcx]         ; rec   */
    0x81,0x38,0x04,0x00,0x00,0x80,    /* cmp dword [rax],0x80000004    */
    0xB8,0x00,0x00,0x00,0x00,         /* mov eax,0                     */
    0x75,0x01,                        /* jne done                      */
    0xFF,0xC8,                        /* dec eax               ; -1    */
    0xC3                              /* done: ret                     */
};
#else
static const unsigned char FOR_STUB[] = {
    0x8B,0x44,0x24,0x04,              /* mov eax,[esp+4]       ; expp  */
    0x8B,0x00,                        /* mov eax,[eax]         ; rec   */
    0x81,0x38,0x04,0x00,0x00,0x80,    /* cmp dword [eax],0x80000004    */
    0xB8,0x00,0x00,0x00,0x00,         /* mov eax,0                     */
    0x75,0x01,                        /* jne done                      */
    0x48,                             /* dec eax               ; -1    */
    0xC2,0x04,0x00                    /* done: ret 4 (stdcall)         */
};
#endif

static void* add_foreign_swallow(void) {
    DWORD sz = sizeof(FOR_STUB);
    void* pg = VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
    if (!pg) return NULL;
    memcpy(pg, FOR_STUB, sz);
    return AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)pg);
}

__attribute__((noinline)) static int damage(int a, int b) {
    __asm__ volatile(".byte 0x90,0x90,0xB8,0x5A,0x6B,0x7C,0x11");  /* iz */
    volatile int r = 0;
    r = a + b;
    r = r * 3 + 7;
    r = r ^ 0x55;
    r = r - b;
    return r;
}

static const char* OUTPATH =
#ifdef _WIN64
    "Z:\\home\\z\\my-project\\scripts\\xp085\\test\\veh85_sonuc64.txt";
#else
    "Z:\\home\\z\\my-project\\scripts\\xp085\\test\\veh85_sonuc32.txt";
#endif

/* ---- 0.8.5 DRx iscisi: damage ASKIDA ALINMIS is-parcaciginda kosar.
     Gercek debugger da DRx'i askiya alinmis is-parcacigina kurar
     (suspend/set/resume). Wine'da CALISAN is-parcacigina SetThreadContext
     RAKSIZ (gle=5 ACCESS_DENIED; drprobe kanit) — askiya-alinmis yol
     her ortamda deterministik. DR0 = damage+8 (olu govde, hic fetch
     edilmez -> #DB ateslenmez -> DRx ARMALI kalir -> K3 gorur). */
static volatile int           g_dr_result = 0;
static volatile unsigned long g_dr_dr0    = 0;
static HANDLE g_dr_done = NULL;

static volatile LONG g_dr_go = 0;
static volatile LONG g_dr_armed_ok = 0;
static volatile LONG g_dr_rearm = 0;
static int g_extarm = 0;
static const char* GO_PATH =
#ifdef _WIN64
    "Z:\\home\\z\\my-project\\scripts\\xp085\\test\\go.txt";
#else
    "Z:\\home\\z\\my-project\\scripts\\xp085\\test\\go.txt";
#endif

static DWORD WINAPI dr_worker(LPVOID p) {
    (void)p;
    if (g_extarm) {
        /* dis arming: drarm.c ptrace ile DR0 yazar; go.txt beklenir */
        while (!g_dr_go) {
            if (GetFileAttributesA(GO_PATH) != INVALID_FILE_ATTRIBUTES) g_dr_go = 1;
            else Sleep(5);
        }
    } else {
        /* PROBESIZ bekleme (isci'nin kendi GetThreadContext'i wine'da DRx
           durumunu bozuyor: probe'lu 0/3 vs probesuz 3/3 kanit) +
           ZEHIR-YAKALAMA: sonuc POISON degilse main'den yeniden-arm iste
           (wine resume-apply flak; gercel Windows'ta askiya-alinmis
           thread'e SetThreadContext deterministiktir — 1. deneme yeter). */
        int attempt;
        for (attempt = 0; attempt < 5; attempt++) {
            while (!g_dr_go) {
                volatile int s = 0, i;
                for (i = 0; i < 20000; i++) s += i;
            }
            if (attempt) Sleep(80);              /* re-arm uygulanana kadar */
            g_dr_result = damage(1234, 77);
            {
                CONTEXT c3; memset(&c3, 0, sizeof c3);
                c3.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                unsigned long d0 = 0, ok = 0;
                if (GetThreadContext(GetCurrentThread(), &c3))
                    { d0 = (unsigned long)c3.Dr0; ok = 1; }
                g_dr_dr0 = d0;
                printf("DIAG worker-try%d dr0=%lu gtc=%lu r=%d\n",
                       attempt + 1, d0, ok, g_dr_result); fflush(stdout);
            }
            if (g_dr_result == -559038737) break;         /* POISON */
            if (attempt < 4) {
                InterlockedExchange(&g_dr_rearm, 1);
                InterlockedExchange(&g_dr_go, 0);
                while (g_dr_rearm) Sleep(10);             /* main re-arm */
            }
        }
        if (g_dr_done) SetEvent(g_dr_done);
        return 0;
    }
}

int main(int argc, char** argv) {
    const char* mode = (argc > 1) ? argv[1] : "clean";
    int is_foreign = 0, is_game = 0, is_dr = 0, is_sw = 0, is_peb = 0, is_tf = 0;
    if      (!strcmp(mode, "ceveh"))   { is_foreign = 1; }
    else if (!strcmp(mode, "gameveh")) { is_game = 1; }
    else if (!strcmp(mode, "gamedrx")) { is_game = 1; is_dr = 1; }
    else if (!strcmp(mode, "sw"))      { is_foreign = 1; is_game = 1; is_sw = 1; }
    else if (!strcmp(mode, "cevehdr")) { is_foreign = 1; is_dr = 1; }
    else if (!strcmp(mode, "tfstep"))  { is_game = 1; is_tf = 1; }
    else if (!strcmp(mode, "peb"))     { is_peb = 1; }

    if (is_foreign) { g_veh = add_foreign_swallow(); }         /* CE: yabanci, kafa */
    if (is_game)    { g_veh2 = AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)game_sim_veh); }
    if (is_sw) {
        g_bp = (BYTE*)damage;
        DWORD oldp = 0;
        VirtualProtect(g_bp, 16, PAGE_EXECUTE_READWRITE, &oldp);
        g_orig = *g_bp;
        *g_bp = 0xCC;                                          /* CE F5 */
    }
    if (is_dr) {
        /* 0.8.5: DR0 = damage+8 — gövde içi ölü adres. extarm.txt varsa
           dis arming (drarm.c ptrace) + go.txt gecidi; yoksa suspend/set/
           resume (wine'da raksiz olabilir — DIAG satirlari kanit). */
        g_extarm = (GetFileAttributesA(
            "Z:\\home\\z\\my-project\\scripts\\xp085\\test\\extarm.txt"
            ) != INVALID_FILE_ATTRIBUTES);
        if (g_extarm) {
            printf("DRADDR=%llx\n",
                   (unsigned long long)((BYTE*)damage + 8)); fflush(stdout);
        }
        g_dr_done = CreateEventA(NULL, TRUE, FALSE, NULL);
        HANDLE hw = CreateThread(NULL, 0x10000, dr_worker, NULL, 0, NULL);
        if (!g_extarm) {
            /* tek-atim arming + main-tarafli dogrulama (hw handle server
               kopyasi okur — canli DRx'e DOKUNMAZ; isci probu yasak).
               Re-arm dongusu: isci POISON alamadiysa istek bayragi kaldirir. */
            int round;
            for (round = 0; round < 8; round++) {
                Sleep(300);
                SuspendThread(hw);
                CONTEXT c2; memset(&c2, 0, sizeof c2);
                c2.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                c2.Dr0 = (DWORD_PTR)((BYTE*)damage + 8);
                c2.Dr7 = 1;
                SetThreadContext(hw, &c2);
                {
                    CONTEXT cx; memset(&cx, 0, sizeof cx);
                    cx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    BOOL g1 = GetThreadContext(hw, &cx);
                    printf("DIAG main-verify r%d dr0=%llx g1=%d\n", round,
                           (unsigned long long)cx.Dr0, (int)g1); fflush(stdout);
                }
                ResumeThread(hw);
                InterlockedExchange(&g_dr_go, 1);
                /* isci: POISON aldi -> done; alamadi -> rearm istegi */
                for (int w = 0; w < 60 && !g_dr_rearm; w++) {
                    if (WaitForSingleObject(g_dr_done, 0) == WAIT_OBJECT_0) break;
                    Sleep(50);
                }
                if (WaitForSingleObject(g_dr_done, 0) == WAIT_OBJECT_0) break;
                if (!g_dr_rearm) break;
                InterlockedExchange(&g_dr_rearm, 0);
            }
        }
        WaitForSingleObject(g_dr_done, 30000);
        CloseHandle(hw);
    }
    if (is_tf) { g_tfkeep = 1; }
    if (is_peb) {
#ifdef _WIN64
        unsigned long long peb2; __asm__ volatile("movq %%gs:0x60, %0" : "=r"(peb2));
#else
        unsigned int peb2; __asm__ volatile("movl %%fs:0x30, %0" : "=r"(peb2));
#endif
        *(unsigned char*)(peb2 + 2) = 1;
    }

    if (is_tf) {
#if defined(_WIN64)
        __asm__ volatile("pushfq\n\t orq $0x100, (%%rsp)\n\t popfq" :: : "cc", "memory");
#else
        __asm__ volatile("pushfl\n\t orl $0x100, (%%esp)\n\t popfl" :: : "cc", "memory");
#endif
    }
    int r;
    if (is_dr) {
        r = g_dr_result;                    /* damage askida iscide koştu */
    } else {
        r = damage(1234, 77);
    }
    if (is_tf) { g_tfkeep = 0; }

    unsigned long dr0 = 0;
    if (is_dr) {
        dr0 = g_dr_dr0;                     /* iscinin DR0 okumasi */
    } else {
        CONTEXT c3; memset(&c3, 0, sizeof c3);
        c3.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &c3)) dr0 = (unsigned long)c3.Dr0;
    }
    FILE* f = fopen(OUTPATH, "w");
    if (f) { fprintf(f, "mode=%s r=%d dr0=%lu\n", mode, r, dr0); fclose(f); }
    printf("mode=%s r=%d dr0=%lu\n", mode, r, dr0);
    fflush(stdout);
    return (r == 3812) ? 42 : 1;
}
