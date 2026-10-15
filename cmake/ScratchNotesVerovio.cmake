# Verovio (LGPLv3) renders the notes and the tablature from MusicXML as SVG (F-NA-01). It is a Git submodule
# (external/verovio, release 6.3.0) built here as a static library with our own target instead of its
# CMake project, which builds a command-line tool and sets compile options for its whole directory.
#
# Only the formats Scratch Notes needs are compiled: MEI and MusicXML in, SVG out. Humdrum, Plaine & Easie,
# ABC, GABC and DARMS are left out, which also roughly halves the build time.
#
# Verovio reads its fonts from a resource directory at run time (external/verovio/data). It is copied
# next to every executable that renders notes (snotes_copy_verovio_data).

set(SNOTES_VEROVIO_DIR "${PROJECT_SOURCE_DIR}/external/verovio")
if(NOT EXISTS "${SNOTES_VEROVIO_DIR}/include/vrv/toolkit.h")
    message(FATAL_ERROR "Verovio is missing in ${SNOTES_VEROVIO_DIR}: git submodule update --init --recursive")
endif()

file(GLOB _vrv_sources CONFIGURE_DEPENDS
    "${SNOTES_VEROVIO_DIR}/src/*.cpp"
    "${SNOTES_VEROVIO_DIR}/src/midi/*.cpp"
    "${SNOTES_VEROVIO_DIR}/src/crc/*.cpp"
    "${SNOTES_VEROVIO_DIR}/libmei/dist/*.cpp"
    "${SNOTES_VEROVIO_DIR}/libmei/addons/*.cpp")
list(APPEND _vrv_sources
    "${SNOTES_VEROVIO_DIR}/src/json/jsonxx.cc"
    "${SNOTES_VEROVIO_DIR}/src/pugi/pugixml.cpp")

add_library(snotes_verovio STATIC ${_vrv_sources})
# Verovio's public headers include from all of its include directories.
target_include_directories(snotes_verovio SYSTEM PUBLIC
    "${SNOTES_VEROVIO_DIR}/include"
    "${SNOTES_VEROVIO_DIR}/include/vrv"
    "${SNOTES_VEROVIO_DIR}/include/crc"
    "${SNOTES_VEROVIO_DIR}/include/midi"
    "${SNOTES_VEROVIO_DIR}/include/hum"
    "${SNOTES_VEROVIO_DIR}/include/json"
    "${SNOTES_VEROVIO_DIR}/include/pugi"
    "${SNOTES_VEROVIO_DIR}/include/tuning-library"
    "${SNOTES_VEROVIO_DIR}/include/zip"
    "${SNOTES_VEROVIO_DIR}/libmei/dist"
    "${SNOTES_VEROVIO_DIR}/libmei/addons"
    $<$<BOOL:${WIN32}>:${SNOTES_VEROVIO_DIR}/include/win32>)
target_compile_definitions(snotes_verovio PUBLIC
    NO_HUMDRUM_SUPPORT NO_PAE_SUPPORT USE_PAE_OLD_PARSER NO_ABC_SUPPORT NO_GABC_SUPPORT NO_DARMS_SUPPORT
    NO_RUNTIME
    # Never used: the program always sets the resource path (notation::ScoreRenderer).
    RESOURCE_DIR="")
if(MSVC)
    target_compile_options(snotes_verovio PRIVATE /bigobj /W0)
else()
    target_compile_options(snotes_verovio PRIVATE -w)
endif()
# UBSan reports undefined behaviour inside Verovio 6.3.0 on every MusicXML score with tablature (a vrv::TabGrp
# cast to vrv::Chord in iomusxml.cpp, an uninitialised bool in scoredef.h). That is upstream code, so UBSan is
# off for Verovio; ASan stays on, and our own code keeps all checks.
if(SNOTES_SANITIZERS MATCHES "undefined")
    target_compile_options(snotes_verovio PRIVATE -fno-sanitize=undefined)
endif()
# The coverage gate measures our own code only (E-02); instrumenting Verovio would only slow the build.
if(SNOTES_COVERAGE)
    target_compile_options(snotes_verovio PRIVATE -fno-profile-arcs -fno-test-coverage)
endif()

# Copies Verovio's resource directory (fonts) next to the executable as verovio-data/.
function(snotes_copy_verovio_data target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SNOTES_VEROVIO_DIR}/data" $<TARGET_FILE_DIR:${target}>/verovio-data)
endfunction()
