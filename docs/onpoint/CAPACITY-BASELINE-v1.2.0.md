# ON POINT capacity baseline: v1.2.0 (Stage 1, §3)

Local provenance and capacity record of the published **v1.2.0** release, the baseline for ON POINT's
regional-variant work. Raw logs, images, ELFs and maps are kept outside the repository (not committed).

## Inputs

| Item | Value |
|---|---|
| Commit | `85bad5424d6dd91966aa997f46b426a823ca83db` (tag `v1.2.0`, annotated tag object `23733059…`) |
| Tag binding | candidate-run `38630358450`, candidate-sums-sha256 `5477cfce5c953981b790a0cbcb03cf5aa251f2534f9effdac59160ae33fff5e7`, acceptance-sha256 `3922e7e8…642ab` |
| Submodules | `freeink-sdk` `111fdcc7f0176c3ee38391a160ee296bf492dbd8`; `freeink-sdk/libs/assets/Icons/lucide` `c81680e066f45b640743ca78ae36cdedda3f0318` |
| Working tree | clean (detached disposable worktree; recursive submodule init) |
| PlatformIO Core | 6.1.19 (= CI pin) |
| Platform | espressif32 55.3.311 (pioarduino release zip, as `platformio.ini`) |
| Framework | framework-arduinoespressif32 3.3.11; -libs 5.5.5+sha.b774170ff46; framework-espidf 3.50505.0 (ESP-IDF v5.5.5) |
| Toolchain | toolchain-xtensa-esp-elf 14.2.0+20260121; tool-esptoolpy 5.3.0 |
| Host | macOS arm64 (Apple M3 Pro). CI: `ubuntu-latest`, Python 3.13.15 |
| Builds | clean and forced: `.pio/build/<env>` removed, `PLATFORMIO_BUILD_CACHE_DIR` = fresh empty directory, every build |
| Resolved flags | `pio run -e <env> -t idedata` per env (defines and C/C++ flags recorded). Package defines: P `ON_POINT_ENABLED`; PD + `DOGEAR_ENABLED`; GP `GOTO_ENABLED` + `ON_POINT_ENABLED`; GPD all three; all `FREEINK_DEVICE_X4PRO`, `LOG_LEVEL=1`, `CROSSPOINT_OTA_VERSION="1.2.0"` |

## Published release verification

`gh release download v1.2.0`: all 17 assets verified with `shasum -a 256 -c SHA256SUMS.txt`, and each against
GitHub's own asset digest. `sha256(SHA256SUMS.txt)` = `5477cfce…` = the tag's candidate-sums-sha256.

## Sizes and headroom (app slot `0x640000` = 6,553,600 B)

Headroom is from the **published** image. Section sizes are from the local ELF (`objdump -h`). ON POINT = symbols in
`on_point::` / `OnPointActivity`, from the local ELF.

| Pkg | Env | Published asset | Published bytes | Slot used | **Headroom** | Local bytes | `.flash.text` | `.flash.rodata` | `.dram0.data` | `.dram0.bss` | IRAM | ON POINT text / rodata |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| P | `x4pro-onpoint-gh_release` | `firmware-x4pro-p.bin` | 5,678,832 | 86.65% | **874,768** | 5,678,992 (+160) | 2,160,948 | 3,398,292 | 29,615 | 72,256 | 85,119 | 9,495 / 2,336 |
| PD | `x4pro-onpoint-dogear-gh_release` | `firmware-x4pro-pd.bin` | 5,745,744 | 87.67% | **807,856** | 5,745,904 (+160) | 2,213,608 | 3,412,548 | 29,615 | 72,256 | 85,119 | 9,495 / 2,336 |
| GP | `x4pro-gh_release` | `firmware-x4pro-gp.bin` | 5,696,560 | 86.92% | **857,040** | 5,696,720 (+160) | 2,174,768 | 3,402,196 | 29,615 | 72,256 | 85,119 | 9,495 / 2,336 |
| GPD | `x4pro-all-gh_release` | `firmware-x4pro-gpd.bin` | 5,763,440 | 87.94% | **790,160** | 5,763,600 (+160) | 2,227,604 | 3,416,244 | 29,615 | 72,256 | 85,119 | 9,495 / 2,336 |

Static internal RAM (`.dram0.data` + `.dram0.bss`) is 101,871 B in every package. ON POINT allocates nothing on the
heap and keeps its tables in `.flash.rodata`.

## Hashes

| Pkg | Published `.bin` | Local run-1 `.bin` | Local run-1 `.elf` | Local run-1 `.map` |
|---|---|---|---|---|
| P | `5d6a59e3a125fa79dbdf735eee1ef83a2f6c93c380aeef155ead0fce1a26dc95` | `76c3a98c03772004685b7f4f480395202f9e01cb56a04e2c87be0f0244613411` | `39f452c7787288903a13f9c2284b8c6f78c37c3abca4e777a56a11f964860571` | `17277b3bd0842e2d5409687ad66c81530439a09f3c3db520546b543f2de158b4` |
| PD | `cdd6079a6316e1b8a5af811dea7ab0f0d190dcd87c0eaed295726d7622fa1edb` | `fa59c7c79c2cce46a1d1c2f0a3892779272410fafa8d1811a17fc0fd4460409c` | `8ab793cc0ff6545df06dc56461e0590930fecd326ee5ece8b30335ff7e755fad` | `32d63c558aab5c8b82bb2e4983de64132f8bab09909e4674d5ad5f49c8995e7d` |
| GP | `ee894df34d4556d87455c01bcdcca6ac9236ce45520bb069f692648d3f900949` | `1e05c1b24c21c9aac79769cde88bb192d5072d795fbd10115288087a217cc2de` | `d75c3530f3f8bbc8c9bba8b1d1aa36e15408001637d9ce8a5af1a035a71727c4` | `525a67ab9b16c7258fa0250225099739ddba34c3bae936f62f1560e044c43176` |
| GPD | `009d6a8ecf001aebb728a77da78c987206e9f7aafff45a06949903754631c0f8` | `72c78476fb9ea24b0c299fec05f7bf859de4b9e1e811c41d3166059b433fd6c3` | `2b904e0677f4ab70f56329dfad73bbfecf7208924aa97a54eeb4e73dbf9fa478` | `ca6b9dd182f1f530026cd03c4ffd23d482bd4ad1c806b8c0e5661d286f934fdd` |

Image metadata (`esptool image-info`), identical in published and local images:
- 7 segments; app descriptor "arduino-lib-builder" / `ee57070` / compile time `Jul 20 2026 10:51:03` (the
  prebuilt Arduino libraries' own descriptor, constant across builds); ESP-IDF v5.5.5.
- The descriptor's ELF SHA-256 field is the project ELF's hash. It therefore differs with every build: published
  `ae5d3044…`, local run 1 `39f452c7…` (= the local ELF).

## Normalised comparison (§3.4)

**Equal size is only a sanity check, and it does not hold here:** every local image is 160 B larger than the
published one.

| Pkg | N1: run 1 vs run 2 (same path) | N3: size vs published | N3: memory regions | Strings compared | Web-page ETags differ | Residual runs (local / published) |
|---|---|---|---|---|---|---|
| P | 71 B in 4 ranges, all build-time | +160 B = 32 paths × 5 | DROM +160; IROM/IRAM/DRAM identical | 30,231 | 5 | 8 / 6 |
| PD | 74 B in 4 ranges, all build-time | +160 B = 32 paths × 5 | DROM +160; IROM/IRAM/DRAM identical | 30,550 | 5 | 15 / 11 |
| GP | 73 B in 4 ranges, all build-time | +160 B = 32 paths × 5 | DROM +160; IROM/IRAM/DRAM identical | 30,343 | 5 | 11 / 8 |
| GPD | 75 B in 4 ranges, all build-time | +160 B = 32 paths × 5 | DROM +160; IROM/IRAM/DRAM identical | 30,652 | 5 | 9 / 8 |

**N1 (two clean local builds, minutes apart).** Every differing byte is attributed to build time:
- two `__TIME__` strings;
- the app descriptor's ELF SHA-256 field (`esp_app_desc+0x90`, 32 B);
- the image checksum byte and the appended SHA-256.

Nothing else differs.

**N2 (`SOURCE_DATE_EPOCH` pinned to the published `Oct 14 2026 20:19:25`).** This doesn't reproduce CI. CI compiled
the two time-stamped translation units at different moments (for example P: `20:20:33` and `20:21:22`). A single
epoch makes both strings identical, the linker merges them, and the image shrinks by 16 B, shifting the layout. One
epoch can't reproduce two compile times, so N2 is recorded as **not applicable**.

**N3 (local vs published), after normalisation.** The comparison maps the PlatformIO path prefix to one token, masks
build date and time, and strips little-endian pointer fragments symmetrically.
- **Size:** each image differs by exactly **+160 B = 32 embedded framework source paths × 5 B**
  (`<home>/.platformio` is 5 characters longer than CI's `/home/runner/.platformio`). The paths come from framework
  assert and log strings.
- **Memory regions:** read-only data (DROM) +160 B. Code (IROM), IRAM and DRAM totals are byte-for-byte equal in
  size, and every segment load address matches except where DRAM is split around the flash-mapping alignment gap.
- **Strings:** about 30,000 printable strings per image are identical after normalisation, except:
  1. **5 web-page ETags** (`scripts/build_html.py`: SHA-256 of each gzip-compressed page; `mtime=0`, level 9). The
     compressed bytes differ between this host and CI. 🔶 Attributed to the zlib version (local Python zlib 1.2.12;
     CI's Python 3.13 zlib not recorded).
  2. **6–15 residual runs per package** whose text is identical and whose adjacent byte differs (data/pointer
     boundaries moved by the layout shift).

**Result:** the local builds are **normalised-equivalent with attributed differences**, not byte-reproduced. Every
difference is explained by build time, the CI build path, or web-page compression. Optional N4 (a Linux container
rebuild at CI's checkout path, Q4) was **not run**: it needs separate approval. It would remove the path and possibly
the zlib differences, though not the two CI compile times.
