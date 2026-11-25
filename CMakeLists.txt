cmake_minimum_required(VERSION 3.22)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS" FORCE)
endif()
project(FeetBack VERSION 1.0.0 LANGUAGES C CXX)
if(NOT APPLE)
    message(FATAL_ERROR "This project builds a macOS Audio Unit. Use a Mac to build it.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/JUCE-9.0.5" CACHE PATH "JUCE 9.0.5 source")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE is missing. Run bash scripts/build.sh to download the pinned dependency.")
endif()
add_subdirectory("${JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)

juce_add_binary_data(FeetBackArt NAMESPACE FeetBackArt SOURCES Assets/feet.svg)
juce_add_plugin(FeetBack
    VERSION 1.0.0 COMPANY_NAME "Niko Audio"
    BUNDLE_ID "audio.niko.feetback"
    PLUGIN_MANUFACTURER_CODE Niko PLUGIN_CODE FtBk
    PRODUCT_NAME "FeetBack"
    DESCRIPTION "Singing guitar feedback with harmonic selection"
    FORMATS AU AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(FeetBack PRIVATE Processor.cpp Editor.cpp)
target_compile_definitions(FeetBack PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(FeetBack PRIVATE FeetBackArt juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_executable(FeetBackDspCheck DspCheck.cpp)
target_compile_features(FeetBackDspCheck PRIVATE cxx_std_17)
target_compile_options(FeetBackDspCheck PRIVATE -O3 -Wall -Wextra -Wpedantic)
add_executable(FeetBackPluginCheck PluginCheck.cpp)
target_include_directories(FeetBackPluginCheck PRIVATE "${JUCE_DIR}/modules")
target_compile_definitions(FeetBackPluginCheck PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
target_link_libraries(FeetBackPluginCheck PRIVATE FeetBack)

enable_testing()
add_test(NAME DSP COMMAND FeetBackDspCheck)
add_test(NAME PluginStateAndUI COMMAND FeetBackPluginCheck "${CMAKE_CURRENT_BINARY_DIR}/FeetBack-preview.png")
