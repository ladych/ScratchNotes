cmake_minimum_required(VERSION 3.22)

project(ScratchNotes VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows: static MSVC runtime, so ScratchNotes.exe runs on machines without the
# Visual C++ Redistributable (testers get only the exe). Applies to JUCE, Verovio and GoogleTest too.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(SNOTES_BUILD_APP "Build the JUCE desktop application" ON)
option(SNOTES_BUILD_TESTS "Build the unit tests" ON)
option(SNOTES_COVERAGE "Instrument the build for coverage (GCC/Clang)" OFF)
option(SNOTES_ASIO "Windows: build with ASIO (Steinberg ASIO SDK, GPLv3)" ON)
set(SNOTES_ASIO_SDK_DIR "" CACHE PATH "Windows: local ASIO SDK directory (contains common/iasiodrv.h); empty downloads it")
set(SNOTES_ONNXRUNTIME_DIR "" CACHE PATH "Local ONNX Runtime package (contains include/ and lib/); empty downloads it")
set(SNOTES_SANITIZERS "" CACHE STRING "Comma-separated sanitizers, e.g. address,undefined or thread")

include(cmake/ScratchNotesOptions.cmake)
include(cmake/ScratchNotesJuce.cmake)
include(cmake/ScratchNotesOnnxRuntime.cmake)
include(cmake/ScratchNotesVerovio.cmake)

add_subdirectory(src)

if(SNOTES_BUILD_TESTS)
    enable_testing()
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()
