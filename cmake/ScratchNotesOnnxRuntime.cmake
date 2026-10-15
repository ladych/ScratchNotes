# ONNX Runtime (MIT) runs the Basic Pitch model of the note detection (F-TX-02). CMake downloads the
# official prebuilt package of the pinned release from GitHub instead of building it; SNOTES_ONNXRUNTIME_DIR
# uses a local copy instead (e.g. for offline builds). The program loads it as a shared library, which is
# copied next to every executable that links it (snotes_copy_onnxruntime).

set(SNOTES_ONNXRUNTIME_VERSION 1.31.0)

if(NOT SNOTES_ONNXRUNTIME_DIR)
    if(WIN32)
        set(_ort_package onnxruntime-win-x64-${SNOTES_ONNXRUNTIME_VERSION}.zip)
        set(_ort_hash 3958d8a44984160692c1b4ffd0de3cb48f8c39f9d34bb31e433db06e30b82195)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(_ort_package onnxruntime-linux-x64-${SNOTES_ONNXRUNTIME_VERSION}.tgz)
        set(_ort_hash cc5c72baf5ae5c8238a6841f0897227be2d02826b9cf98eaf02fdefaeeb03a57)
    else()
        message(FATAL_ERROR "No prebuilt ONNX Runtime for this platform; set SNOTES_ONNXRUNTIME_DIR")
    endif()
    include(FetchContent)
    FetchContent_Declare(onnxruntime
        URL https://github.com/microsoft/onnxruntime/releases/download/v${SNOTES_ONNXRUNTIME_VERSION}/${_ort_package}
        URL_HASH SHA256=${_ort_hash}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        # The package has no CMake project to build; only its headers and libraries are used.
        SOURCE_SUBDIR no-cmake-project)
    FetchContent_MakeAvailable(onnxruntime)
    set(SNOTES_ONNXRUNTIME_DIR "${onnxruntime_SOURCE_DIR}")
endif()

if(NOT EXISTS "${SNOTES_ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h")
    message(FATAL_ERROR "The ONNX Runtime in ${SNOTES_ONNXRUNTIME_DIR} has no include/onnxruntime_cxx_api.h")
endif()

add_library(snotes_onnxruntime SHARED IMPORTED GLOBAL)
target_include_directories(snotes_onnxruntime SYSTEM INTERFACE "${SNOTES_ONNXRUNTIME_DIR}/include")
if(WIN32)
    set_target_properties(snotes_onnxruntime PROPERTIES
        IMPORTED_LOCATION "${SNOTES_ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
        IMPORTED_IMPLIB "${SNOTES_ONNXRUNTIME_DIR}/lib/onnxruntime.lib")
else()
    set_target_properties(snotes_onnxruntime PROPERTIES
        IMPORTED_LOCATION "${SNOTES_ONNXRUNTIME_DIR}/lib/libonnxruntime.so.${SNOTES_ONNXRUNTIME_VERSION}"
        IMPORTED_SONAME "libonnxruntime.so.1")
endif()
message(STATUS "ONNX Runtime ${SNOTES_ONNXRUNTIME_VERSION} in ${SNOTES_ONNXRUNTIME_DIR}")

# Copies the ONNX Runtime library next to the executable, so it starts without installing anything.
# On Linux the executable finds it there through its RPATH $ORIGIN.
function(snotes_copy_onnxruntime target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:snotes_onnxruntime> $<TARGET_FILE_DIR:${target}>
        COMMAND_EXPAND_LISTS)
    if(NOT WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                libonnxruntime.so.${SNOTES_ONNXRUNTIME_VERSION} $<TARGET_FILE_DIR:${target}>/libonnxruntime.so.1)
        set_target_properties(${target} PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
    endif()
endfunction()
