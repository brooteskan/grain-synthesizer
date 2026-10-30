# Validation

Validated on Windows x64 with MSVC 19.51 / Visual Studio 2026 and CMake 4.3.1.

- 27 standalone tests passed in Release: 18 original wozzits tests, seven realtime
  adapter tests, one MiniAudio graph test and a notification-only architecture
  check that rejects frame-bus polling in the O3DE component sources.
- `GrainSynthesizer` and `GrainSynthesizer.Editor` compiled and linked in profile
  against the local O3DE 2.7.0 TG SDK, using a separate minimal consumer project.
- The notification-based runtime and editor modules also compiled and linked
  against TG's source-engine build of O3DE 2.7.0. All nine `GrainSynthesizer.Tests`
  tests passed: ready and delayed assets, synchronous connection notifications,
  duplicate references, explicit Play, Stop during loading, invalid/error assets,
  deactivation, initial spatial placement and transform-driven spatial changes.
  These tests use a device-free MiniAudio engine and perform real decoding and
  rendering without dispatching simulation frames.
- The five extracted upstream source/test files were verified byte-for-byte
  against the source checkout at the revision in Provenance.md.

The GitHub workflow runs standalone tests on Windows and Linux. Hosted CI does
not build O3DE; the integration tests run locally against TG. Physical-device
playback and subjective listening are outside the automated test coverage.
