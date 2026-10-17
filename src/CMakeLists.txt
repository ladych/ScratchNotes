# Testable logic (E-02): everything in this library is measured by the coverage gate.
add_library(snotes_core STATIC
    audio/DeviceRecovery.cpp
    audio/DriverChoice.cpp
    engine/AudioEngine.cpp
    engine/AudioRingBuffer.cpp
    engine/ClickTrack.cpp
    engine/InputMeter.cpp
    engine/LatencyProbe.cpp
    engine/PlaybackMixer.cpp
    engine/RecordingSession.cpp
    engine/TakeAudio.cpp
    engine/TestToneSource.cpp
    engine/Transport.cpp
    i18n/LanguageCatalog.cpp
    i18n/Translate.cpp
    io/ProjectRepository.cpp
    io/ProjectSerializer.cpp
    io/SettingsStore.cpp
    io/StdFileSystem.cpp
    io/TakeFileWriter.cpp
    io/WavWriter.cpp
    model/InstrumentProfile.cpp
    model/Project.cpp
    model/ProjectFactory.cpp
    model/SnapshotBuilder.cpp
    model/SnapshotPublisher.cpp
    model/TreeChangeListener.cpp
    model/TreeNode.cpp
    model/UuidGenerator.cpp
    notation/ScoreRenderer.cpp
    transcription/OnnxRuntime.cpp
    ui/AboutInfo.cpp
    ui/AudioStatus.cpp
    ui/DocumentController.cpp
    ui/LatencyCalibration.cpp
    ui/LatencyCompensation.cpp
    ui/LevelMeter.cpp
    ui/RecordingController.cpp
    ui/RecordingTime.cpp
    ui/TakeAudioCache.cpp
    ui/ThemeSetting.cpp
    ui/TrackList.cpp
    ui/TransportControl.cpp
    ui/VoiceList.cpp
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
    app/Dialogs.cpp
    app/JuceDocumentView.cpp
    app/JuceTakeAudioLoader.cpp
    app/LatencyDialog.cpp
    app/MainComponent.cpp
    app/MainMenu.cpp
    app/TrackView.cpp
    app/WriterThread.cpp
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
