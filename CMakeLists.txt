cmake_minimum_required(VERSION 3.22)
project(NoizzFurther VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NoizzFurther
    COMPANY_NAME "Noizz Studio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Noiz
    PLUGIN_CODE NzFr
    FORMATS VST3
    PRODUCT_NAME "NoizzFurther"
)

juce_generate_juce_header(NoizzFurther)

target_sources(NoizzFurther
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/background.png")
    juce_add_binary_data(NoizzAssets SOURCES Assets/background.png)
    target_link_libraries(NoizzFurther PRIVATE NoizzAssets)
    target_compile_definitions(NoizzFurther PRIVATE HAS_BACKGROUND_PNG=1)
endif()

target_compile_definitions(NoizzFurther
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NoizzFurther
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)
