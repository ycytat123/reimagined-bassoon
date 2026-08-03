cmake_minimum_required(VERSION 3.22)
project(LTCReader VERSION 1.0.0)

# Add JUCE framework
add_subdirectory(juce)

# libltc static library
add_library(ltc STATIC
    libltc/src/decoder.c
    libltc/src/encoder.c
    libltc/src/ltc.c
    libltc/src/timecode.c
)
target_include_directories(ltc PUBLIC libltc/src)
target_compile_definitions(ltc PRIVATE LTC_NO_ABI)
if(MSVC)
    target_compile_options(ltc PRIVATE /W0)
endif()

# Plugin sources
set(PluginSources
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/LtcDecoder.cpp
    src/TimecodeDisplay.cpp
    src/DarkTheme.cpp
    src/LicenseVerifier.cpp
)

# JUCE plugin target
juce_add_plugin(LTCReader
    COMPANY_NAME             "Hahahah"
    BUNDLE_ID                "com.hahahah.ltcreader"
    PLUGIN_CODE               LTCr
    PLUGIN_MANUFACTURER_CODE "Haha"
    FORMATS                   VST3 AU
    PLUGIN_NAME              "LTC Reader"
    PRODUCT_NAME             "LTC Reader"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_INSTRUMENT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER      "Hahahah"
    VST3_CATEGORIES          "Fx|Analyzer"
)

# Add user source files to the plugin target
target_sources(LTCReader PRIVATE ${PluginSources})

# Generate JuceHeader.h for our target (required for user sources to #include <JuceHeader.h>)
juce_generate_juce_header(LTCReader)

target_include_directories(LTCReader PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/LTCReader_artefacts/JuceLibraryCode
    src
)

# Suppress VST3 parameter migration error (we only ship VST3, no VST2)
target_compile_definitions(LTCReader PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# Enable juce_cryptography for RSA license verification (SHA-256 hashing)
target_link_libraries(LTCReader PRIVATE ltc juce_cryptography)
target_include_directories(LTCReader PRIVATE src)

# ─────────────────────────────────────────────────────────────────────
# Debug tool: prints the machine ID the plugin computes (for licensing)
# Build:  cmake --build build --config Release --target machineid_tool
# ─────────────────────────────────────────────────────────────────────
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/machineid.cpp)
    juce_add_console_app(machineid_tool
        PRODUCT_NAME          "machineid_tool"
        COMPANY_NAME          "Hahahah"
        BUNDLE_ID             "com.hahahah.machineidtool"
    )
    target_sources(machineid_tool PRIVATE tools/machineid.cpp src/LicenseVerifier.cpp)
    target_include_directories(machineid_tool PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/machineid_tool_artefacts/JuceLibraryCode
        src
    )
    target_link_libraries(machineid_tool PRIVATE juce_cryptography)
    juce_generate_juce_header(machineid_tool)
endif()

# ─────────────────────────────────────────────────────────────────────
# License Generator: GUI app for generating .ltclic license files
# Build:  cmake --build build --config Release --target license_gen
# ─────────────────────────────────────────────────────────────────────
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/license_gen.cpp)
    juce_add_gui_app(license_gen
        PRODUCT_NAME          "LTC License Generator"
        COMPANY_NAME          "Hahahah"
        BUNDLE_ID             "com.hahahah.licensegen"
    )
    target_sources(license_gen PRIVATE
        tools/license_gen.cpp
        src/LicenseSigner.cpp
    )
    target_include_directories(license_gen PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/license_gen_artefacts/JuceLibraryCode
        src
    )
    target_link_libraries(license_gen PRIVATE juce_cryptography juce_gui_basics juce_gui_extra)
    juce_generate_juce_header(license_gen)
endif()
