// drprobe.c — wine DRx round-trip tezgahi (K3 test dogrulamasi icin)
// 1) SetThreadContext(GetCurrentThread())             -> geri okuma?
// 2) SuspendThread + SetThreadContext(uye is-parcacigi) + Resume -> geri okuma?
#include <stdio.h>
#include <windows.h>

static DWORD WINAPI drthread(LPVOID p) {
    volatile int spin = 0;
    HANDLE ev = (HANDLE)p;
    // main thread suspend+set yapana kadar don
    while (WaitForSingleObject(ev, 0) != WAIT_OBJECT_0) { spin++; Sleep(0); }
    CONTEXT c3; memset(&c3, 0, sizeof c3);
    c3.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    unsigned long dr0 = 0, ok = 0;
    if (GetThreadContext(GetCurrentThread(), &c3)) { dr0 = (unsigned long)c3.Dr0; ok = 1; }
    FILE* f = fopen("Z:\\home\\z\\my-project\\scripts\\xp085\\test\\drprobe2.txt", "w");
    if (f) { fprintf(f, "child dr0=%lu gtc=%lu\n", dr0, ok); fclose(f); }
    printf("child dr0=%lu gtc=%lu\n", dr0, ok); fflush(stdout);
    return 0;
}

int main(void) {
    // yol 1: kendi is-parcaciginda set/oku
    CONTEXT c2; memset(&c2, 0, sizeof c2);
    c2.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    c2.Dr0 = 0x004D7610; c2.Dr7 = 1;
    LONG sc1 = SetThreadContext(GetCurrentThread(), &c2);
    CONTEXT c3; memset(&c3, 0, sizeof c3);
    c3.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    unsigned long dr0a = 0; BOOL ga = GetThreadContext(GetCurrentThread(), &c3);
    if (ga) dr0a = (unsigned long)c3.Dr0;
    printf("self  sc=%ld gtc=%d dr0=%lu\n", sc1, (int)ga, dr0a); fflush(stdout);

    // yol 2: askiya alinmis ikinci is-parcacigi
    HANDLE ev = CreateEventA(NULL, TRUE, FALSE, NULL);
    HANDLE h = CreateThread(NULL, 0x10000, drthread, ev, 0, NULL);
    Sleep(200);
    SuspendThread(h);
    CONTEXT c4; memset(&c4, 0, sizeof c4);
    c4.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(h, &c4);
    c4.Dr0 = 0x004D7610; c4.Dr7 = 1;
    LONG sc2 = SetThreadContext(h, &c4);
    CONTEXT c5; memset(&c5, 0, sizeof c5);
    c5.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    unsigned long dr0b = 0; BOOL gb = GetThreadContext(h, &c5);
    if (gb) dr0b = (unsigned long)c5.Dr0;
    printf("child-suspend sc=%ld gtc=%d dr0=%lu\n", sc2, (int)gb, dr0b); fflush(stdout);
    ResumeThread(h);
    SetEvent(ev);
    WaitForSingleObject(h, 3000);
    FILE* f = fopen("Z:\\home\\z\\my-project\\scripts\\xp085\\test\\drprobe1.txt", "w");
    if (f) { fprintf(f, "self sc=%ld dr0=%lu | child sc=%ld dr0=%lu\n", sc1, dr0a, sc2, dr0b); fclose(f); }
    return 0;
}
