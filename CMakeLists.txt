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

project(Gatecrasher VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------------------------
# neon-foundry-core: shared BEHAVIOUR across the suite, pinned by tag exactly like JUCE above.
#
# **This is not a dependency on a sibling casting.** Depending on one of those is still forbidden -
# it couples two things that must ship on their own schedules. This is a versioned library that no
# casting owns, so a change to it forces nothing on anyone until they move their own pin, which is
# the same relationship every casting already has with JUCE. The root CLAUDE.md draws that
# distinction under "Structure".
#
# **Declared AFTER JUCE, and that ordering is load-bearing.** Core links juce::juce_core and does
# not fetch its own when consumed; it fails with an explicit message if JUCE is not already
# available. Two JUCE trees in one build link two juce_core builds into one binary and surface as
# duplicate symbols a long way from the cause.
#
FetchContent_Declare(NeonFoundryCore
    GIT_REPOSITORY https://github.com/trudslev/neon-foundry-core.git
    GIT_TAG v1.0.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(NeonFoundryCore)


# Runtime GUI assets only.
#
# Rev 6 inverted the architecture: gatecrasher-panel-plate@2x.png is a FULLY PRINTED plate carrying
# every label, numeral, unit and tick, and it is the build's background. The old bare chassis
# (-bare@2x) is kept in design/assets/ for mocking layouts but is no longer a build input, and the
# code that drew those labels is gone - see spec section 0.
#
# The dressed renders (gatecrasher-panel@Nx, -gate-open@Nx), the knob render comparison and the
# SHAPE-switch scope references are pixel-matching acceptance targets for design/, not runtime
# assets, so they stay out of BinaryData.
#
# **This paragraph said the wordmark was baked into the plate and that TudorVictors "ships in
# design/ only so the wordmark can be re-baked".** Both stopped being true: the panel rewrite
# deleted the plate, so the wordmark became a draw call and the face became a runtime input - and
# export 11 withdrew the face entirely on licensing. §9's wordmark is artwork now, embedded below.
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
        design/assets/fonts/BarlowCondensed-Bold.ttf
        design/assets/fonts/BarlowCondensed-SemiBold.ttf
        design/assets/fonts/BarlowCondensed-Regular.ttf
        design/assets/fonts/ShareTechMono-Regular.ttf
        # The wordmark face, embedded as of 2026-08-19. shared/FONTS.md row 16: licensed,
        # embeddable, ships - and RECUT.md says distributable. It was absent because the wordmark
        # was baked into the plate, which §0 has now deleted; the theme's old note gave the LICENCE
        # as the reason and that half was simply wrong, which is the half that would have stopped
        # the next person embedding it.
        # §9's wordmark, as ARTWORK. `TudorVictors.ttf` is © Chequered Ink 2020, All Rights
        # Reserved by its own `name` table; no licence was bought, and the licences on offer grant
        # use of the face to make things rather than the right to redistribute the file. Withdrawn
        # from the bundle at export 11 and from this list with it.
        #
        # 699 x 120 raster drawn at 233 x 40, transparent ground. TapeRot's treatment arriving at
        # the second casting — and Chorus-60's Librestile was checked on its own evidence at the
        # same time and DIFFERS (SIL OFL), so it ships unchanged. A conclusion about one wordmark
        # face transfers to another only by accident.
        design/assets/gatecrasher-wordmark.png
        # §8's 500 weight, delivered export 12 — the same cut TapeRot has, 680 glyphs,
        # usWeightClass 500, v1.408, OFL. Four §8 rows asked for it and drew SemiBold until now:
        # unit, scale numeral, the scope legend's scale values and the scope header data.
        # Numerals sit one step below the labels they qualify, and that step is the hierarchy.
        design/fonts/BarlowCondensed-Medium.ttf)

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
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/design/icons/gatecrasher-icon-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/design/icons/gatecrasher-icon-256.png"
    BUNDLE_ID "com.neonfoundry.gatecrasher"
    # Without this the Standalone cannot get audio INPUT on macOS, and fails silently doing it.
    # An app with no NSMicrophoneUsageDescription is never granted capture access - the OS does not
    # prompt and does not error, the input stream simply delivers zeros. On an effect that reads as
    # dead metering: IN sits at -INF and OUT follows, because there is nothing to process. The
    # meters are wired correctly; they have nothing to show.
    #
    # It bites this casting twice over: the sidechain KEY SOURCE has nothing to listen to either, so
    # a gate fed from an external key reads as permanently closed. Plugin formats are unaffected -
    # the host owns the input there.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Gatecrasher needs audio input so the Standalone can process what you play into it."
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
    Source/GUI/KnobComponent.cpp
    Source/GUI/GateScope.cpp
    Source/GUI/GateLamp.cpp
    Source/GUI/InputMeter.cpp
    Source/GUI/ToggleSwitchComponent.cpp
    Source/GUI/ProgramHeader.cpp
    Source/GUI/GatecrasherEditorContent.cpp)

target_compile_definitions(Gatecrasher PUBLIC
    # §9's version stamp is drawn now rather than baked, so the panel needs the number. Derived
    # from PROJECT_VERSION rather than typed as artwork - three castings bake theirs into a plate
    # and have to be re-cut when it moves; this one does not, and should not acquire the problem.
    NF_VERSION_SHORT="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
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
        # ProgramManager.cpp is compiled into both this target and the tests, and calls into core,
        # so both link it.
        nf::core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

add_subdirectory(Tests)
