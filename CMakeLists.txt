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

# Runtime GUI assets only.
#
# Rev 6 inverted the architecture: gatecrasher-panel-plate@2x.png is a FULLY PRINTED plate carrying
# every label, numeral, unit and tick, and it is the build's background. The old bare chassis
# (-bare@2x) is kept in design/assets/ for mocking layouts but is no longer a build input, and the
# code that drew those labels is gone - see spec section 0.
#
# The dressed renders (gatecrasher-panel@Nx, -gate-open@Nx), the knob render comparison and the
# SHAPE-switch scope references are pixel-matching acceptance targets for design/, not runtime
# assets, so they stay out of BinaryData. gatecrasher-wordmark@3x.png is gone with
# WordmarkComponent - the wordmark is baked into the plate now. TudorVictors.ttf ships in design/
# only so the wordmark can be re-baked; it is not a runtime face and is not embedded.
#
# Fonts: the faces spec section 2.3 draws at RUNTIME. All THREE Barlow weights are needed, which is
# one more than design/assets/fonts/README.md implies - it says SemiBold (600) is wanted only for a
# group caption, but section 6.4 puts the SAVE/DELETE button labels at 600 as well, and those are
# drawn live. So:
#
#   Bold 700     - selected state-dependent labels (section 0.4)
#   Regular 400  - unselected state-dependent labels
#   SemiBold 600 - SAVE / DELETE / STORE / CANCEL button labels (section 6.4)
#
# Share Tech Mono Regular renders the LCD name, live values and IN/OUT. Both families are OFL 1.1,
# which permits embedding; design/assets/fonts/OFL.txt carries the notice the licence requires to
# travel with them.
juce_add_binary_data(GatecrasherBinaryData
    SOURCES
        design/assets/gatecrasher-panel-plate@2x.png
        design/assets/knob_large_160px_128f.png
        design/assets/knob_small_160px_128f.png
        design/assets/fonts/BarlowCondensed-Bold.ttf
        design/assets/fonts/BarlowCondensed-SemiBold.ttf
        design/assets/fonts/BarlowCondensed-Regular.ttf
        design/assets/fonts/ShareTechMono-Regular.ttf)

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

# Single source of truth for the two identity strings that appear BOTH in the bundle metadata and
# in ProgramManager's user-Programs directory path. ProgramManager.cpp can't read JUCE's
# JucePlugin_* macros - they only exist in the plugin target's generated header, and that file is
# also compiled into the Tests console app - so it used to carry a hand-synced copy marked "keep in
# sync". It didn't stay in sync: COMPANY_NAME became "Neon Foundry" while the copy still said
# "Tanis", quietly pointing saved Programs at the old directory. CMake now hands both targets the
# same values, so drift is a build error instead of silent data loss.
set(GATECRASHER_COMPANY_NAME "Neon Foundry")
set(GATECRASHER_PRODUCT_NAME "Gatecrasher")

juce_add_plugin(Gatecrasher
    PRODUCT_NAME "${GATECRASHER_PRODUCT_NAME}"
    COMPANY_NAME "${GATECRASHER_COMPANY_NAME}"
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
    Source/GUI/StateLabels.cpp
    Source/GUI/ProgramHeader.cpp
    Source/GUI/GatecrasherEditorContent.cpp)

target_compile_definitions(Gatecrasher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    NF_COMPANY_NAME="${GATECRASHER_COMPANY_NAME}"
    NF_PRODUCT_NAME="${GATECRASHER_PRODUCT_NAME}")

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
