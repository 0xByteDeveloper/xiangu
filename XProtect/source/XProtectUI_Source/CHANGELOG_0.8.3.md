# XProtect 0.8.3 — Oyun-güvenli anti-debug (0.8.2 oyun-çökmesi fix)

## Kullanıcı bildirimi (0.8.2)
metin2client.exe (PE32) + d7610 ULTRA + Check Debugger=Yes → **oyunda damage
attığında debugger hiç bağlanmadan çökme**.

## Kök nedenler (kanıtlı)
1. **TF+RDTSC probu yanlış-pozitifi (asil çökme):** 0.8.2 bataryası (G)
   "tek-adım istisnası birisi tarafından YUTULDUysa = debugger" kararını
   tek başına veriyordu. Gerçekte izi bırakan: boşta-enjekte CE VEH
   (debugger BAĞLI değilken), oyun koruma VEH, AV istisna-olarakları.
   → Temiz oyunda batarya TETİKLENDİ → zehirli dönüş 0xDEADBEEF → oyunun
   d7610 çağırıcısı değeri pointer/sayaç olarak kullandığından ÇÖKME.
2. **x86 pseudo-handle hatası (sessiz körluk):** (C) NtQueryInformationProcess
   ve (F) NtProtectVirtualMemory'ye 0xFFFFFFFE (CURRENT THREAD) verildi;
   doğru olan 0xFFFFFFFF (NtCurrentProcess). x86'da iki kontrol hiç
   çalışmıyordu (x64'te -1 doğruydu; x86 runtime wine syswow64 olmadığından
   hiç koşulmamıştı — 0.8.2'nin "veh_matrix_x86.txt" kanıtı aslında
   rc=1'leri DURDU sayan bozuk bir kanıttı).
3. **RVEH sonrası ECX okuma (derleme tuzağı):** TF-probe bağlam-DRx değeri
   registerde tutulunca RtlRemoveVectoredExceptionHandler çağrisi ecx'i
   bozuyor (Win32'de volatile) — değer belleğe yazılarak çözüldü.

## 0.8.3 düzeltmeleri (vm_core.c)
- **Karar politikası:** KESİN sinyaller tek başına tespit verir:
  PEB.BeingDebugged/NtGlobalFlag, KAPI (fonksiyon girişi 0xCC),
  ProcessDebugPort/DebugObjectHandle (handle fix'li), GetThreadContext
  DR0-3, TF-probu istisna-bağlamından DRx (core.cc:771 1:1).
  GÖLGE sinyaller tek başına KARAR VERMEZ: TF-yutuldu (0.8.2 çökmesinin
  kaynağı), ntdll API 0xCC taraması (AV hook'ları aynı izi bırakır).
- **(G) TF probu core.cc:771 1:1:** istisna BİZE ulaşırsa bağlamın
  DR0|DR1|DR2|DR3'ü okunup ECX'e yazılır (GetThreadContext'siz DRx —
  GTC hook'lansa bile yakalar); yutulursa gölge.
- **Zehirli dönüş = 0** (0.8.2'de 0xDEADBEEF): "fonksiyon çalışmadı"
  anlamı korundu, oyun çağırıcısı artık çöp değer almıyor.
- **x86 handle fix:** QIP/NtPVM = 0xFFFFFFFF (NtCurrentProcess).
- **CONTEXT tamponları tam boyut:** x64 0x4D0, x86 0x2CC (0.8.2'de 0x100/0x20).
- **xa_module sağlamlık:** DllBase==0 LDR girdileri atlanır (manuel-maplenen
  koruma DLL'lerine karşı).
- **VM_DELTA64/86 0xC00→0x1400:** batarya C çerçevesi (CONTEXT 0x4D0/0x2CC)
  emüle-yığın penceresinin üstünde kalır (0.8.2 x64 bisekt dersinin devamı).

## Kanıtlar
- **x86 Unicorn matrisi (İLK GERÇEK x86 runtime doğrulaması) 14/14 TAM**
  (test/x86_unic_matrix.py → kanit/veh_matrix_x86_083.txt): clean CALISIR,
  tfeaten CALISIR (0.8.2'de FP=çökme), peb/ntglobal/kapicc/dbgport/dbgobj/
  drx/pageguard/tfctxdrx/tfeaten+drx DURDU, apicc CALISIR (hook-FP yok).
- **x64 wine VEH matrisi 5/5** (test/veh_matrix_x64.txt): clean CALISTI rc=42,
  sw DURDU (r=0), vehonly CALISTI (yeni politika), peb DURDU (r=0);
  hw=wine kısıtı (SetThreadContext DR0 self'de etki etmiyor — dr0=0 kanıtı;
  DRx doğrulaması Unicorn [drx]/[tfctxdrx] senaryolarında).
- **R5 ULTIMATE rc=42** (V+M+pack+sectcrypt+loadercrc+filecrc+stripfixups).
- **metin2 d7610 statik:** seed=0xD7610 bc[0]=0x0A→itab→0x1E ANTIDBG ✓,
  giriş 0x55→0x60 kapı ✓ (5232640 bayt).
- Not: r5 tamper-askısı ortam-bağlantılı (0.8.2 kanıt ikilisi de aynı
  ortamda askıda) — 0.8.3 regresyonu değildir.

## Sürüm notu
0.8.2'den geçenler: VM derinliği (CFB crypt_command/key, 577-op uzayı,
polimorfik işleyiciler, SEH, SectionCryptor), SDK-40, lisans, LZMA paket.
