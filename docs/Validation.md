# Initial validation

Validated on Windows x64 with MSVC 19.51 / Visual Studio 2026 and CMake 4.3.1.

- 26 standalone tests passed in Release: 18 original wozzits tests, seven realtime
  adapter tests and one MiniAudio graph test covering both spatial and stereo
  configurations plus source detachment.
- `GrainSynthesizer` and `GrainSynthesizer.Editor` compiled and linked in profile
  against the local O3DE 2.7.0 TG SDK, using a separate minimal consumer project.
- The five extracted upstream source/test files were verified byte-for-byte
  against the source checkout at the revision in Provenance.md.

The GitHub workflow runs standalone tests on Windows and Linux. Hosted CI does
not build O3DE; the O3DE build above is a local compatibility check. Editor
interaction, physical-device playback, subjective listening and TG integration
have not been verified. No claim of production readiness is implied by a build.
