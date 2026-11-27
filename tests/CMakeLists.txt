cmake_minimum_required(VERSION 3.16)
project(sonaria_host_tests C)
enable_testing()

set(COMPONENTS ${CMAKE_CURRENT_SOURCE_DIR}/../components)
set(PITCH ${COMPONENTS}/pitch_analysis)
set(OLED ${COMPONENTS}/oled_display)
set(TUNER ${COMPONENTS}/tuner_display)

function(sonaria_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${PITCH}/include ${OLED}/include ${TUNER}/include)
    target_compile_features(${name} PRIVATE c_std_11)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(${name} PRIVATE m)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

sonaria_test(pitch_tests test_pitch.c ${PITCH}/pitch_detector.c)
sonaria_test(note_tests test_note.c ${PITCH}/note_mapper.c)
sonaria_test(tracker_tests test_tracker.c ${PITCH}/pitch_tracker.c ${PITCH}/pitch_detector.c ${PITCH}/note_mapper.c)
sonaria_test(screen_tests test_screen.c ${TUNER}/tuner_screen.c ${OLED}/oled_graphics.c ${OLED}/oled_text.c)
sonaria_test(melody_tests test_melody.c ${PITCH}/melody_detector.c ${PITCH}/pitch_detector.c ${PITCH}/pitch_tracker.c ${PITCH}/note_mapper.c)

# Asset regression: score is independent of detector input, and the runner
# executes the same C implementation as the target firmware.
find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(SONARIA_FFMPEG ffmpeg)
find_program(SONARIA_HOST_CC cc)
if(Python3_Interpreter_FOUND AND SONARIA_FFMPEG AND SONARIA_HOST_CC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/twinkle_twinkle.mp3")
    add_test(NAME score_twinkle COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/simulate_score.py --mode melody --check)
endif()

if(Python3_Interpreter_FOUND AND SONARIA_FFMPEG AND SONARIA_HOST_CC)
    add_test(NAME score_mary_mono COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/simulate_score.py --score mary --mode mono --synthesize --check)
endif()

if(Python3_Interpreter_FOUND)
    set(PRACTICE ${COMPONENTS}/practice)
    set(PRACTICE_DISPLAY ${COMPONENTS}/practice_display)
    set(SCORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../scores)
    file(GLOB SCORE_FILES CONFIGURE_DEPENDS ${SCORE_DIR}/*.json)
    set(GENERATED_SCORES ${CMAKE_CURRENT_BINARY_DIR}/generated_scores.c)
    add_custom_command(OUTPUT ${GENERATED_SCORES}
        COMMAND ${Python3_EXECUTABLE} ${SCORE_DIR}/compile_scores.py ${SCORE_DIR} ${GENERATED_SCORES}
        DEPENDS ${SCORE_DIR}/compile_scores.py ${SCORE_FILES} VERBATIM)
    add_library(practice_host STATIC ${GENERATED_SCORES} ${PRACTICE}/practice_session.c ${PRACTICE}/score_catalog.c
        ${PRACTICE}/note_event_detector.c ${PRACTICE}/practice_engine.c ${PRACTICE}/practice_control.c
        ${PITCH}/pitch_detector.c ${PITCH}/pitch_tracker.c ${PITCH}/note_mapper.c)
    target_include_directories(practice_host PUBLIC ${PRACTICE}/include ${PITCH}/include)
    target_compile_features(practice_host PRIVATE c_std_11)
    target_compile_options(practice_host PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(practice_host PUBLIC m)
    sonaria_test(practice_tests test_practice.c ${PRACTICE_DISPLAY}/practice_screen.c ${OLED}/oled_graphics.c ${OLED}/oled_text.c)
    target_include_directories(practice_tests PRIVATE ${PRACTICE_DISPLAY}/include)
    target_link_libraries(practice_tests PRIVATE practice_host)
    add_executable(simulate_practice simulate_practice.c)
    target_link_libraries(simulate_practice PRIVATE practice_host)
    add_test(NAME score_compiler COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_score_compiler.py)
    add_test(NAME practice_wrong_note COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_practice_replay.py $<TARGET_FILE:simulate_practice>)
    if(SONARIA_FFMPEG AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/twinkle_twinkle_wrong1.wav"
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/twinkle_twinkle_wrong2.wav"
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/mary_had_a_little_lamb_wrong1.wav"
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/mary_had_a_little_lamb_wrong2.wav")
        add_test(NAME practice_piano_errors COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_piano_fixtures.py $<TARGET_FILE:simulate_practice>)
    endif()
    if(SONARIA_FFMPEG AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/mary_had_a_little_lamb_monophonic.wav")
        add_test(NAME practice_mary COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/simulate_practice.py $<TARGET_FILE:simulate_practice> mary ${CMAKE_CURRENT_SOURCE_DIR}/../assets/mary_had_a_little_lamb_monophonic.wav --check)
    endif()
    if(SONARIA_FFMPEG AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/twinkle_twinkle_monophonic.wav")
        add_test(NAME practice_twinkle COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/simulate_practice.py $<TARGET_FILE:simulate_practice> twinkle ${CMAKE_CURRENT_SOURCE_DIR}/../assets/twinkle_twinkle_monophonic.wav --check)
    endif()
endif()
