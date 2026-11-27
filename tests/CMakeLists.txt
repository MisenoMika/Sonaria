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
sonaria_test(screen_tests test_screen.c ${TUNER}/tuner_screen.c ${OLED}/oled_graphics.c)
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
