cmake_minimum_required(VERSION 3.24)
# LANGUAGES needs C as well as CXX: JUCE/VST3-SDK/other fetched third-party deps bundle some .c
# sources, and Xcode/Visual Studio tolerate that without an explicit enable_language(C), but
# Linux's single-config Makefiles/Ninja generator does not - it fails at CMake's generate step
# with "CMAKE_C_COMPILE_OBJECT not set" if C was never enabled at the top level.
# These MUST come before project(): project() creates these cache entries itself, after which a
# plain set(... CACHE ...) is a silent no-op. Sitting below project() they had never taken effect -
# builds inherited the host architecture and the default SDK deployment target instead, which only
# surfaced when a CI-published .pkg turned out to contain an arm64-only binary.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal: Apple Silicon + Intel")
endif()

project(Gatecrasher VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Runtime GUI assets only - the dressed panel renders, the knob render comparison, and the
# SHAPE-switch scope references (scope-hard/soft-release) are pixel-matching acceptance targets for
# design/GATECRASHER-GUI-SPEC.md, not loaded at runtime, so they're deliberately excluded.
#
# gatecrasher-wordmark@3x.png is the spray-stencil wordmark baked to a transparent PNG, per the
# spec's section 8 ("Do not render the sprayed lettering in code. Bake it."). TudorVictors.ttf is
# deliberately NOT embedded and is NOT tracked in this repository: its licence (Chequered Ink)
# permits non-profit use but grants no redistribution right, and allows embedding only in a form
# that "protects the Product from extraction", which a BinaryData blob is not. See
# WordmarkComponent's class comment.
#
# Barlow Condensed SemiBold (600, labels) / Bold (700, lamp text) and Share Tech Mono Regular
# (numeric/LED readouts) per the spec's typography section - only the specific weights actually used
# are embedded, not the full 18-file Barlow Condensed family design/assets/ ships.
juce_add_binary_data(GatecrasherBinaryData
    SOURCES
        design/assets/gatecrasher-panel-bare@2x.png
        design/assets/gatecrasher-wordmark@3x.png
        design/assets/knob_large_128px_128f.png
        design/assets/knob_small_128px_128f.png
        design/assets/BarlowCondensed-SemiBold.ttf
        design/assets/BarlowCondensed-Bold.ttf
        design/assets/ShareTechMono-Regular.ttf)

# AU is Apple-only; Windows/Linux only ever build VST3 + Standalone. VST3_COPY_DIR/AU_COPY_DIR are
# only set on Apple - leaving them unset elsewhere lets JUCE apply its own correct per-OS default
# rather than a macOS path built on a non-guaranteed $ENV{HOME}.
if(APPLE)
    set(GATECRASHER_FORMATS AU VST3 Standalone)
    set(GATECRASHER_EXTRA_ARGS
        VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        AU_COPY_DIR   "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
else()
    set(GATECRASHER_FORMATS VST3 Standalone)
    set(GATECRASHER_EXTRA_ARGS "")
endif()

juce_add_plugin(Gatecrasher
    PRODUCT_NAME "Gatecrasher"
    COMPANY_NAME "Neon Foundry"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/design/assets/icon/gatecrasher-icon-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/design/assets/icon/gatecrasher-icon-256.png"
    BUNDLE_ID "com.neonfoundry.gatecrasher"
    PLUGIN_MANUFACTURER_CODE Nfdy
    PLUGIN_CODE Gr85
    FORMATS ${GATECRASHER_FORMATS}
    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    ${GATECRASHER_EXTRA_ARGS})

target_sources(Gatecrasher PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/TriggerDetector.cpp
    Source/DSP/GateEnvelopeGenerator.cpp
    Source/DSP/RoomTank.cpp
    Source/DSP/PlateTank.cpp
    Source/DSP/ChamberTank.cpp
    Source/DSP/AmbienceTank.cpp
    Source/DSP/ReverbEngine.cpp
    Source/DSP/DampingStage.cpp
    Source/DSP/SlamSaturation.cpp
    Source/DSP/StereoWidthStage.cpp
    Source/DSP/OutputMixStage.cpp
    Source/DSP/ProgramManager.cpp
    Source/GUI/GatecrasherLookAndFeel.cpp
    Source/GUI/GatecrasherPanelBackground.cpp
    Source/GUI/KnobFilmstripComponent.cpp
    Source/GUI/GateScope.cpp
    Source/GUI/GateLamp.cpp
    Source/GUI/InputMeter.cpp
    Source/GUI/ToggleSwitchComponent.cpp
    Source/GUI/ProgramHeader.cpp
    Source/GUI/WordmarkComponent.cpp
    Source/GUI/PanelChrome.cpp
    Source/GUI/PanelReadouts.cpp
    Source/GUI/GatecrasherEditorContent.cpp)

target_compile_definitions(Gatecrasher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Gatecrasher
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        GatecrasherBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

add_subdirectory(Tests)
