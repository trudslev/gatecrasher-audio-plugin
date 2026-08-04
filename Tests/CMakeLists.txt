juce_add_console_app(GatecrasherTests PRODUCT_NAME "GatecrasherTests")

target_sources(GatecrasherTests PRIVATE
    TestMain.cpp
    TriggerDetectorTests.cpp
    GateEnvelopeGeneratorTests.cpp
    ReverbTankTests.cpp
    DampingStageTests.cpp
    SlamSaturationTests.cpp
    StereoWidthStageTests.cpp
    OutputMixStageTests.cpp
    ParametersStateTests.cpp
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

target_compile_definitions(GatecrasherTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(GatecrasherTests PRIVATE
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
