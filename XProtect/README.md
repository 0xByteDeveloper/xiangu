# XProtect — VMProtect benzeri koruma motoru (C++ portu)

Bu klasör, `universalObfusPublicSrc` (VMProtect 3.5.1 kaynak ağacı) referans alınarak
geliştirilen **CPlusPort / XProtect** projesinin kalıcı evidir. Sandbox sıfırlanmalarına
karşı her şey burada tutulur.

## Klasörler

| Yol | İçerik |
| --- | --- |
| `releases/` | Sürümlü resmi paketler (XProtect_0.x.x.zip) + MD5'leri |
| `source/`   | Son sürümün kaynak ağacı (CPlusPort_Source, Test_exe_Source, VMPHelper_Source) |
| `Config_info.md` | universalObfusPublicSrc TAM özellik envanteri + port durumu matrisi |
| `worklog.md` | Kronolojik iş günlüğü (istek → yapılan → zip → MD5) |

## Sürümler

| Sürüm | Tarih | Özet |
| --- | --- | --- |
| 0.1.0 | 2026-10-20 | İlk resmi sürüm: sanallaştırma zinciri (x64+x86), CPlusPort.exe/32 GUI, VMPHelper v1.3, Test oyunu + korumalı çıktılar, wine64 birebir doğrulama |
| 0.2.0 | 2026-11-07 | Win32 API UI (XProtectUI x64+x86, universalObfusPublicSrc GUI birebir replika) + motor gömülü derleme + wine64/Xvfb selftest |

## Referanslar

- VMProtect kaynak zipleri: `universalObfusPublicSrc.zip` + `universalObfuspublicsrc2.zip`
- Port takibi: `Config_info.md` (özellik matrisi, dosya:satır kanıtlı)
- Test oyunu: MiniPac (Test.exe x64 / Test32.exe x86)
