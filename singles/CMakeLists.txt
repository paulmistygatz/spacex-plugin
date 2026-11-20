cmake_minimum_required(VERSION 3.22)

# Mistycat Singles - die SpaceX-Module als einzelne Plugins.
# Fester Fairy-Tale-Look, keine Presets, kein Random, keine Kategorien.
# Klang und Zeichnung kommen direkt aus SpaceX (../Source/DSP, ../Source/GUI).

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

set(MS_SPACEX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../Source")

# Ein Einzel-Plugin. Name im Host = PRODUCT (kein Herstellername).
# CODE und BUNDLE nach dem ersten Verkauf nie mehr aendern.
function(mistycat_single TARGET PRODUCT CODE BUNDLE SOURCE)
    juce_add_plugin(${TARGET}
        COMPANY_NAME "Mistycat"
        PLUGIN_MANUFACTURER_CODE Mcat
        PLUGIN_CODE ${CODE}
        FORMATS ${MS_FORMATS}
        PRODUCT_NAME "${PRODUCT}"
        BUNDLE_ID "${BUNDLE}"
        VERSION ${PROJECT_VERSION}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES "Fx" "Stereo"
        AU_MAIN_TYPE kAudioUnitType_Effect)
    target_sources(${TARGET} PRIVATE ${SOURCE})
    target_include_directories(${TARGET} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        "${MS_SPACEX_SRC}"
        "${MS_SPACEX_SRC}/DSP")
    target_compile_definitions(${TARGET} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        SPACEX_CPU_OPT=1
        SPACEX_PX_DIAG_ONLY=1)
    target_link_libraries(${TARGET}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

mistycat_single(Micropitch "Micropitch" Mpit "com.mistycat.micropitch" micropitch/Micropitch.cpp)
mistycat_single(LCR        "LCR"        Mlcr "com.mistycat.lcr"        lcr/LCR.cpp)
mistycat_single(Polarity   "Polarity"   Mpol "com.mistycat.polarity"   polarity/Polarity.cpp)
mistycat_single(MidSide    "Mid-Side"   Mmsd "com.mistycat.midside"    midside/MidSide.cpp)
mistycat_single(Autopan    "Autopan"    Mapn "com.mistycat.autopan"    autopan/Autopan.cpp)
mistycat_single(Phaser     "Phaser"     Mphs "com.mistycat.phaser"     phaser/Phaser.cpp)
