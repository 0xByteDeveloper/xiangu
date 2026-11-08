#!/bin/bash
# ============================================================
# build_ui.sh — XProtectUI'yi bastan sona derler (x64 + x86)
# Gereksinim: pip install ziglang  (python3 -m ziglang)
# Kullanim:  bash build_ui.sh
# Cikti:     XProtectUI.exe (x64) + XProtectUI32.exe (x86)
# ============================================================
set -e
cd "$(dirname "$0")"
ZIG="python3 -m ziglang"

echo "== [1/2] XProtectUI.exe (x64) =="
$ZIG c++ -O2 -std=c++17 -target x86_64-windows-gnu -DUNICODE -D_UNICODE \
    xprotect_ui.cpp engine_main.cpp engine_pe.cpp engine_disasm.cpp engine_mut.cpp \
    engine_lift.cpp engine_stub.cpp engine_util.cpp engine_sym.cpp vm_blob.cpp xrt_blob.cpp \
    -o XProtectUI.exe \
    -lcomctl32 -lcomdlg32 -lshell32 -lgdi32 -luser32 -fno-sanitize=undefined

echo "== [2/2] XProtectUI32.exe (x86) =="
$ZIG c++ -O2 -std=c++17 -target x86-windows-gnu -DUNICODE -D_UNICODE \
    xprotect_ui.cpp engine_main.cpp engine_pe.cpp engine_disasm.cpp engine_mut.cpp \
    engine_lift.cpp engine_stub.cpp engine_util.cpp engine_sym.cpp vm_blob.cpp xrt_blob.cpp \
    -o XProtectUI32.exe \
    -lcomctl32 -lcomdlg32 -lshell32 -lgdi32 -luser32 -fno-sanitize=undefined

echo "== TAMAM =="
ls -la XProtectUI.exe XProtectUI32.exe
