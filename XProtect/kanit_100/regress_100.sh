#!/bin/bash
# regress_100.sh — 1.0.0-UO tam regresyon (0.9.0 tabani + cmNor/cmNand yigin
# kapilari; xguard v3 gozetiminde kosulur).
# Kapilar: T1 SELFTEST | T2 deepvm x64 bayt-bayt | T3 x86 Unicorn diferansiyeli
#          T4 R5 rc=42 | T5 tamper 117 | T6 md5 determinizmi | T7 metin2 statik
# NOT: 0.9.5/0.10.0'daki IR-junk/nonce testleri KALDIRILDI (ozellikler geri alindi).
# STAGE=1 -> T1-T3 | STAGE=2 -> T4-T7 | bos -> hepsi
STAGE="${STAGE:-}"
set -e
cd /home/z/my-project/scripts/xb100u
K=kanit_100
mkdir -p $K
W=/home/z/my-project/scripts/wchroot.sh
XG=/home/z/my-project/scripts/xguard
PY="python3 -m ziglang"
# chroot icin Windows-yolu (wine Z: -> /)
U='Z:\home\z\my-project\scripts\xb100u'
KK='Z:\home\z\my-project\scripts\xb100u\kanit_100'
M2='Z:\home\z\my-project\scripts\metin2\metin2client.exe'
PASS=0; FAIL=0
ok(){ echo "PASS $*"; PASS=$((PASS+1)); }
no(){ echo "FAIL $*"; FAIL=$((FAIL+1)); }

if [ -z "$STAGE" ] || [ "$STAGE" = 1 ]; then
echo "== T1: SELFTEST =="
$XG run --tmo 180 -- $W "$U\\XProtectUI.exe" --selftest > $K/selftest100.log 2>&1 || true
grep -q "SELFTEST OK" $K/selftest100.log && ok "T1 SELFTEST" || { no "T1 SELFTEST"; tail -5 $K/selftest100.log; }

echo "== T2: test_deepvm64 x64 baz + V-korumali bayt-bayt =="
$PY cc -target x86_64-windows-gnu -O1 -fno-omit-frame-pointer \
  test_deepvm.c -o $K/test_deepvm64.exe -lkernel32 -luser32 2> $K/zig_deepvm.log \
  || { no "T2 derleme"; tail -5 $K/zig_deepvm.log; }
$XG run --tmo 120 -- $W "$KK\\test_deepvm64.exe" > /dev/null 2>&1 || true
cp /tmp/my-project/wine-root/deepvm_sonuc.txt $K/deepvm_baz.txt 2>/dev/null || cp deepvm_sonuc.txt $K/deepvm_baz.txt 2>/dev/null || no "T2 baz-kos"
grep -q "SON" $K/deepvm_baz.txt || { no "T2 baz-kos"; cat $K/deepvm_baz.txt; }
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_deepvm64.exe" "$KK\\test_deepvm64_vmp.exe" --type V --all > $K/prot64_V.log 2>&1 || true
grep -a "KAYIT" $K/prot64_V.log >/dev/null && ok "T2a V-protect" || { no "T2a V-protect"; tail -5 $K/prot64_V.log; }
$XG run --tmo 120 -- $W "$KK\\test_deepvm64_vmp.exe" > /dev/null 2>&1 || true
cp /tmp/my-project/wine-root/deepvm_sonuc.txt $K/deepvm_kor.txt 2>/dev/null || cp deepvm_sonuc.txt $K/deepvm_kor.txt 2>/dev/null || true
if diff -q $K/deepvm_baz.txt $K/deepvm_kor.txt >/dev/null; then ok "T2b bayt-bayt (GATE dahil)"; else no "T2b bayt-bayt"; diff $K/deepvm_baz.txt $K/deepvm_kor.txt | head -8; fi

echo "== T3: x86 Unicorn diferansiyeli (oyun mimarisi PE32) =="
$PY cc -target x86-windows-gnu -O1 -fno-omit-frame-pointer \
  test_deepvm.c -o $K/test_deepvm86.exe -lkernel32 -luser32 2>> $K/zig_deepvm.log
# x86 koşturma wine'da imkansiz (wow64'suz) — CIKTI x64 UI'den (capraz-mimari protect)
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_deepvm86.exe" "$KK\\test_deepvm86_vmp.exe" --type V --all > $K/prot86_V.log 2>&1 || true
grep -a "KAYIT" $K/prot86_V.log >/dev/null && ok "T3a x86 V-protect" || { no "T3a x86 V-protect"; tail -5 $K/prot86_V.log; }
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_deepvm86.exe" "$KK\\test_deepvm86_U.exe" --type U --all > $K/prot86_U.log 2>&1 || true
grep -a "KAYIT" $K/prot86_U.log >/dev/null && ok "T3b x86 U-protect" || { no "T3b x86 U-protect"; tail -5 $K/prot86_U.log; }
python3 -c "import unicorn" 2>/dev/null || pip install -q unicorn pefile 2>>$K/zig_deepvm.log || true
python3 unic_deepvm_x86.py $K/test_deepvm86.exe $K/test_deepvm86_vmp.exe > $K/unic_V.log 2>&1 || true
grep -qE "TAMAM|TUMU ESIT|OK" $K/unic_V.log && ok "T3c Unicorn V eslesme" || { no "T3c Unicorn V"; tail -6 $K/unic_V.log; }
python3 unic_deepvm_x86.py $K/test_deepvm86.exe $K/test_deepvm86_U.exe > $K/unic_U.log 2>&1 || true
grep -qE "TAMAM|TUMU ESIT|OK" $K/unic_U.log && ok "T3d Unicorn U eslesme" || { no "T3d Unicorn U"; tail -6 $K/unic_U.log; }
fi

if [ -z "$STAGE" ] || [ "$STAGE" = 2 ]; then
echo "== T4: R5 ULTIMATE (sdkonly+pack+sectcrypt+loadercrc+filecrc+stripfixups) =="
$PY cc -target x86_64-windows-gnu -O1 \
  sdk_ornek_test_rt3.c -o $K/test_rt3.exe 2>> $K/zig_deepvm.log
# 0.8.5 dersi: zig CRT'si TLS callback ekler -> pack TLS'li RED (0.6.0 kurali).
# Uygulama TLS kullanmaz — test-prep'te TLS data-dir temizlenir (yalniz TEST exe'si).
python3 - "$K/test_rt3.exe" <<'EOF'
import sys, pefile
p = sys.argv[1]
pe = pefile.PE(p)
tls = pe.OPTIONAL_HEADER.DATA_DIRECTORY[9]
if tls.VirtualAddress and tls.Size:
    tls.VirtualAddress = 0; tls.Size = 0
    pe.write(p)
    print("test-prep: TLS data-dir temizlendi (zig CRT artefakti)")
else:
    print("test-prep: TLS yok")
EOF
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_rt3.exe" "$KK\\r5_100_n.exe" \
  --sdkonly --pack --sectcrypt --loadercrc --filecrc --stripfixups --rt-nomsg \
  > $K/r5_prot.log 2>&1 || true
grep -aq "KAYIT" $K/r5_prot.log && ok "T4a R5-protect" || { no "T4a R5-protect"; grep -a "OZET\|HATA" $K/r5_prot.log | tail -3; }
rc=0; $XG run --tmo 120 -- $W "$KK\\r5_100_n.exe" > $K/r5_run.log 2>&1 || rc=$?
grep -q "OK hepsi dogru" $K/r5_run.log && [ "$rc" = 42 ] && ok "T4b R5 rc=42" || no "T4b R5 rc=$rc"

echo "== T5: tamper (paketli cikti tek-bayt) =="
python3 - "$K/r5_100_n.exe" <<'EOF'
import sys, pefile
p = sys.argv[1]
pe = pefile.PE(p)
# 0.8.5/0.9.0 altin standardi: .vmp1 (stub) tamper -> rc=117 duru-ret
sec = [s for s in pe.sections if s.Name.rstrip(b'\x00') == b'.vmp1']
if not sec: sec = [s for s in pe.sections if b'.vmp' in s.Name]
s = sec[0]
off = s.PointerToRawData + 512
with open(p, 'r+b') as f:
    f.seek(off); b = f.read(1)
    f.seek(off); f.write(bytes([b[0] ^ 0x55]))
print(f"tamper: {s.Name.decode().strip(chr(0))} raw+512 bayt cevirildi")
EOF
rc=0; $XG run --tmo 120 -- $W "$KK\\r5_100_n.exe" > $K/r5_tamper.log 2>&1 || rc=$?
[ "$rc" = 117 ] && ok "T5 tamper rc=117" || no "T5 tamper rc=$rc (beklenen 117)"

echo "== T6: md5 determinizmi (ayni girdi -> ayni cikti) =="
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_rt3.exe" "$KK\\md5_a.exe" --sdkonly --rt-nomsg > /dev/null 2>&1 || true
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$KK\\test_rt3.exe" "$KK\\md5_b.exe" --sdkonly --rt-nomsg > /dev/null 2>&1 || true
MA=$(md5sum $K/md5_a.exe | cut -d' ' -f1); MB=$(md5sum $K/md5_b.exe | cut -d' ' -f1)
[ -n "$MA" ] && [ "$MA" = "$MB" ] && ok "T6 md5-ozdes ($MA)" || no "T6 md5 farkli ($MA vs $MB)"

echo "== T7: metin2 d7610 ULTRA+antidebug statik ANTIDBG kaniti =="
$XG run --tmo 300 -- $W "$U\\XProtectUI.exe" --cli "$M2" "$KK\\metin2_d7610_100.exe" --add D7610:D766C:V --antidebug --rt-nomsg > $K/m2_prot.log 2>&1 || true
grep -a "KAYIT" $K/m2_prot.log >/dev/null && ok "T7a metin2-protect" || { no "T7a metin2-protect"; tail -4 $K/m2_prot.log; }
python3 static_m2_090.py $K/metin2_d7610_100.exe > $K/static_m2.log 2>&1 || true
grep -q "0x1E" $K/static_m2.log && ok "T7b statik ANTIDBG 0x1E" || { no "T7b statik"; tail -5 $K/static_m2.log; }
python3 bc_census_100.py $K/metin2_d7610_100.exe > $K/census_m2.log 2>&1 || true
grep -q "YAPI-SAGLAM" $K/census_m2.log && ok "T7c bc-census uctan-uca + KAPI" || { no "T7c bc-census"; tail -4 $K/census_m2.log; }
fi

echo
if [ "$STAGE" = 1 ]; then echo "===== STAGE1 OZET: PASS=$PASS FAIL=$FAIL ====="; [ "$FAIL" = 0 ]; fi
if [ "$STAGE" = 2 ]; then echo "===== STAGE2 OZET: PASS=$PASS FAIL=$FAIL ====="; [ "$FAIL" = 0 ]; fi
if [ -z "$STAGE" ]; then echo "===== OZET: PASS=$PASS FAIL=$FAIL ====="; [ "$FAIL" = 0 ]; fi
echo "(stage '$STAGE' bitti)"
