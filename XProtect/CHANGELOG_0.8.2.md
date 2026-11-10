# XProtect 0.8.2 — CE VEH Debugger Yakalama (0.8.1 anti-debug genişletmesi)

## Kullanıcı bildirimi
0.8.1'de: fonksiyon seç → ULTRA + Check Debugger=Yes → derle. Cheat Engine
**Windows debugger** ile F5 → fonksiyon duruyor ✓. **VEH debugger** ile F5 →
fonksiyon ÇALIŞMAYA devam ediyor ✗.

## Kök neden
0.8.1 OP_ANTIDBG yalnız **PEB.BeingDebugged + NtGlobalFlag** kontrol ediyordu.
CE VEH debugger:
- İşletim sistemi debug API'sini KULLANMAZ (DebugActiveProcess yok) →
  PEB.BeingDebugged=0, NtQueryInformationProcess(ProcessDebugPort)=0
- Yazılım-bp'yi fonksiyon girişine 0xCC yazar, tek-adımda **geri yazar**
  (görünmez) → statik tarama da yetmez
- Donanım-bp'de DR0-DR3'ü SetThreadContext ile kurar (kodda hiç iz yok)

## Çözüm — VMProtect (UniversalObfuscator public src) loader.cc/core.cc 1:1 port
Kaynak referansları (orijinal dosya:satır):
| Kontrol | Orijinal kaynak | Açıklama |
|---|---|---|
| PEB.BeingDebugged + NtGlobalFlag | core.cc:764 | (0.8.1'de vardı) |
| ProcessDebugPort + ProcessDebugObjectHandle | core.cc:785 | gerçek debugger |
| TF-prob istisna bağlamından DRx | core.cc:771 | donanım-bp |
| ntdll kritik API girişlerinde 0xCC | loader.cc:2116 | NtOpenFile, NtCreateSection, NtMapViewOfSection, NtUnmapViewOfSection, NtClose, NtProtectVirtualMemory, NtQueryInformationProcess + kernel32!GetThreadContext |
| NtProtectVirtualMemory eski-koruma PAGE_GUARD/NOACCESS | loader.cc:2246+2875 | guard-page bp |
| TF set + RDTSC problaması | loader.cc:2510 | tek-adım istisnasını yutan debugger (CE VEH!) |

XProtect'e özgü ek (orijinalde yok): **kapı kontrolü** — korunan fonksiyonun
giriş bayt-0'ı 0xCC olamaz (E9 tramp / 41 / 60 yerinde). CE VEH yazılım-bp
tek-adımda baytı geri yazdığı için yorumlayıcı İÇİNDE bu kontrol zorunlu.

## Uygulama notları (0.8.2)
- Tüm isim çözümleri PEB-gezgini + FNV-1a hash (rodata yok, blob tam PIC)
- TF probu: RtlAddVectoredExceptionHandler(0=KUYRUK) + pushfq|TF + rdtsc + nop
  + RtlRemoveVectoredExceptionHandler. İstisna kuyruktaki işleyicimize
  ulaşırsa rax=MAGIC (temiz); bir VEH/debugger yuttuysa rax=RDTSC sayacı
  (tespit). İki mimaride de işleyici PC-göreli adresle kurulur (x64: lea
  sym(%rip) + ms_abi; x86: call/pop + (sembol-1b) farkı + stdcall)
- x86 NTSTATUS kontrolü: üst-32-bit maskeleme yerine 0x80000000 testi
- (C) ProcessDebugObjectHandle: orijinaldeki `|| debug_object==0` ifadesi
  temiz süreçte yanlış-pozitif üretir → yalnız NT_SUCCESS && !=0
- **VM_DELTA64/86: 0x600→0xC00** — yeni derin C çağrı zinciri (wine
  server-RPC dahil) eski emüle-yığın bölgesini eziyordu (bisekt ile bulundu:
  belirleyici bozuk hesap r=-2145765869)
- **scratch[3] (u32) cast düzeltmesi** — stub perm tohumuna yalnız 4 bayt
  yazar; üst 32 bit yığın çöpü (strenc kombinasyonunda page-fault)

## Kanıt (wine64 + Xvfb, son 0.8.2 binary)
CE-VEH simülatör diferansiyel matrisi (test/test_veh.c — CE davranışı birebir:
bp baytını yazar, tek-adımda geri yazar, tek-adım istisnalarını yutar):
| Mod | Senaryo | Sonuç | Beklenen |
|---|---|---|---|
| clean | debugger yok | r=3812 rc=42 ÇALIŞIR | ÇALIŞIR ✓ |
| sw | VEH + yazılım-bp (0xCC @giriş) | r=0xDEADBEEF DURDU | DURDU ✓ |
| hw | VEH + DR0 donanım-bp | r=0xDEADBEEF DURDU | DURDU ✓ |
| vehonly | VEH tek-adım-yutan (bp yok) | r=0xDEADBEEF DURDU | DURDU ✓ |
| peb | PEB.BeingDebugged=1 (gerçek-debugger yolu) | r=0xDEADBEEF DURDU | DURDU ✓ |

Notlar:
- hw modunda wine SetThreadContext DR0 round-trip yapmıyor (dr0=0 okunur);
  hw yakalaması wine altında TF-probu üzerinden kanıtlandı. (D) DRx kontrolü
  kod-inceleme düzeyinde (winnt.h ofsetleri) doğrulanmıştır.
- x86 (PE32): runtime wine sınırlaması (syswow64 yok) → statik kanıt:
  vmx86.bin tüm batarya sabitleri içerir; metin2client (PE32!) çıktısında
  bc[0]=0x0A → itab → 0x1E ANTIDBG ✓

## Diğer regresyonlar (0.8.1 referans değerleriyle birebir)
- R5 ULTIMATE (V+M + pack LZMA + sectcrypt CFB-64 + loadercrc + filecrc +
  stripfixups): a=78 b=0x16f9 c=0x1221 d=81, rc=42 ✓
- R4 tamper (paketli .text tek-bayt): rc=115 dur-ret (42 değil) ✓
- strenc + antidebug birlikte: str=XP-gizli-mesaj-0.8.1 ok=1 ✓
- SELFTEST OK (funcs=0 baseline — 0.8.1 ile aynı) ✓
- metin2client d7610 U + antidebug + memprot: statik ANTIDBG ✓, E9 kapı ✓

## Paket
XProtect_0.8.2.zip — XProtectUI.exe (x64) + XProtectUI32.exe (x86) +
xprotect_sdk.h + kanit/ + source/XProtectUI_Source/
