cmake_minimum_required(VERSION 3.22)

project(k4SideChain VERSION 1.0.0 LANGUAGES C CXX)

# Point at the JUCE checkout sitting next to this project
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE
                 ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(k4SideChain
    COMPANY_NAME            "k4 Audio"
    BUNDLE_ID               "com.k4audio.k4sidechain"
    PLUGIN_MANUFACTURER_CODE K4au
    PLUGIN_CODE             Scn1
    FORMATS                 VST3
    PRODUCT_NAME            "k4 SideChain"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    VST3_CATEGORIES         "Fx" "Dynamics")

target_sources(k4SideChain PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SidechainEngine.cpp
    Source/WaveformScope.cpp)

target_compile_definitions(k4SideChain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(k4SideChain PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
