cmake_minimum_required(VERSION 3.22)
project(ArmNongMuangAnalyzer VERSION 34.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ArmNongMuangAnalyzer
    COMPANY_NAME "Muepim Studio"
    BUNDLE_ID "com.muepimstudio.armnongmuanganalyzer"
    PLUGIN_MANUFACTURER_CODE MnMu
    PLUGIN_CODE An32
    FORMATS VST3 Standalone
    PRODUCT_NAME "Arm Nong Muang Analyzer"
    DESCRIPTION "Bit-transparent spectrum and stereo position analyzer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(ArmNongMuangAnalyzer)

juce_add_binary_data(ArmNongMuangAnalyzerAssets
    NAMESPACE AnalyzerPng
    SOURCES
        Assets/ArmNongMuang_Analyzer_UI.png
        Assets/00_original_full_interface.png
        Assets/01_header_brand_and_preset.png
        Assets/02_spectrum_graph.png
        Assets/03_navigation_and_zoom.png
        Assets/04_phase_correlation_graph.png
        Assets/05_analyzer_controls.png
        Assets/06_stereo_level_meters.png
        Assets/07_data_memory_panel.png
        Assets/08_footer_branding.png
        Assets/09_button_save_data.png
        Assets/10_button_save.png
        Assets/11_button_graph_mem.png
        Assets/12_button_clear_mem.png
        Assets/13_button_lf_res.png
        Assets/14_button_weight.png
        Assets/15_button_freeze.png
        Assets/16_button_show.png
        Assets/17_button_peak_hold.png
        Assets/18_button_clear.png
        Assets/19_button_detect_rms.png
        Assets/20_button_response.png
)

target_sources(ArmNongMuangAnalyzer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(ArmNongMuangAnalyzer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ArmNongMuangAnalyzer PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    ArmNongMuangAnalyzerAssets
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
