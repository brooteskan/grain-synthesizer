# Wozzits extraction

Source repository: https://github.com/brooteskan/wozzits.
Source commit: `91a345aad96f1af5beee67262263bb81f5c1e3b4`.

The following files were copied without DSP changes:

| Upstream | This repository |
|---|---|
| window_engine/audio/audio_buffer.h | include/audio/audio_buffer.h |
| window_engine/audio/grain_cloud.h | include/audio/grain_cloud.h |
| window_engine/audio/grain_window.h | include/audio/grain_window.h |
| src/audio/grain_cloud.cpp | src/grain_cloud.cpp |
| tests/audio/grain_cloud_tests.cpp | tests/upstream/grain_cloud_tests.cpp |

The realtime control queue, MiniAudio bridge and O3DE components are new adapter
code. The wozzits mixer, scheduler, device layer, asset system, behavior ABI and
recordings are not copied. Keep upstream file paths and the source revision here
when importing future changes; compare the same seeded fixtures before changing
the DSP. The original MIT license is retained verbatim.
