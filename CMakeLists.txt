cmake_minimum_required(VERSION 3.22)
project(RaSampler VERSION 0.2.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
if(DEFINED JUCE_SOURCE_DIR)
  add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
else()
  FetchContent_Declare(JUCE URL https://codeload.github.com/juce-framework/JUCE/tar.gz/51a8a6d7aeae7326956d747737ccf1575e61e209 URL_HASH SHA256=20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe)
  FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(RaSampler
  COMPANY_NAME "Voyagersquare" BUNDLE_ID "com.voyagersquare.rasampler"
  PLUGIN_MANUFACTURER_CODE Vysq PLUGIN_CODE Rasm
  FORMATS VST3 Standalone PRODUCT_NAME "RaSampler"
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Sampler)
juce_generate_juce_header(RaSampler)
target_sources(RaSampler PRIVATE Source/Parameters.cpp Source/SampleData.cpp Source/Engine.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(RaSampler PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(RaSampler PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
juce_add_console_app(RaSamplerCheck PRODUCT_NAME "RaSamplerCheck")
juce_generate_juce_header(RaSamplerCheck)
target_sources(RaSamplerCheck PRIVATE Tests/CheckMain.cpp)
target_link_libraries(RaSamplerCheck PRIVATE RaSampler juce::juce_audio_utils juce::juce_dsp)
enable_testing()
add_test(NAME RaSamplerDSP COMMAND RaSamplerCheck --self-test)

if(MSVC)
  target_compile_options(RaSampler PRIVATE /MP4)
  target_compile_options(RaSamplerCheck PRIVATE /MP4)
endif()
