cmake_minimum_required(VERSION 3.22)

# Mistycat Singles - die SpaceX-Module als einzelne Plugins.
# Fester Fairy-Tale-Look, keine Presets, kein Random, keine Kategorien.
# Bisher: Micropitch. Weitere Module kommen als eigene Targets dazu.

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version" FORCE)
endif()

project(MistycatSingles VERSION 1.0.0 LANGUAGES C CXX)

get_property(MS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT MS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build-Typ (Standard: Release)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dasselbe JUCE wie SpaceX. Liegt es dort schon (SpaceX einmal gebaut), wird
# es mitbenutzt - dann muss nichts heruntergeladen werden.
set(MS_SPACEX_JUCE "${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/juce-src")
if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_JUCE AND EXISTS "${MS_SPACEX_JUCE}/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_JUCE "${MS_SPACEX_JUCE}" CACHE PATH "JUCE aus dem SpaceX-Build")
endif()
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if (APPLE)
    set(MS_FORMATS VST3 AU)
else()
    set(MS_FORMATS VST3)
endif()

# ===== Micropitch =====
# Name im Host: nur "Micropitch" (kein Herstellername im Plugin-Namen).
# PLUGIN_CODE und BUNDLE_ID nach dem ersten Verkauf nie mehr aendern.
juce_add_plugin(Micropitch
    COMPANY_NAME "Mistycat"
    PLUGIN_MANUFACTURER_CODE Mcat
    PLUGIN_CODE Mpit
    FORMATS ${MS_FORMATS}
    PRODUCT_NAME "Micropitch"
    BUNDLE_ID "com.mistycat.micropitch"
    VERSION ${PROJECT_VERSION}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Stereo"
    AU_MAIN_TYPE kAudioUnitType_Effect
)

target_sources(Micropitch PRIVATE
    micropitch/MicropitchProcessor.cpp
    micropitch/MicropitchEditor.cpp
)

target_include_directories(Micropitch PRIVATE common common/dsp micropitch)

target_compile_definitions(Micropitch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    SPACEX_CPU_OPT=1
)

target_link_libraries(Micropitch
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
