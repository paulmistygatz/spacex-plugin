cmake_minimum_required(VERSION 3.22)

# Universal Binary (Apple Silicon + Intel) - MUSS vor project() gesetzt
# werden, sonst baut CMake nur fuer die Architektur des Mac, auf dem
# gerade kompiliert wird. Ohne das laeuft das fertige Plugin auf einem
# Intel-Mac (z.B. bei einem Freund/Kollegen) gar nicht erst an, obwohl
# Cubase beim Scannen nicht mal meckert - es erscheint einfach nicht in
# der Plugin-Liste.
if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
    # Mindest-macOS. OHNE diese Zeile traegt CMake die Version des BUILD-Rechners
    # ein - ein auf macOS 26 gebautes Plugin laesst sich dann auf keinem
    # aelteren System laden, obwohl beide Architekturen drinstecken. Genau
    # daran ist die erste Fassung auf einem Intel-Mac gescheitert: universal
    # ja, aber "min macOS 26.0".
    # 10.15 (Catalina) deckt praktisch jeden Intel-Mac ab, der heute noch in
    # einem Studio steht. Sollte JUCE das irgendwann nicht mehr mitmachen, ist
    # 11.0 (Big Sur) die naechste sinnvolle Stufe.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version" FORCE)
endif()

project(LCRMSPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE wird beim Konfigurieren automatisch heruntergeladen (Internetzugang noetig)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # Feste Version statt "master": der Mac-Build hier und der Windows-Build
    # auf GitHub muessen dasselbe JUCE benutzen, sonst sucht man Fehler, die
    # nur aus einem zwischenzeitlichen JUCE-Commit stammen. 9.0.2 ist genau
    # das, was lokal schon geholt wurde - fuer diesen Rechner aendert sich
    # also nichts.
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# ===== Layout-Vergleichsbuilds =====
# Leer = normales SpaceX (unveraendert). A, B oder C baut ein EIGENES Plugin
# (eigener Name, eigener Plugin-Code), damit alle gleichzeitig im Host
# geladen werden koennen. Siehe build_variants.sh.
set(SPACEX_VARIANT "" CACHE STRING "Vergleichsbuild: leer, box, dotsin, dotsout, paraCPU (alt: raye1, raye2, A, B, C)")
set(SPX_PRODUCT "SpaceX")
set(SPX_CODE    "Lcrm")
set(SPX_ROW2    0)
# Runde 54 (User: "Der Build soll ab jetzt einfach wieder SpaceX heissen - ich
# will keine Variationen mehr"): was bisher nur SpaceXout hatte, ist jetzt der
# NORMALE Build. ./install.sh baut damit das fertige SpaceX.
set(SPX_PXUI    2)   # Parallax: 0 = Amount + 4 Knoepfe, 1 = drei Regler, 2 = Klick-Knopf
set(SPX_RAYEUI  1)   # RAYE: 1 = Amount + Charakter
set(SPX_CPUOPT  1)   # optimierte DSP - seit Runde 42 in allen Builds (User: "CPU mode passt")
set(SPX_NOVIS   0)
set(SPX_DOTSIN  0)   # 1 = Modus-Punkte IN der Pille (Parallax + RAYE)
# Runde 98 (User: "mach mal diag als das normale"): die Icon-Fassung IST jetzt
# der normale Build. Die alte Fassung mit Pillenrahmen und ohne Icons gibt es
# noch als Vergleichsbuild "box" (SpaceXbox).
set(SPX_PXDIAG  1)
set(SPX_EXTRA   BUNDLE_ID "com.mistycat.spacex")
set(SPX_OWN     "")
if (SPACEX_VARIANT STREQUAL "A")
    set(SPX_ROW2 1)
    set(SPX_OWN "SpaceX-A" "SpxA")
elseif (SPACEX_VARIANT STREQUAL "B")
    set(SPX_ROW2 2)
    set(SPX_OWN "SpaceX-B" "SpxB")
elseif (SPACEX_VARIANT STREQUAL "C")
    set(SPX_ROW2 3)
    set(SPX_OWN "SpaceX-C" "SpxC")
elseif (SPACEX_VARIANT STREQUAL "paraCPU")
    set(SPX_PXUI 1)
    set(SPX_CPUOPT 1)
    set(SPX_OWN "SpaceXparaCPU" "SXpc")
elseif (SPACEX_VARIANT STREQUAL "presets")
    set(SPX_PXUI 0)
    set(SPX_OWN "SpaceXpresets" "SXpr")
elseif (SPACEX_VARIANT STREQUAL "dotsout")
    # Runde 49: Parallax-Klickknopf + RAYE (Amount/Charakter), Punkte UNTER
    # den Pillen - wie bisher, nur in beiden Sektionen gleich gross.
    set(SPX_PXUI 2)
    set(SPX_RAYEUI 1)
    set(SPX_OWN "SpaceXout" "SXdo")
elseif (SPACEX_VARIANT STREQUAL "box")
    # Runde 98: die alte Fassung - Modusname in einer gerahmten Pille, keine
    # Icons, kein Schimmer.
    set(SPX_PXDIAG 0)
    set(SPX_OWN "SpaceXbox" "SXbx")
elseif (SPACEX_VARIANT STREQUAL "dotsin")
    # wie dotsout, aber die Punkte sitzen IN der Pille.
    set(SPX_PXUI 2)
    set(SPX_RAYEUI 1)
    set(SPX_DOTSIN 1)
    set(SPX_OWN "SpaceXin" "SXdi")
elseif (NOT SPACEX_VARIANT STREQUAL "")
    message(FATAL_ERROR "SPACEX_VARIANT muss leer, box, dotsin, dotsout, paraCPU, A, B oder C sein")
endif()
if (SPX_OWN)
    list(GET SPX_OWN 0 SPX_PRODUCT)
    list(GET SPX_OWN 1 SPX_CODE)
    string(TOLOWER "com.mistycat.${SPX_PRODUCT}" SPX_BUNDLE)
    set(SPX_EXTRA BUNDLE_ID "${SPX_BUNDLE}")
endif()

juce_add_plugin(LCRMSPlugin
    COMPANY_NAME "Mistycat"
    # ACHTUNG: der Hersteller-Code geht in die eindeutige Plugin-ID ein. Mit
    # der Aenderung sieht jeder Host SpaceX als NEUES Plugin - bestehende
    # Projekte finden die alte Instanz nicht mehr. Genau deshalb jetzt und
    # nicht nach dem Verkaufsstart.
    PLUGIN_MANUFACTURER_CODE Mcat
    PLUGIN_CODE ${SPX_CODE}
    FORMATS VST3
    PRODUCT_NAME "${SPX_PRODUCT}"
    ${SPX_EXTRA}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Stereo"
)

juce_generate_juce_header(LCRMSPlugin)

# Das Handbuch liegt im Plugin selbst (User-Wunsch). Beim ersten Oeffnen ueber
# das Menue wird es nach ~/Library/Application Support/SpaceX entpackt und
# dort geoeffnet - damit funktioniert "Open Manual" auch dann, wenn nur das
# VST3 kopiert und kein Installer benutzt wurde.
juce_add_binary_data(SpaceXManual
    HEADER_NAME SpaceXManualData.h
    NAMESPACE   SpaceXManualData
    SOURCES     docs/SpaceXManual_EN.pdf
                docs/theme-previews/theme_daynight.png
                docs/theme-previews/theme_fireflies.png
                docs/theme-previews/theme_scifi.png
                docs/theme-previews/theme_pop.png)

target_sources(LCRMSPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GUI/GoniometerComponent.cpp
    Source/GUI/SpaceAssets.cpp
)

target_compile_definitions(LCRMSPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    SPACEX_ROW2_VARIANT=${SPX_ROW2}
    SPACEX_PARALLAX_UI=${SPX_PXUI}
    SPACEX_RAYE_UI=${SPX_RAYEUI}
    SPACEX_CPU_OPT=${SPX_CPUOPT}
    SPACEX_NO_VISUALS=${SPX_NOVIS}
    SPACEX_DOTS_INSIDE=${SPX_DOTSIN}
    SPACEX_PX_DIAG_ONLY=${SPX_PXDIAG}
)

target_link_libraries(LCRMSPlugin
    PRIVATE
        SpaceXManual
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
