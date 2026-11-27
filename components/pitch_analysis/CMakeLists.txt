idf_component_register(SRCS "pitch_detector.c" "note_mapper.c" "pitch_tracker.c" "melody_detector.c"
                       INCLUDE_DIRS "include")

# FFT hot loop must meet the 32 ms hop budget on the target.
set_source_files_properties(melody_detector.c PROPERTIES COMPILE_OPTIONS "-O2")
