# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(snotes_core STATIC
    audio/DeviceRecovery.cpp
    audio/DriverChoice.cpp
    engine/TestToneSource.cpp
    i18n/LanguageCatalog.cpp
    i18n/Translate.cpp
    io/SettingsStore.cpp
    io/StdFileSystem.cpp
    notation/ScoreRenderer.cpp
    transcription/OnnxRuntime.cpp
    ui/AboutInfo.cpp
    ui/AudioStatus.cpp
    ui/ChannelOverview.cpp
    ui/ThemeSetting.cpp
)
target_include_directories(snotes_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(snotes_core PUBLIC snotes_juce snotes_onnxruntime PRIVATE snotes_verovio)
snotes_enable_warnings(snotes_core)

if(NOT SNOTES_BUILD_APP)
    return()
endif()

# Humble objects (E-03): JUCE glue without logic, listed in coverage-exclusions.txt.
set(SNOTES_APP_SOURCES
    app/Main.cpp
    app/AboutDialog.cpp
    app/AppComposition.cpp
    app/AudioDeviceSupervisor.cpp
    app/ChannelList.cpp
    app/Dialogs.cpp
    app/MainComponent.cpp
    app/MainMenu.cpp
)

juce_add_gui_app(ScratchNotes
    PRODUCT_NAME "Scratch Notes"
    COMPANY_NAME "ladych"
    VERSION ${PROJECT_VERSION}
)

target_sources(ScratchNotes PRIVATE ${SNOTES_APP_SOURCES})
snotes_enable_warnings_for_sources(${SNOTES_APP_SOURCES})

target_compile_definitions(ScratchNotes PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ScratchNotes,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ScratchNotes,JUCE_VERSION>"
)

# Translation files (i18n::LanguageCatalog): one file per language, see resources/Languages/de.txt.
file(GLOB SNOTES_LANGUAGE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/resources/Languages/*.txt")
juce_add_binary_data(ScratchNotesLanguages
    HEADER_NAME LanguageData.h
    NAMESPACE LanguageData
    SOURCES ${SNOTES_LANGUAGE_FILES})
# Its generated header is included as a system header, so clang-tidy does not check it.
target_link_libraries(ScratchNotes PRIVATE $<LINK_ONLY:ScratchNotesLanguages>)
target_include_directories(ScratchNotes SYSTEM PRIVATE
    $<TARGET_PROPERTY:ScratchNotesLanguages,INTERFACE_INCLUDE_DIRECTORIES>)

# JUCE itself comes compiled from snotes_juce through snotes_core (cmake/ScratchNotesJuce.cmake).
target_link_libraries(ScratchNotes PRIVATE snotes_core)
snotes_copy_onnxruntime(ScratchNotes)
snotes_copy_verovio_data(ScratchNotes)
