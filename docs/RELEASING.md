# Releasing the app suite

A release publishes the exact bytes that were installed and checked on hardware.
Builds embed their build time (`__DATE__`/`__TIME__`, the ESP-IDF app
descriptor), so a rebuild can never match accepted bytes. Nothing is rebuilt
between acceptance and publication.

```
release/v<apps>[-...] branch ── release_candidate.yml (dispatch) ── `release-candidate` artifact
        │                                                              │ download, hash, install
        │                                                              ▼
        │                                    physical acceptance (all packages) -> ACCEPTANCE.json
        ▼                                                              │ PASS, owner-approved
annotated tag v<apps> (run, commit, SHA256SUMS digest, packages, record digest) ── release.yml ── DRAFT
                                                                               │ verify-release
                                                                               ▼
                                       candidate.py publish (preflight, then publish + latest) ── OTA test
```

Scripts: `scripts/release/candidate.py`, with the acceptance schema
`scripts/release/acceptance-v1.json` (tests:
`python3 -m unittest discover -s scripts/release` and
`python3 -m unittest scripts/test_git_branch.py`).

## 1. Candidate

1. Set `apps_version` in `platformio.ini` and commit. Push the commit to a branch
   named `release/v<apps_version>` or `release/v<apps_version>-<suffix>`.
2. Dispatch **Compile Release Candidate** (`release_candidate.yml`) on that branch.
   It builds the release envs cold and fails unless:
   - every build reports `CrossPoint build version: <version>-<PACKAGE>`;
   - the release is exactly the 16 firmware files below plus `e3-host-evidence.json`;
   - every host test passes on the run's own commit, and each required E3 host
     test (`hostCovered` in `acceptance-v1.json`) ran and passed, none skipped
     (the `e3-host-tests` job; its `e3-host-evidence.json` is a release file);
   - the legacy copies are byte copies;
   - each `.factory.bin` is exactly bootloader, partition table, `boot_app0` and app,
     with `0xFF` everywhere else, NVS included.

   | Env | Package | Files |
   |---|---|---|
   | `gh_release` | `x4-g` | `firmware-x4-g.bin`, legacy `firmware.bin` |
   | `x4-goto-dogear-gh_release` | `x4-gd` | `firmware-x4-gd.bin` |
   | `x4pro-goto-gh_release` | `x4pro-g` | `firmware-x4pro-g.bin`, `.factory.bin` |
   | `x4pro-goto-dogear-gh_release` | `x4pro-gd` | `firmware-x4pro-gd.bin`, `.factory.bin` |
   | `x4pro-onpoint-gh_release` | `x4pro-p` | `firmware-x4pro-p.bin`, `.factory.bin` |
   | `x4pro-onpoint-dogear-gh_release` | `x4pro-pd` | `firmware-x4pro-pd.bin`, `.factory.bin` |
   | `x4pro-gh_release` | `x4pro-gp` | `firmware-x4pro-gp.bin`, `.factory.bin`, legacy `firmware-x4pro.bin` |
   | `x4pro-all-gh_release` | `x4pro-gpd` | `firmware-x4pro-gpd.bin`, `.factory.bin` |

   The package letters are `g` (GOTO), `p` (ON POINT) and `d` (DOGEAR, opt-in:
   only an env whose own `custom_package` lists it; see `scripts/git_branch.py`).
   This is the canonical package order everywhere: the tag, the acceptance
   record and the tooling.

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

## 2. Physical acceptance (every package, before the tag)

Install only the candidate's own files: the X4 from microSD
(`firmware-x4-<package>.bin`, Settings > System > SD Card Firmware Update), the
X4 Pro over USB only (§4: the env's pieces from `x4pro-flash-support` plus
`firmware-x4pro-<package>.bin`). The X4 has no USB data connection; the X4 Pro is
never installed from microSD.

**Required checks.** `scripts/release/acceptance-v1.json` defines every check
(by id, with what PASS means) and exactly which checks each package must record:

| Package | Level | Checks |
|---|---|---|
| `x4-g` | full | `install-bytes boots version-string apps-exact goto-edition tools-open book-reads update-check` |
| `x4-gd` | full + DOGEAR | the `x4-g` checks + the 12 DOGEAR checks |
| `x4pro-g` | boot | `install-bytes boots version-string apps-exact goto-opens` |
| `x4pro-gd` | boot | the `x4pro-g` checks + `dogear-opens` |
| `x4pro-p` | boot | `install-bytes boots version-string apps-exact onpoint-opens` |
| `x4pro-pd` | boot | the `x4pro-p` checks + `dogear-opens` |
| `x4pro-gp` | full | `install-bytes boots version-string apps-exact goto-edition onpoint-opens tools-open book-reads settings-kept update-check` |
| `x4pro-gpd` | full + DOGEAR | the `x4pro-gp` checks + the 12 DOGEAR checks |

The 12 DOGEAR checks, on both flagship DOGEAR builds:
- `dogear-opens`;
- the status screens, with `/dogear` renamed aside: `dogear-e1-no-issue-yet`
  (online against production while it has published nothing) and
  `dogear-e2-no-issue-on-device` (Wi-Fi off);
- a schema-2 regular issue (fixture R) as the only SD generation:
  `dogear-v2-cache-offline` and `dogear-v2-cache-503`;
- schema-3 quiet issues (fixtures Q1 and Q2), Wi-Fi off:
  `dogear-v3-q1-cover`, `-pages`, `-navigation` and the same three for Q2;
- `dogear-no-writes`.

**E3 (`CAN’T LOAD AN ISSUE`, a Wi-Fi network without internet). Physical E3 is
not a release gate. Host E3 coverage is mandatory.** It is not a physical check
on any package, and a record that lists it as one is refused. Instead the
candidate run's `e3-host-tests` job runs the exact host tests listed in
`acceptance-v1.json` under `hostCovered` (the state rules for a failed manifest,
every combination, the approved status copy, the TLS refusals, cut and
interrupted reads, bootstrap classification) on the run's own commit, and
`candidate.py e3-evidence` writes `e3-host-evidence.json` only if each one ran
and passed, none skipped. That file names the commit, the run and the digest of
the test definition; it is in `SHA256SUMS.txt`, and the acceptance record
carries it under `hostCovered`. A dev build's test run is never this evidence.

The fixtures are temporary, hash-verified SD generations
(`/dogear/issues/<id>.<sha16>.json` and `/dogear/active.txt`), generated from the
DOGEAR publisher commit that will be synced. Every card change is approved by the
owner first; a device's own `/dogear` is only renamed aside and restored, never
deleted.

**The record.** Fill in `ACCEPTANCE.json` (format: `acceptance_record` in
`scripts/release/test_candidate.py`): the candidate's repository, tag, commit,
run and `SHA256SUMS.txt` digest; the fixtures (generator commit, issue ids,
hashes, shape); one entry per package, in canonical order, with its env, file
and digest, the X4 Pro pieces' digests, the install method, the date and exactly
its required checks, each `PASS`; under `hostCovered`, the candidate's own
`e3-host-evidence.json` content, unchanged (its canonical bytes must be the file
in `SHA256SUMS.txt`); the owner's approval. No free text. Then:
```bash
python3 scripts/release/candidate.py acceptance --record ACCEPTANCE.json \
  --candidate-dir release-candidate --flash-support x4pro-flash-support
```
It refuses a missing, extra, repeated or reordered package; a missing, extra or
non-`PASS` check; any digest that is not the candidate's; and duplicate JSON
keys. It prints `acceptance-sha256`, the sha256 of the record's canonical JSON
(sorted keys, no spaces), which the tag carries. The owner approves that exact
record. The full record later becomes public in DOGEAR's firmware-contract sync
record; it is not attached to the release.

## 3. Tag, draft, publish

1. Only after every package passes and the owner has approved the record, create an
   **annotated** tag on the candidate commit and push it. Each field appears exactly
   once, and no other `field:` line may appear:
   ```
   v1.2.0

   candidate-run: <run-id>
   candidate-commit: <candidate sha>
   candidate-sums-sha256: <sha256 of SHA256SUMS.txt>
   physically-accepted: x4-g x4-gd x4pro-g x4pro-gd x4pro-p x4pro-pd x4pro-gp x4pro-gpd
   acceptance-sha256: <from candidate.py acceptance>
   ```
2. **Publish Release Draft** (`release.yml`) checks, before creating anything:
   - the tag equals `v<apps_version>`, is annotated, and points directly at the commit, which is its `candidate-commit`;
   - the run is this repository's `release_candidate.yml` (by workflow id, and by path
     with any `@ref` suffix removed);
   - the run was dispatched on `release/v<apps_version>*` at exactly the tagged commit, and succeeded;
   - every package was physically accepted, in canonical order;
   - the run's artifact matches the accepted digest.

   It then creates a **draft** from those files, never marked latest, and checks the
   draft's assets against GitHub's own sha256 digests.
3. **Publish only with `candidate.py publish`.** Never publish or mark a release latest any other way: not on the website, and not with `gh release edit`.

   Run it first without `--publish`. That is a dry run: it performs the whole preflight and changes nothing. Then run it again with `--publish`:
   ```bash
   python3 scripts/release/candidate.py publish --repository <owner>/<repo> --tag v1.2.0 \
     --commit <candidate sha> --run-id <run-id> --candidate release-candidate \
     --sums-sha256 <accepted sha256 of SHA256SUMS.txt> --acceptance ACCEPTANCE.json [--publish]
   ```
   In order, and failing closed before any state change:
   0. The local `release-candidate` folder is the accepted one: its files match its `SHA256SUMS.txt`, whose sha256 is the accepted digest, and its provenance names the commit and run. The acceptance record validates against it (as `candidate.py acceptance` does).
   1. `GET /repos/<owner>/<repo>/immutable-releases` reports `enabled: true`.
   2. Exactly one release carries the tag. It is still a **draft** (an already published release is refused, never republished) and not a prerelease. The tag is annotated, points directly at the commit, and its message names the same run, commit and digest, every package as physically accepted, and exactly this acceptance record's `acceptance-sha256`.
   3. The run is this repository's `release_candidate.yml`, dispatched on `release/<tag>*`, successful, at the commit. The draft's assets are exactly the accepted files (`e3-host-evidence.json` among them) plus `SHA256SUMS.txt`, with GitHub digests equal to the accepted ones.
   4. Only then, one `PATCH` sets `draft: false` and `make_latest: true`.
   5. It re-fetches and requires all of these:
      - published, not a prerelease, `immutable: true`;
      - the same assets and digests;
      - `/releases/latest` is this release;
      - the tag is unchanged.

   Exit status:
   - **0:** one of two things. Without `--publish`, the preflight passed and nothing was requested or changed; the release is still a draft. With `--publish`, the release was published, is latest and immutable, and verified.
   - **1:** a preflight failure. Nothing was requested and nothing changed.
   - **3:** the publication command could not execute at all, so nothing was sent and nothing changed. This is the only "not published" outcome, and it needs proof of one of:
     - `gh` is not on `PATH`;
     - `gh` is not executable;
     - starting it failed to exec exactly that file (`ENOENT`/`EACCES` naming it).
   - **2, "MAY BE PUBLISHED":** the request may have reached GitHub and the outcome is not verified. This covers any of:
     - any other failure to start `gh` (pipes, fork, child setup, an interruption), since it does not prove `gh` never ran;
     - once `gh` runs, any exception (communication, pipe, decoding, timeout, interruption);
     - any non-zero exit, whatever HTTP status or message `gh` reports (a 4xx included, since its error text cannot prove the request was not applied);
     - any failure after a successful request.

     Check the release at once; it may be public.

**Release asset integrity.** `verify-release`, and `publish` around its single state change, fail unless every one of these holds:
- the release is that tag, and in the expected draft or published state, and not a prerelease;
- its assets are exactly the accepted files plus `SHA256SUMS.txt`, with nothing missing, nothing extra and no duplicates;
- every asset is `uploaded`;
- GitHub's `digest` for each asset is `sha256:` plus the accepted digest;
- the release is marked `immutable` (always checked after publishing, or when `--require-immutable` is passed).

Two layers protect the published release:
- **Immutability:** the repository setting *Release immutability* is a precondition of `publish`. Once published, the release's assets and tag cannot be changed.
- **Re-verify regardless:** anything that later relies on a release re-fetches these digests with `verify-release --state published --require-immutable` and fails closed on any difference. That covers anything that verifies devices or syncs a firmware contract.

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
4. On the X4 Pro, check the OTA slot against the published app without writing:
   `esptool.py --after hard_reset verify_flash <slot offset> firmware-x4pro-<package>.bin` (a bulk
   `read_flash` of a whole slot is unreliable over this USB link). Never leave the chip in
   download mode: it looks frozen until it is reset.
