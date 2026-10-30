# grain-synthesizer

Wozzits granular synthesis as an independent C++ library and O3DE Gem. Generate
evolving wind, machinery, environmental beds and pitched textures from up to
eight recorded source clips. Gaussian grains, weighted source blending, seeded
jitter, sample-rate parameter ramps and overlap normalization retain the original
wozzits implementation.

Like terrain-compositor, this repository is independent of its consuming game;
the O3DE Gem lives in `integrations/o3de`. It uses O3DE's MiniAudio Gem for the
output device and positioning. It does not replace AudioSystem/ATL or use Wwise.

## Checkout and tests

```sh
git clone --recurse-submodules https://github.com/brooteskan/grain-synthesizer.git
cd grain-synthesizer
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Requires CMake 3.24 and a C++20 compiler for the upstream tests (core: C++17).
GoogleTest 1.15.2 and miniaudio 0.11.22 are pinned Git submodules, used only for
standalone testing. CMake does not fetch dependencies. For an existing checkout,
run `git submodule update --init --recursive`. O3DE builds use the engine's
MiniAudio dependency and do not compile the test submodules.

## Add to an O3DE project

```sh
git submodule add https://github.com/brooteskan/grain-synthesizer.git Gems/grain-synthesizer
```

Merge these entries into `project.json`:

```json
{
  "external_subdirectories": ["Gems/grain-synthesizer/integrations/o3de"],
  "gem_names": ["GrainSynthesizer"]
}
```

MiniAudio is a declared Gem dependency. Reconfigure and rebuild the project and
Editor. Add a **MiniAudio Listener** to the camera and an **Audio / Grain Emitter**
component to an entity. Assign one to eight MiniAudio sound assets imported from
WAV, FLAC or MP3. Enter game mode to hear the emitter. Editor configuration is
supported; audition buttons outside game mode are not implemented yet.

Disable **Spatial** for stereo ambience. Enable it for a mono point emitter and
set Pan Spread to zero; MiniAudio then supplies world positioning and distance
attenuation. All source clips are decoded to mono, matching the wozzits grain
model. The grain decoder does not yet support OGG, even though ordinary MiniAudio
playback does.

Use `GrainEmitterRequestBus` from C++ or Lua for `Play`, `Stop`, and
`SetParameter(parameter, value, rampSeconds, sourceIndex)`. Calls must come from
the main thread. `false` reports a rejected parameter, absent runtime or full
command queue. `Play` can accept a deferred start while assets load; failures
are logged. `Stop` stops new grains and lets existing grains finish.

| Parameter | Ordinal | Units |
|---|---:|---|
| Gain | 0 | Linear amplitude |
| Density | 1 | Grains/sec, 0..10000 |
| Position | 2 | 0..1 |
| Pitch | 3 | Multiplier, 0..16 |
| BlendRate | 4 | Hz |
| BlendDepth | 5 | 0..1 |
| SourceWeight | 6 | Relative weight; select clip with sourceIndex |
| PositionJitter | 7 | 0..1 |
| PitchJitter | 8 | Semitones, 0..48 |
| PanCenter | 9 | -1..1 |
| PanSpread | 10 | 0..1 |
| GrainMs | 11 | Milliseconds, 1..10000 |
| WindowParam | 12 | Gaussian sigma, 0.001..1 |
| Window | 13 | 0 = Gaussian; discrete |

Normalization, seed, source bank and capacity are authored at activation. Missing
weights default to 1. All-zero weights are silent. Current capacity is 1..256
grains per emitter; saturated pools drop new grains.

## Standalone use

Link `GrainSynthesizer::core`. Use `wz::audio::GrainCloud` directly for offline
rendering, or `GrainSynthesizer::RealtimeCloud` for single-producer control and
audio-thread rendering. Source PCM is borrowed and must outlive rendering. The
library opens no devices and has no O3DE dependency.

## Scope and provenance

This initial version provides the original grain DSP, realtime controls, an
editor-authored runtime emitter, asynchronous asset loading and a custom
MiniAudio data source. See [architecture and limits](docs/Architecture.md) and
[upstream provenance](docs/Provenance.md). Reusable program assets, atomic
whole-program morphs, background decoding, streaming, occlusion and reverb-zone
authoring are future work. No audio recordings are included.

O3DE emitters react to asset-ready/error and transform-change notifications;
they do not subscribe to a frame bus. In O3DE builds with tests enabled,
`GrainSynthesizer.Tests` exercises startup, loading cancellation, deactivation,
duplicate source references and spatial updates without dispatching simulation
frames. Standalone CI also enforces the notification-only component architecture.

See [validation results and limits](docs/Validation.md).

MIT licensed; original Wozzits notices are preserved in `licenses/wozzits-MIT.txt`.
