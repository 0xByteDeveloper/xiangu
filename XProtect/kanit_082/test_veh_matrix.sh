#!/bin/bash
# test_veh_matrix.sh — 0.8.2 CE-VEH yakalama diferansiyel test matrisi
# Kullanım: bash test_veh_matrix.sh [x64|x86]
# Kanıt çıktıları: test/veh_matrix_<arch>.txt
set -u
cd "$(dirname "$0")/.."
ARCH="${1:-x64}"
MGW=/tmp/mgw/usr/bin
ZIG="python3 -m ziglang"
WINE=/home/z/my-project/scripts/xp081/wine64x.sh
OUT=test/veh_matrix_$ARCH.txt
: > $OUT

# 1) derle
if [ "$ARCH" = "x64" ]; then
  BIN=test/test_veh64.exe
  "$MGW/x86_64-w64-mingw32-gcc-posix" -O2 -o $BIN test/test_veh.c -static -lkernel32 2>> $OUT \
    || { echo "DERLEME HATASI" >> $OUT; exit 1; }
else
  BIN=test/test_veh32.exe
  $ZIG cc -O2 -target x86-windows-gnu -o $BIN test/test_veh.c -static -lkernel32 2>> $OUT \
    || { echo "DERLEME HATASI" >> $OUT; exit 1; }
fi

# 2) damage gerçek RVA/boyut (find_damage_rva.py: .pdata / prolog taraması)
read RVA SZ <<< "$(python3 test/find_damage_rva.py $BIN)"
echo "damage RVA=0x$RVA size=0x$SZ" >> $OUT
if [ "$RVA" = "0" ]; then echo "RVA BULUNAMADI" >> $OUT; exit 1; fi
END=$((0x$RVA + 0x$SZ))

# 3) koru + koş: U + antidebug + memprot (kullanıcı senaryosu)
# UI GUI-subsystem'dir: Xvfb gerekir (0.8.1 dersnotu — aynı komut içinde
# başlat, komut bitince süreç ölür)
XVFB_PID=""
if command -v Xvfb >/dev/null 2>&1; then
  Xvfb :97 -screen 0 1024x768x16 >/dev/null 2>&1 &
  XVFB_PID=$!
  sleep 0.4
  export DISPLAY=:97
fi
SUF=$([ "$ARCH" = "x64" ] && echo 64 || echo 32)
for MODE in clean sw hw vehonly peb; do
  "$WINE" "Z:\\home\\z\\my-project\\scripts\\xp081\\XProtectUI.exe" --cli "Z:\\home\\z\\my-project\\scripts\\xp081\\$BIN" "Z:\\home\\z\\my-project\\scripts\\xp081\\test\\veh_${ARCH}_${MODE}.exe" \
      --add 0x$RVA:$(printf %X $END):U --antidebug --memprot > test/veh_${ARCH}_${MODE}.log 2>&1
  rm -f test/veh_sonuc$SUF.txt
  "$WINE" "Z:\\home\\z\\my-project\\scripts\\xp081\\test\\veh_${ARCH}_${MODE}.exe" $MODE >> $OUT 2>&1
  RC=$?
  RES=$(cat test/veh_sonuc$SUF.txt 2>/dev/null | tail -1)
  RVAL=$(echo "$RES" | sed -n 's/.*r=\([-0-9]*\).*/\1/p')
  if   [ "$RVAL" = "3812" ]; then ST=CALISTI
  elif [ -z "$RVAL" ];       then ST=DURDU
  else                        ST=DURDU; fi
  if [ "$MODE" = clean ]; then WANT=CALISTI; else WANT=DURDU; fi
  VERD=$([ "$ST" = "$WANT" ] && echo GECTI || echo HATA)
  echo "[$MODE] $RES rc=$RC -> $ST (beklenen $WANT) = $VERD" >> $OUT
done
if [ -n "$XVFB_PID" ]; then kill $XVFB_PID 2>/dev/null; fi
echo "=== MATRIS SONUC ($ARCH) ==="
cat $OUT
