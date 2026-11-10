XProtect 0.8.0 — VM Derinlestirme
=================================
0.7.0-final uzerine: CFB crypt_command/key, polimorfik opcode permütasyonu,
komut genisletme (rol/ror/bswap/xchg), SEH dinamik kayit (RtlAddFunctionTable),
SectionCryptor (--sectcrypt: pack'li akislarin CFB-64 sifrelenmesi), adim
butcesi + STRDEC sertligi, movslq + encoder-perm kritik fixleri.

Kullanim (CLI):
  XProtectUI.exe --cli <girdi> <cikti> [--add RVA:RVA:V|M|U ...]
      [--loadercrc] [--filecrc] [--stripfixups] [--stripdebug]
      [--pack] [--sectcrypt] [--pubkey <hex> --prodcode <hex>]

Kanit matrisi: CHANGELOG_0.8.0.md (R1..R10 — hepsi 42 / duru-ret).
Kapi: wine self-test "SELFTEST OK".
