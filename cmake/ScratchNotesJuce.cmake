# JUCE as one static library (snotes_juce) shared by snotes_core, the tests and the app.
#
# JUCE modules are CMake INTERFACE libraries that compile their sources into every target linking
# them. The core library uses JUCE (strings, ValueTree, audio files), and the app links snotes_core, so
# linking the modules directly would compile JUCE twice into the app. Instead the modules are
# compiled once here, with JUCE's own warning level, and only their headers and compile definitions
# are passed on.
#
# Without the app only the non-GUI modules are built, so the coverage and sanitizer builds need no
# X11, ALSA or JACK headers and skip juceaide.

if(NOT SNOTES_BUILD_APP)
    set(JUCE_MODULES_ONLY ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)

# Every module whose headers our code may include, with all of their dependencies.
set(SNOTES_JUCE_MODULES juce_core juce_events juce_data_structures juce_audio_basics juce_audio_formats)
if(SNOTES_BUILD_APP)
    list(APPEND SNOTES_JUCE_MODULES
        juce_graphics juce_gui_basics juce_gui_extra
        juce_audio_devices juce_audio_processors juce_audio_utils)
endif()

add_library(snotes_juce STATIC)
target_link_libraries(snotes_juce
    PRIVATE
        ${SNOTES_JUCE_MODULES}
    PUBLIC
        juce::juce_recommended_config_flags)

target_compile_definitions(snotes_juce PUBLIC
    JUCE_STANDALONE_APPLICATION=1
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    $<IF:$<CONFIG:Debug>,DEBUG=1 _DEBUG=1,NDEBUG=1 _NDEBUG=1>
    $<$<PLATFORM_ID:Linux>:JUCE_JACK=1>
    $<$<PLATFORM_ID:Linux>:JUCE_ALSA=1>)

# ASIO (F-AO-03): Steinberg publishes the ASIO SDK under GPLv3 since October 2025, which is compatible
# with our AGPLv3 (section 13). The build downloads the pinned release from Steinberg instead of keeping a
# copy in the repository; SNOTES_ASIO_SDK_DIR uses a local copy instead (e.g. for offline builds).
if(WIN32 AND SNOTES_BUILD_APP AND SNOTES_ASIO)
    if(NOT SNOTES_ASIO_SDK_DIR)
        include(FetchContent)
        FetchContent_Declare(asiosdk
            URL https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip
            URL_HASH SHA256=d5ebf0c20dd2c5f43771fd0c1418f4b361bf52434ee670097cfa6b3a335e2eca
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            # The SDK has no CMake project of its own; only its headers and sources are used.
            SOURCE_SUBDIR no-cmake-project)
        FetchContent_MakeAvailable(asiosdk)
        set(SNOTES_ASIO_SDK_DIR "${asiosdk_SOURCE_DIR}")
    endif()
    if(NOT EXISTS "${SNOTES_ASIO_SDK_DIR}/common/iasiodrv.h")
        message(FATAL_ERROR "The ASIO SDK in ${SNOTES_ASIO_SDK_DIR} has no common/iasiodrv.h")
    endif()
    message(STATUS "ASIO enabled with the SDK in ${SNOTES_ASIO_SDK_DIR}")
    target_compile_definitions(snotes_juce PUBLIC JUCE_ASIO=1)
    target_include_directories(snotes_juce SYSTEM PUBLIC "${SNOTES_ASIO_SDK_DIR}/common")
endif()

foreach(module IN LISTS SNOTES_JUCE_MODULES)
    target_compile_definitions(snotes_juce PUBLIC $<TARGET_PROPERTY:${module},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(snotes_juce SYSTEM PUBLIC $<TARGET_PROPERTY:${module},INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()
