# PAL-EU support in the shared port

US and EU builds use the same source tree, Xbox runtime, renderers, audio decoder, Switch platform layer and heap bootstrap. Region-specific hooks live under `src/regions/eu/`; selecting EU does not copy or patch a separate runtime tree.

## Supported discs

The EU build requires the tested European `default.xbe`: 4,038,656 bytes, SHA-256 `34e2a0dbc722302d12b05b12fbaf8d7c3ddd024183f3e33b96fff09156b29604`, entry `0x0021B77E`. The existing US translation uses entry `0x0021B1CE`.

Each NRO contains one region's translation. EU and US NROs require their matching game data. Startup rejects a mismatch before guest memory mapping. Both need extracted disc files (`default.xbe`, `NFSUNDER/`, etc.); neither opens ISO files directly. Other executable revisions are unverified.

The normal game language chooser is retained. There is no forced German selection or language override.

## Generation and build

The portable command uses the shared upstream toolkit:

```text
python tools/regen.py --xbe /path/to/EU/default.xbe --region EU --gen-dir /path/to/gen-eu
cmake -S . -B build-eu -DNFSU2_REGION=EU -DNFSU2_GEN_DIR=/path/to/gen-eu -DNFSU2_VULKAN=ON
```

Supply the normal Switch toolchain/SDK arguments for Switch. For US, use `--region US`, `-DNFSU2_REGION=US` and a separate generated directory. `--region auto` identifies the executable during generation; the resulting NRO still contains one translation. Existing complete analysis can be supplied with `--analysis-dir`. Output must be empty; existing translations and inputs are not overwritten.

Existing verified EU generated files can also be supplied with `NFSU2_GEN_DIR`. A pass-through ABI bridge is selected only when an older translation renamed the original chooser. It invokes that original chooser without selecting a language.

## Maintenance

Renderer, audio, platform and runtime fixes are maintained once and compiled for both builds. New game-specific hooks may require an EU address/ABI mapping. Callback seeds and bounded CPU/I/O models are explicit; no global address offset is assumed.

The Vulkan unified-memory fallback treats locality/caching as preferences and retains host visibility/coherence requirements. The optional `NVK_LOCALIZED_DISPATCH` bridge supports SDK archives whose direct Vulkan symbols are localized. It is shared by both builds and defaults off.

## Validation

The full analysis/generation pipeline runs natively on Windows. A freshly generated EU translation produces an ELF and NRO with the shared current upstream renderer/runtime; reuse of the previous EU translation is also supported. This integration still needs a real Switch test. Previous PAL prototype observations do not establish stability or performance of this new build. Full playthrough and US hardware regression testing are pending.

`python tests/regions/run.py --cc clang --eu-xbe /path/to/EU/default.xbe` checks both build modes, detection and modified-input rejection. `python tests/switch_heap/run.py --cc clang` exercises the actual heap bootstrap. `python tests/vulkan_memory/run.py --cc clang` checks the renderer's allocation policy.

The shared native matrix and movie-row helpers are exercised by `tests/native_matrix/run.py` and `tests/movie_row/run.py`, including overlap, signed saturation/wrap and guest register postconditions.
