# Releasing the app suite

A release publishes the exact bytes that were installed and checked on hardware.
Builds embed their build time (`__DATE__`/`__TIME__`, the ESP-IDF app
descriptor), so a rebuild can never match accepted bytes. Nothing is rebuilt
between acceptance and publication.

```
release/v<apps>[-...] branch ── release_candidate.yml (dispatch) ── `release-candidate` artifact
        │                                                              │ download, hash, install
        │                                                              ▼
        │                                                  physical acceptance (all packages)
        ▼                                                              │ PASS
annotated tag v<apps> (run id, SHA256SUMS digest, packages) ── release.yml ── DRAFT release
                                                                               │ verify-release
                                                                               ▼
                                                     publish + latest (manual) ── verify-release ── OTA test
```

Scripts: `scripts/release/candidate.py` (tests:
`python3 -m unittest discover -s scripts/release`).

## 1. Candidate

1. Set `apps_version` in `platformio.ini` and commit. Push the commit to a branch
   named `release/v<apps_version>` or `release/v<apps_version>-<suffix>`.
2. Dispatch **Compile Release Candidate** (`release_candidate.yml`) on that branch.
   It builds the release envs cold and fails unless:
   - every build reports `CrossPoint build version: <version>-<PACKAGE>`;
   - the release is exactly the 9 files below;
   - the legacy copies are byte copies;
   - each `.factory.bin` is exactly bootloader, partition table, `boot_app0` and app,
     with `0xFF` everywhere else, NVS included.

   | Env | Package | Files |
   |---|---|---|
   | `gh_release` | `x4-g` | `firmware-x4-g.bin`, legacy `firmware.bin` |
   | `x4pro-goto-gh_release` | `x4pro-g` | `firmware-x4pro-g.bin`, `.factory.bin` |
   | `x4pro-onpoint-gh_release` | `x4pro-p` | `firmware-x4pro-p.bin`, `.factory.bin` |
   | `x4pro-gh_release` | `x4pro-gp` | `firmware-x4pro-gp.bin`, `.factory.bin`, legacy `firmware-x4pro.bin` |

3. Before downloading anything, prove the run built the intended commit:
   ```bash
   gh run view <run-id> --json headSha,headBranch,event,conclusion,jobs
   git ls-remote <remote> refs/heads/release/v<apps>-<suffix>
   ```
   - `headSha` must be the intended commit.
   - The event must be `workflow_dispatch`.
   - Every job must be `success`.
   - The branch must still point at that commit.

   Any mismatch means dispatching a new candidate, never a manual fix.
4. Download `release-candidate` and `x4pro-flash-support`, keep them read-only, and check them:
   ```bash
   python3 scripts/release/candidate.py verify-dir release-candidate \
     --support x4pro-flash-support --commit <sha> --run-id <run-id>
   ```
   Record `sha256(SHA256SUMS.txt)`. That digest is what gets accepted.

## 2. Physical acceptance (every package)

Install only the candidate's own files. Every row must pass. The tag's
`physically-accepted` line must list exactly every package; `release.yml` refuses
anything less.

| Package | Device | Install | Checks |
|---|---|---|---|
| `x4-g` | X4 | microSD: `firmware-x4-g.bin`, Settings > System > SD Card Firmware Update | full |
| `x4pro-gp` | X4 Pro | USB (§4): `x4pro-gh_release` pieces + `firmware-x4pro-gp.bin` | full |
| `x4pro-g` | X4 Pro | USB (§4): `x4pro-goto-gh_release` pieces + `firmware-x4pro-g.bin` | boot |
| `x4pro-p` | X4 Pro | USB (§4): `x4pro-onpoint-gh_release` pieces + `firmware-x4pro-p.bin` | boot |

Afterwards, reinstall `x4pro-gp` on the X4 Pro.

- **Full:**
  - the device boots;
  - Settings shows the version `<version>-<PACKAGE>`;
  - Home > Apps lists the package's apps and Tools, and nothing else;
  - GOTO loads an edition (`g`);
  - ON POINT opens (`p`);
  - Calculator and Calendar open, and Clock opens on the X4 Pro;
  - a book opens and turns pages;
  - the X4 Pro keeps its Wi-Fi network and settings;
  - Settings > System > Check for updates offers nothing older.
- **Boot:**
  - the device boots;
  - the version reads `<version>-G` or `-P`;
  - Home > Apps lists that package's apps;
  - the package's app opens.

The X4 has no USB data connection and is installed only from microSD. The
X4 Pro is installed only over USB.

## 3. Tag, draft, publish

1. Only after every row passes, create an **annotated** tag on the candidate commit and push it:
   ```
   v1.1.1

   candidate-run: <run-id>
   candidate-sums-sha256: <sha256 of SHA256SUMS.txt>
   physically-accepted: x4-g x4pro-g x4pro-p x4pro-gp
   ```
2. **Publish Release Draft** (`release.yml`) checks, before creating anything:
   - the tag equals `v<apps_version>`, is annotated, and points directly at the commit;
   - the run is this repository's `release_candidate.yml` (by workflow id, and by path
     with any `@ref` suffix removed);
   - the run was dispatched on `release/v<apps_version>*` at exactly the tagged commit, and succeeded;
   - every package was physically accepted;
   - the run's artifact matches the accepted digest.

   It then creates a **draft** from those files, never marked latest, and checks the
   draft's assets against GitHub's own sha256 digests.
3. Check the draft yourself, publish it, and check again:
   ```bash
   gh api repos/<owner>/<repo>/releases --jq '.[] | select(.tag_name=="v1.1.1" and .draft)' > draft.json
   python3 scripts/release/candidate.py verify-release draft.json --sums SHA256SUMS.txt --tag v1.1.1 --state draft
   gh release edit v1.1.1 --draft=false --latest
   gh api repos/<owner>/<repo>/releases/tags/v1.1.1 > published.json
   python3 scripts/release/candidate.py verify-release published.json --sums SHA256SUMS.txt --tag v1.1.1 \
     --state published --require-immutable
   gh api repos/<owner>/<repo>/releases/latest --jq .tag_name   # v1.1.1
   ```

**Release asset integrity.** `verify-release` fails unless every one of these holds:
- the release is that tag, and in the expected draft or published state, and not a prerelease;
- its assets are exactly the accepted files plus `SHA256SUMS.txt`, with nothing missing, nothing extra and no duplicates;
- every asset is `uploaded`;
- GitHub's `digest` for each asset is `sha256:` plus the accepted digest;
- `--require-immutable` is passed and the release is marked `immutable`.

- **With immutability on:** the repository setting *Release immutability* must be on before publishing. Then the published release's assets and tag cannot be changed.
- **Re-verify regardless:** anything that later relies on a release re-fetches these digests and fails closed on any difference. That covers anything that verifies devices or syncs a firmware contract.

## 4. X4 Pro USB install (NVS kept)

Write the four pieces at their offsets. NVS (`0x9000`–`0xdfff`: Wi-Fi, settings) is never written.

| Offset | Piece |
|---|---|
| `0x0` | `bootloader.bin` |
| `0x8000` | `partitions.bin` |
| `0xe000` | `boot_app0.bin` (otadata: boot `app0`) |
| `0x10000` | `firmware-x4pro-<package>.bin` |

`boot_app0` resets the boot selection, so the device boots the new image whichever
OTA slot it ran before. A `.factory.bin` written at `0x0` would also erase NVS.

```bash
esptool.py --chip esp32s3 --port <port> --baud 921600 write_flash -z --flash_mode dio --flash_freq 80m \
  --flash_size detect 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 <app>.bin
```

- Every region must report `Hash of data verified.`
- For a published release (for example, returning to the previous version before an OTA test), cut the pieces from its `.factory.bin`. The app slice must equal the release's own app file:
  ```bash
  python3 scripts/release/candidate.py slice-factory firmware-x4pro-gp.factory.bin --out pieces \
    --app firmware-x4pro-gp.bin
  ```

## 5. Update path

After publication:
1. Put each device on the previous release.
2. Run Settings > System > Check for updates. It must offer the new version; install it over Wi-Fi.
3. Repeat the full checks.
4. On the X4 Pro, read the OTA slot back (`esptool.py read_flash <slot offset> <size> slot.bin`, read-only) and compare its sha256 with the published app's digest.
