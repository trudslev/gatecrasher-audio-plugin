# Building Gatecrasher

Gatecrasher builds on macOS (AU + VST3 + Standalone), Windows (VST3 + Standalone), and Linux
(VST3 + Standalone) — AU is Apple-only. JUCE 8.0.14 is fetched automatically via CMake
`FetchContent` on first configure (no local JUCE checkout needed) on any platform.

## macOS

### Requirements

- Xcode (full install, not just Command Line Tools) — `xcodebuild -version` must succeed.
- CMake 3.24+.
- [pluginval](https://github.com/Tracktion/pluginval) for VST3 validation: `brew install --cask pluginval`.

### Build

```sh
cmake -B build -G Xcode
cmake --build build --config Release
```

This builds AU, VST3, and a Standalone app, and installs the AU/VST3 bundles to:

```
~/Library/Audio/Plug-Ins/Components/Gatecrasher.component
~/Library/Audio/Plug-Ins/VST3/Gatecrasher.vst3
```

### Validate

```sh
auval -a | grep -i gatecrasher               # confirm AU registration + 4-char codes
auval -v aufx Gr85 Nfdy                      # full AU validation

/Applications/pluginval.app/Contents/MacOS/pluginval \
    --strictness-level 8 \
    --validate ~/Library/Audio/Plug-Ins/VST3/Gatecrasher.vst3
```

Since Gatecrasher declares an optional stereo sidechain input bus (for Trigger Source = Sidechain),
pluginval and auval should be run with a host/test harness that can exercise both the with- and
without-sidechain bus layouts — the default validation above only covers the no-sidechain case.

If Logic Pro doesn't pick up a freshly built AU: Preferences → Audio Units Manager → "Reset & Rescan Selection", or restart Logic.

### Run the unit/DSP tests

```sh
./build/Tests/GatecrasherTests_artefacts/Release/GatecrasherTests
```

## Windows

### Requirements

- Visual Studio 2022 or later with the "Desktop development with C++" workload.
- CMake 3.24+ (bundled with Visual Studio, or install separately).
- [pluginval](https://github.com/Tracktion/pluginval) for VST3 validation (Windows build available from the same releases page) — no `auval` equivalent, since AU doesn't exist on Windows.

### Build

```bat
cmake -B build -A x64
cmake --build build --config Release
```

This builds VST3 and a Standalone app. VST3 install location is JUCE's own platform default
(`%COMMONPROGRAMFILES%\VST3\Gatecrasher.vst3`, i.e. usually `C:\Program Files\Common Files\VST3\`) —
Gatecrasher doesn't override `VST3_COPY_DIR` on Windows.

### Validate

```bat
pluginval.exe --strictness-level 8 --validate "%COMMONPROGRAMFILES%\VST3\Gatecrasher.vst3"
```

### Run the unit/DSP tests

```bat
build\Tests\GatecrasherTests_artefacts\Release\GatecrasherTests.exe
```

## Linux

### Requirements

- A C++20-capable compiler (GCC or Clang) and CMake 3.24+.
- JUCE's standard Linux build dependencies:
  ```sh
  sudo apt-get install -y \
      libasound2-dev libjack-jackd2-dev \
      libcurl4-openssl-dev \
      libfreetype6-dev libfontconfig1-dev \
      libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
      libwebkit2gtk-4.1-dev \
      libglu1-mesa-dev mesa-common-dev
  ```
  (package names above are for Debian/Ubuntu — adjust for other distros).
- [pluginval](https://github.com/Tracktion/pluginval) for VST3 validation (Linux build available from the same releases page) — no `auval` equivalent, since AU doesn't exist on Linux.

### Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Unlike the Xcode/Visual Studio generators used on macOS/Windows, CMake's default Linux generators
(Makefiles/Ninja) are single-config, so `CMAKE_BUILD_TYPE=Release` must be set at configure time.

This builds VST3 and a Standalone app. VST3 install location is JUCE's own platform default
(`~/.vst3/Gatecrasher.vst3`) — Gatecrasher doesn't override `VST3_COPY_DIR` on Linux.

### Validate

```sh
./pluginval --strictness-level 8 --validate ~/.vst3/Gatecrasher.vst3
```

### Run the unit/DSP tests

```sh
./build/Tests/GatecrasherTests_artefacts/Release/GatecrasherTests
```

## What the DSP test suite covers

Trigger-detector filtering and sidechain-fallback behavior; the gate envelope's sample-accurate
open transition, Hold retriggering, Hard vs. Soft release-curve timing, and [0,1]-bounded output
under random input; each reverb algorithm's stability (silence-in/silence-out, and finite/bounded
output from an impulse at maximum decay/density/size) plus `ReverbEngine`'s algorithm-switch
crossfade; damping's HF/LF attenuation; Slam's bounded output across its range; stereo width's
0%/100%/200% behavior; output mix/trim's dry/wet blend and gain math; APVTS parameter defaults and
session round-tripping; factory-program structural sanity (valid choice indices, in-range fields,
unique names); and a full-chain CPU check at 48kHz/64-sample buffers against the real-time budget.

Tonal/character correctness of the reverb algorithms and the 17 factory programs' values is
explicitly **not** covered here — see the "DSP tuning" note below.

## Notes

- `PLUGIN_MANUFACTURER_CODE` (`Nfdy`, shared across the suite), `PLUGIN_CODE` (`Gr85`), `BUNDLE_ID`
  (`com.neonfoundry.gatecrasher`), and `COMPANY_NAME` (`Neon Foundry`) —
  finalize these before any real release, since they're effectively permanent once shipped or
  automated against.
- JUCE's free/personal tier splash screen is enabled (no paid license configured).
- **DSP tuning**: every DSP stage has real, functioning processing (no stubs), but the reverb
  tanks' comb/allpass delay-length tables and feedback ranges, and the 17 factory programs'
  parameter values, are a structurally-plausible first pass, not yet tuned by ear against the real
  algorithms — the same status TapeRot's own factory presets had before their by-ear pass (see
  `FactoryProgramsTests.cpp` and `CombAllpassNetwork.h`'s class comment). Build, load, listen,
  adjust.
- **GUI asset placeholders**: the wordmark is drawn from the embedded TudorVictors typeface rather
  than the pre-baked sprayed-stencil PNG `GATECRASHER-GUI-SPEC.md` section 8 calls for, and labels/
  numeric readouts use JUCE's default fonts rather than Barlow Condensed / Share Tech Mono — none of
  those assets exist in `design/assets/` yet. See `prompts/PROMPTS.md` for the follow-up.
- Gatecrasher declares an optional stereo sidechain input bus (`Trigger Source: Sidechain`) in
  addition to the main stereo in/out — `isBusesLayoutSupported` also accepts a disabled or mono
  sidechain.
