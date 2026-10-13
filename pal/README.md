# PAL/DE source profile (draft)

This profile imports the EU prototype's region-specific hooks and runtime fixes as an opt-in source tree. The normal NTSC-U sources and generation commands remain available. The shared Switch heap correction is also proposed directly in `src/switch_heap.c`.

The supported EU executable is 4,038,656 bytes, SHA-256 `34e2a0dbc722302d12b05b12fbaf8d7c3ddd024183f3e33b96fff09156b29604`, entry `0x0021B77E`. It needs its own translated executable; selecting English or German does not make a US translation compatible with an EU disc.

Included PAL changes cover movie/audio integration, D3D/input callbacks, frame fences, native matrix helpers, video-option rows. The game retains its normal language chooser, without an automatic selection or language override. The source profile retains the PAL prototype's initialization errors, memory reserve, renderer caches and synchronization checks.

## Preparing sources

```text
python tools/prepare_eu_source.py --xbe /path/to/EU/default.xbe --gen-dir /path/to/verified-EU-gen --output build-eu/source
```

`--check` validates the profile without keeping an additional source tree. Inputs are read only; the output directory must be empty. The patch is bound to its recorded upstream commit and source hashes, so an upstream change is never silently overwritten. No game data, generated game code, binaries, user saves or hardware logs are included.

Build the resulting tree with the existing Switch SDK and CMake toolchain, setting `NFSU2_GEN_DIR` to the verified EU translation and `NFSU2_VULKAN=ON` for Vulkan. `pal/nfsu2x_env.txt` supplies the PAL prototype's configuration; title takeover and shared-memory mode are required.

EU regeneration from a fresh XBE and consolidation of the frozen PAL renderer with the latest upstream Vulkan implementation remain draft work. This profile is not a claim that the new upstream integration has completed a hardware playthrough.

## Validation

The PAL prototypes reached boot, movies, menus, Quick Race and Career driving with audio on a Switch. Full playthrough and remaining graphics/stability issues are unconfirmed. The imported v0.21 source includes optimizations whose hardware FPS gain is still pending verification.

`python tests/switch_heap/run.py --cc clang` executes the real heap bootstrap against mocked loader/kernel services, including loader prefixes, applet budgets, resize failures and loader-return restoration. `prepare_eu_source.py --check` verifies the imported source and, when supplied, the exact existing translation hashes.
