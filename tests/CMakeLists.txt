add_executable(snotes_tests
    audio/DeviceRecoveryTest.cpp
    audio/DriverChoiceTest.cpp
    engine/AudioEngineTest.cpp
    engine/AudioRingBufferTest.cpp
    engine/ClickTrackTest.cpp
    engine/InputMeterTest.cpp
    engine/LatencyProbeTest.cpp
    engine/PlaybackMixerTest.cpp
    engine/RecorderTest.cpp
    engine/SnapshotExchangeTest.cpp
    engine/SpscQueueTest.cpp
    engine/TakeAudioTest.cpp
    engine/TestToneSourceTest.cpp
    engine/TransportTest.cpp
    i18n/GermanTestLanguage.cpp
    i18n/LanguageCatalogTest.cpp
    i18n/TranslateTest.cpp
    io/ProjectRepositoryTest.cpp
    io/ProjectSerializerTest.cpp
    io/SettingsStoreTest.cpp
    io/StdFileSystemTest.cpp
    io/TakeFileWriterTest.cpp
    io/WavWriterTest.cpp
    model/BeatGridTest.cpp
    model/InstrumentProfileTest.cpp
    model/ProjectTest.cpp
    model/SnapshotBuilderTest.cpp
    model/TreeChangeListenerTest.cpp
    model/UuidGeneratorTest.cpp
    notation/ScoreRendererTest.cpp
    notation/MusicXmlTest.cpp
    notation/ScoreMidiTest.cpp
    notation/ScoreTest.cpp
    notation/SvgForJuceTest.cpp
    rhythm/BeatTrackerTest.cpp
    rhythm/CleanUpTest.cpp
    rhythm/MeterTest.cpp
    rhythm/OnsetStrengthTest.cpp
    rhythm/QuantizerTest.cpp
    rhythm/RhythmAnalysisTest.cpp
    rhythm/RhythmCsvTest.cpp
    rhythm/RhythmReference.cpp
    tab/TabTextTest.cpp
    tab/TablatureTest.cpp
    transcription/CommandLineTest.cpp
    transcription/LevelGateTest.cpp
    transcription/NoteCreationTest.cpp
    transcription/NoteMidiTest.cpp
    transcription/OnnxRuntimeTest.cpp
    transcription/PitchAnalysisTest.cpp
    transcription/ReferenceData.cpp
    transcription/ResamplerTest.cpp
    transcription/TranscriberTest.cpp
    ui/AboutInfoTest.cpp
    ui/AudioStatusTest.cpp
    ui/DocumentControllerTest.cpp
    ui/GridCorrectionTest.cpp
    ui/LatencyCalibrationTest.cpp
    ui/LatencyCompensationTest.cpp
    ui/LevelMeterTest.cpp
    ui/RecordingControllerTest.cpp
    ui/RhythmUpdateTest.cpp
    ui/ScoreDisplayTest.cpp
    ui/ScoreExportTest.cpp
    ui/TakeAudioCacheTest.cpp
    ui/ThemeSettingTest.cpp
    ui/TrackListTest.cpp
    ui/TranscriptionControllerTest.cpp
    ui/TransportControlTest.cpp
    ui/VoiceListTest.cpp
)

# Q-04: replaces the global operator new to count allocations, which clashes with the
# sanitizers' own allocator. The regular, coverage and Windows builds run it.
if(NOT SNOTES_SANITIZERS)
    target_sources(snotes_tests PRIVATE engine/RealtimeAllocationTest.cpp)
endif()

target_link_libraries(snotes_tests PRIVATE snotes_core GTest::gmock_main)
target_include_directories(snotes_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The tests run in German (i18n/GermanTestLanguage.cpp), so they also check the German translations.
# Verovio reads its fonts from its own resource directory in the submodule.
target_compile_definitions(snotes_tests PRIVATE
    SNOTES_LANGUAGES_DIR="${PROJECT_SOURCE_DIR}/resources/Languages"
    SNOTES_VEROVIO_DATA_DIR="${SNOTES_VEROVIO_DIR}/data"
    SNOTES_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    SNOTES_MODEL_FILE="${PROJECT_SOURCE_DIR}/resources/Models/basic-pitch.onnx")
snotes_enable_warnings(snotes_tests)
snotes_copy_onnxruntime(snotes_tests)

include(GoogleTest)
gtest_discover_tests(snotes_tests DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "TSAN_OPTIONS=suppressions=${CMAKE_CURRENT_SOURCE_DIR}/tsan-suppressions.txt")

# Every language file translates every text of the program.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME TranslationsComplete
             COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/check-translations.py ${PROJECT_SOURCE_DIR})
endif()
