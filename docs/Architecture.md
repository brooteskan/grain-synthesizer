# Architecture and contracts

`include/audio` and `src/grain_cloud.cpp` retain wozzits' device-independent DSP.
`RealtimeCloud` adds a fixed 128-slot SPSC command ring (127 usable slots), bounded
parameter validation and callback-owned synthesis. The callback snapshots the
producer position, consumes at most that snapshot and clears/renders float PCM.
It performs no heap allocations, locks, asset operations or EBus calls. Commands
take effect at callback boundaries; ramps advance per sample. This adapter does
not import the wozzits timestamped scheduler.

Configure and destroy only while no callback can run. There must be exactly one
producer and one consumer. Stop-queue failure is reported; the caller may retry.
Stopping emits a natural grain tail; it does not free source storage. A stopped
O3DE emitter stays attached and retains decoded PCM for reuse until deactivation.

The MiniAudio custom data source renders at the engine's sample rate, stereo for
ambience or mono before spatialization. `ma_sound_uninit` detaches the source
before source uninitialization and PCM destruction. It shares the existing engine
and device. Do not unload MiniAudio while emitters remain active.

O3DE source assets load asynchronously. When all are ready, the main thread
decodes their encoded data into owned mono float PCM, up to 256 MiB per emitter.
Decoding is outside the audio callback but can pause a game frame; prewarming
emitters or adding background decoding is appropriate before production use.
Assets, descriptors and callback state are not hot-reloaded during playback.
Configure/asset/engine failures log an error and leave the emitter silent.

Emitters use asset-ready/error notifications for startup and transform-change
events for spatial movement. Already-ready sources start immediately. Stops and
deactivation cancel pending starts and disconnect asset subscriptions; late
notifications cannot restart playback. Initial spatial position is set before
starting the sound, then updated only when the entity transform changes. There
is no frame-bus subscription or polling fallback. Normal asset notifications and
control requests are consumed on the O3DE main thread, keeping one producer for
the DSP command ring. With Auto Play disabled, the emitter requests loading and
playback only on an explicit Play (other engine systems can preload the assets).

The editor component only authors configuration and builds the runtime component;
it does not open sound sources while editing. A listener is supplied by MiniAudio.
The Gem needs no engine source patches and no hard-coded checkout paths.

The upstream Gaussian window and linear interpolator are preserved for sound
compatibility. Rendering does not limit the final mix; source and emitter gain
must leave headroom. Overlap normalization manages overlap energy, not peaks.
The standalone tests preserve upstream behavior and verify queues, ramps,
concurrency, mono/stereo output and device-less MiniAudio graph playback/detach.

Follow-up work: reusable preset assets, atomic preset-change commands, background
decode/cache, hot reload, clip streaming, occlusion/reverb, editor audition,
profiling and voice virtualization. Dedicated-server audio is not a goal of this
initial version.
