# Compiler settings shared by all of our own targets (E-08, E-09).

if(MSVC)
    # Sources are UTF-8 (German UI texts); without /utf-8 MSVC reads them in the ANSI code page.
    add_compile_options(/utf-8)
    set(SNOTES_WARNING_FLAGS /W4 /WX /permissive-)
else()
    # Designated initializers deliberately leave members at their defaults; GCC would flag that.
    set(SNOTES_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-missing-field-initializers -Werror)
endif()

# Applies warnings-as-errors to a target made only of our own sources.
function(snotes_enable_warnings target)
    target_compile_options(${target} PRIVATE ${SNOTES_WARNING_FLAGS})
endfunction()

# Applies warnings-as-errors to single source files. Used for the JUCE app target,
# whose JUCE module sources are compiled into the same target and must keep
# JUCE's own warning level.
function(snotes_enable_warnings_for_sources)
    set_source_files_properties(${ARGN} PROPERTIES COMPILE_OPTIONS "${SNOTES_WARNING_FLAGS}")
endfunction()

if(SNOTES_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "SNOTES_SANITIZERS is only supported with GCC/Clang")
    endif()
    add_compile_options(-fsanitize=${SNOTES_SANITIZERS} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${SNOTES_SANITIZERS})
endif()

if(SNOTES_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "SNOTES_COVERAGE is only supported with GCC/Clang")
    endif()
    # Atomic counters: the thread-safety tests run code on several threads at once, and racing plain
    # counters can make gcov report negative hit counts.
    add_compile_options(--coverage -O0 -fno-inline -fprofile-update=atomic)
    add_link_options(--coverage)
endif()
