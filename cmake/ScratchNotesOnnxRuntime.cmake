# ONNX Runtime (MIT) runs the Basic Pitch model of the note detection (F-TX-02). CMake downloads the
# official prebuilt package of the pinned release from nuget.org (Microsoft.ML.OnnxRuntime, one package
# for Windows and Linux) instead of building it; SNOTES_ONNXRUNTIME_DIR uses a local copy instead (e.g. for
# offline builds, a directory with include/ and lib/). The program loads it as a shared library, which is
# copied next to every executable that links it (snotes_copy_onnxruntime).

set(SNOTES_ONNXRUNTIME_VERSION 1.31.0)

if(NOT SNOTES_ONNXRUNTIME_DIR)
    if(WIN32)
        set(_ort_runtime win-x64)
        set(_ort_libraries onnxruntime.dll onnxruntime.lib)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(_ort_runtime linux-x64)
        set(_ort_libraries libonnxruntime.so)
    else()
        message(FATAL_ERROR "No prebuilt ONNX Runtime for this platform; set SNOTES_ONNXRUNTIME_DIR")
    endif()
    include(FetchContent)
    FetchContent_Declare(onnxruntime
        URL https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/${SNOTES_ONNXRUNTIME_VERSION}/microsoft.ml.onnxruntime.${SNOTES_ONNXRUNTIME_VERSION}.nupkg
        URL_HASH SHA256=7e35fdb4f5adb62f01e60798e2e5ed05108f5fbbe1cca03fbcc37903cf7c5e3b
        # A .nupkg is a zip archive; CMake recognises it only by this extension.
        DOWNLOAD_NAME onnxruntime-${SNOTES_ONNXRUNTIME_VERSION}.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        # The package has no CMake project to build; only its headers and libraries are used.
        SOURCE_SUBDIR no-cmake-project)
    FetchContent_MakeAvailable(onnxruntime)
    # Arranges the package like a local copy: include/ and lib/ with the versioned Linux library name.
    set(SNOTES_ONNXRUNTIME_DIR "${CMAKE_BINARY_DIR}/onnxruntime-${SNOTES_ONNXRUNTIME_VERSION}")
    file(COPY "${onnxruntime_SOURCE_DIR}/build/native/include/" DESTINATION "${SNOTES_ONNXRUNTIME_DIR}/include")
    foreach(_ort_library IN LISTS _ort_libraries)
        set(_ort_target "${_ort_library}")
        if(_ort_library STREQUAL "libonnxruntime.so")
            set(_ort_target "libonnxruntime.so.${SNOTES_ONNXRUNTIME_VERSION}")
        endif()
        configure_file("${onnxruntime_SOURCE_DIR}/runtimes/${_ort_runtime}/native/${_ort_library}"
                       "${SNOTES_ONNXRUNTIME_DIR}/lib/${_ort_target}" COPYONLY)
    endforeach()
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
