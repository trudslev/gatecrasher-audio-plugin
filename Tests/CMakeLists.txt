juce_add_console_app(GatecrasherTests PRODUCT_NAME "GatecrasherTests")

target_sources(GatecrasherTests PRIVATE
    RealtimeSafetyTests.cpp
    CpuBaselineTests.cpp
    NumericalRobustnessTests.cpp
    InvarianceTests.cpp
    AutomationLatencyTests.cpp
    ChannelLayoutTests.cpp
    PendingProgramLockTests.cpp
    MeterReadoutBudgetTests.cpp
    TestMain.cpp
    ReadoutConformanceTests.cpp
    EditorWiringTests.cpp
    PrintedScaleTests.cpp
    MenuMetricsTests.cpp
    TriggerDetectorTests.cpp
    GateEnvelopeGeneratorTests.cpp
    ReverbTankTests.cpp
    DampingStageTests.cpp
    SlamSaturationTests.cpp
    StereoWidthStageTests.cpp
    OutputMixStageTests.cpp
    ParametersStateTests.cpp
    ParameterTextTests.cpp
    ProgramManagerTests.cpp
    FactoryProgramsTests.cpp
    CPUCheckTests.cpp
    ../Source/DSP/TriggerDetector.cpp
    ../Source/DSP/GateEnvelopeGenerator.cpp
    ../Source/DSP/RoomTank.cpp
    ../Source/DSP/PlateTank.cpp
    ../Source/DSP/ChamberTank.cpp
    ../Source/DSP/AmbienceTank.cpp
    ../Source/DSP/ReverbEngine.cpp
    ../Source/DSP/DampingStage.cpp
    ../Source/DSP/SlamSaturation.cpp
    ../Source/DSP/StereoWidthStage.cpp
    ../Source/DSP/OutputMixStage.cpp
    ../Source/DSP/ProgramManager.cpp
    # **The REAL processor and the REAL editor, not stand-ins.** Everything above this line is what
    # core does not own at the DSP layer; everything below it is the wiring and the panel, which no
    # test could reach while this target compiled DSP alone.
    #
    # That gap was not theoretical. Reflect-84 shipped the stale-replay guard with ZERO call sites
    # for its disarm, and no suite could have seen it: the defect lived in an editor .cpp that was
    # not compiled into anything a test could run.
    ../Source/PluginProcessor.cpp
    ../Source/PluginEditor.cpp
    ../Source/GUI/GatecrasherEditorContent.cpp
    ../Source/GUI/ProgramHeader.cpp
    ../Source/GUI/GatecrasherPanelBackground.cpp
    ../Source/GUI/GatecrasherLookAndFeel.cpp
    ../Source/GUI/KnobComponent.cpp
    ../Source/GUI/ToggleSwitchComponent.cpp
    ../Source/GUI/GateScope.cpp
    ../Source/GUI/GateLamp.cpp
    ../Source/GUI/InputMeter.cpp
)

# NF_COMPANY_NAME/NF_PRODUCT_NAME come from the parent CMakeLists and must match what the plugin
# target gets - ProgramManager.cpp is compiled into both, and builds the user-Programs path from
# them. Inherited from the parent scope rather than restated here, so there is nothing to re-sync.
#
# The JucePlugin_* pair and NF_VERSION_SHORT arrived with the processor and panel sources: a console
# app gets none of the plugin target's generated definitions, and PluginProcessor uses
# JucePlugin_Name for getName() while PanelBackground prints the version stamp. Inherited from the
# same parent variables the plugin target uses, so there is nothing to re-sync — a hand-synced copy
# in CHORUS-60 once drifted to a dead company name and quietly pointed saved Programs at a directory
# nothing reads.
# **The core commit this casting was pinned at, resolved at CONFIGURE time and baked into the CPU
# test's provenance.** The tag moves while core is pre-release, so `v1.0.0` in a baseline file would
# record nothing — the whole value of the provenance block is that a figure can be attributed to one
# state of six castings and one state of core, which is what makes "one sitting, all six, one machine
# state" checkable after the fact rather than a promise.
execute_process(
    COMMAND git -C "${neonfoundrycore_SOURCE_DIR}" rev-parse --short HEAD
    OUTPUT_VARIABLE NF_CORE_PIN_COMMIT_VALUE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT NF_CORE_PIN_COMMIT_VALUE)
    set(NF_CORE_PIN_COMMIT_VALUE "unknown")
endif()

target_compile_definitions(GatecrasherTests PRIVATE
    NF_CORE_PIN_COMMIT="${NF_CORE_PIN_COMMIT_VALUE}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NF_COMPANY_NAME="${GATECRASHER_COMPANY_NAME}"
    NF_PRODUCT_NAME="${GATECRASHER_PRODUCT_NAME}"
    # The comment above named this and the list did not carry it, which cost a build the first time
    # the panel drew the version stamp: ProgramHeader.cpp is compiled into both targets, and a
    # console app gets none of the plugin target's definitions. Same shape as NF_COMPANY_NAME's own
    # history - the fix is CMake handing both targets the value, not a second copy anywhere.
    NF_VERSION_SHORT="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    JucePlugin_Name="${GATECRASHER_PRODUCT_NAME}"
    JucePlugin_Manufacturer="${GATECRASHER_COMPANY_NAME}")

target_link_libraries(GatecrasherTests PRIVATE
    # The bug sweep's measurement drivers. TEST target only — never the plugin, which must not
    # carry an operator new override. See nf/testing/ProcessorHarness.h.
    nf::testing
    # ProgramManager.cpp is compiled into both targets and calls into core, so both link it.
    nf::core
    # GatecrasherBinaryData, because ReadoutConformanceTests includes ProgramHeader.h to read
    # readoutFormat() - the SHIPPING format, not a copy of it - and the header chain reaches
    # GatecrasherTheme.h, which pulls in BinaryData.h for the embedded faces. Linking the real
    # generated header rather than stubbing it is the point, and it is what Fifth Member's and
    # TapeRot's test targets already do.
    GatecrasherBinaryData
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
