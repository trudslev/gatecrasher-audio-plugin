juce_add_console_app(GatecrasherTests PRODUCT_NAME "GatecrasherTests")

target_sources(GatecrasherTests PRIVATE
    TestMain.cpp
    ReadoutConformanceTests.cpp
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
    ../Source/DSP/ProgramManager.cpp)

# NF_COMPANY_NAME/NF_PRODUCT_NAME come from the parent CMakeLists and must match what the plugin
# target gets - ProgramManager.cpp is compiled into both, and builds the user-Programs path from
# them. Inherited from the parent scope rather than restated here, so there is nothing to re-sync.
target_compile_definitions(GatecrasherTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NF_COMPANY_NAME="${GATECRASHER_COMPANY_NAME}"
    NF_PRODUCT_NAME="${GATECRASHER_PRODUCT_NAME}")

target_link_libraries(GatecrasherTests PRIVATE
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
