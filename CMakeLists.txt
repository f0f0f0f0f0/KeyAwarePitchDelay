cmake_minimum_required(VERSION 3.22)

project(KeyAwarePitchDelay VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# ---- JUCE dependency ----
# Option A: place JUCE in a subfolder named "JUCE" next to this CMakeLists.txt
# Option B: pass -DJUCE_DIR=/path/to/JUCE when configuring
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "Using JUCE from ${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    add_subdirectory(JUCE)
elseif (DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Using JUCE from ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    message(FATAL_ERROR "JUCE not found. Either clone JUCE into ./JUCE or configure with -DJUCE_DIR=/path/to/JUCE")
endif()

# ---- Plugin target ----
juce_add_plugin(KeyAwarePitchDelay
    COMPANY_NAME "AAAPLUGIN"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE KAwA
    PLUGIN_CODE KAPD

    FORMATS AU VST3 Standalone
    PRODUCT_NAME "KeyAware Pitch Delay"
)

juce_generate_juce_header(KeyAwarePitchDelay)

target_sources(KeyAwarePitchDelay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(KeyAwarePitchDelay PRIVATE
    Source
    Source/DSP
)

target_compile_definitions(KeyAwarePitchDelay PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(KeyAwarePitchDelay PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
)

