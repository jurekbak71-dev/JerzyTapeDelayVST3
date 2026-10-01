cmake_minimum_required(VERSION 3.22)
project(ToneSnap VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ToneSnap
    COMPANY_NAME "Jerzy Audio"
    BUNDLE_ID "audio.jerzy.tonesnap"
    PLUGIN_MANUFACTURER_CODE JzAu
    PLUGIN_CODE TnSp
    FORMATS VST3
    PRODUCT_NAME "ToneSnap")

juce_generate_juce_header(ToneSnap)
target_sources(ToneSnap PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)
target_compile_features(ToneSnap PRIVATE cxx_std_17)
target_compile_definitions(ToneSnap PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ToneSnap PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
