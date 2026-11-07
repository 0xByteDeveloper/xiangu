#!/bin/bash
# ============================================================
# build_all.sh — XProtect 0.3.0 zincirini bastan sona derler
#  1) VM blob'lari (vm_core.c -> vm64.bin / vmx86.bin)
#     NOT: vm_core.c 0.3.0'da GUNCEL (vm_natcall RSP kaydi rbx'te)
#  2) Motor CLI (Linux test kosumu — muttest)
#  3) CPlusPort.exe / CPlusPort32.exe (eski mini GUI, yeni motor)
# UI (XProtectUI.exe) icin XProtectUI_Source/build_ui.sh kullanin.
# ============================================================
set -e
cd "$(dirname "$0")"

echo "== [1/3] VM blob x64 =="
gcc -O2 -ffreestanding -fno-stack-protector -fno-jump-tables \
    -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident \
    -DVM_IS_X64 -DVM_WINDOWS -c vm_core.c -o vm64.o
ld -Ttext=0x100000 -e vm_entry vm64.o -o vm64.elf
objcopy -O binary vm64.elf vm64.bin

echo "== [1/3] VM blob x86 =="
gcc -m32 -O2 -ffreestanding -fno-stack-protector -fno-jump-tables \
    -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-ident \
    -DVM_IS_X86 -DVM_WINDOWS -c vm_core.c -o vmx86.o
ld -m elf_i386 -Ttext=0x100000 -e vm_entry vmx86.o -o vmx86.elf
objcopy -O binary vmx86.elf vmx86.bin

echo "== [1/3] vm_blob.cpp uretimi =="
python3 gen_blob.py

echo "== [2/3] Motor CLI test kosumu (Linux) =="
g++ -O1 -std=c++17 -g -fno-sanitize=undefined \
    muttest.cpp engine_main.cpp engine_pe.cpp engine_disasm.cpp \
    engine_lift.cpp engine_mut.cpp engine_stub.cpp engine_util.cpp \
    engine_sym.cpp vm_blob.cpp -o muttest

echo "== [3/3] CPlusPort.exe (eski mini GUI, yeni motor) =="
ZIG="python3 -m ziglang"
$ZIG c++ -O2 -std=c++17 -target x86_64-windows-gnu -DUNICODE -D_UNICODE \
    cplport_gui.cpp engine_main.cpp engine_pe.cpp engine_disasm.cpp \
    engine_lift.cpp engine_mut.cpp engine_stub.cpp engine_util.cpp \
    engine_sym.cpp vm_blob.cpp -o ../CPlusPort.exe \
    -lcomdlg32 -lcomctl32 -luser32 -lgdi32 -lshell32 -fno-sanitize=undefined
$ZIG c++ -O2 -std=c++17 -target x86-windows-gnu -DUNICODE -D_UNICODE \
    cplport_gui.cpp engine_main.cpp engine_pe.cpp engine_disasm.cpp \
    engine_lift.cpp engine_mut.cpp engine_stub.cpp engine_util.cpp \
    engine_sym.cpp vm_blob.cpp -o ../CPlusPort32.exe \
    -lcomdlg32 -lcomctl32 -luser32 -lgdi32 -lshell32 -fno-sanitize=undefined

echo "== TAMAM =="
ls -la vm64.bin vmx86.bin muttest ../CPlusPort.exe ../CPlusPort32.exe
