idf_component_register(SRCS "pitch_detector.c" "note_mapper.c" "pitch_tracker.c" "melody_detector.c"
                       INCLUDE_DIRS "include")

# Pitch hot loops must meet the real-time hop budget on the target.
set_source_files_properties(melody_detector.c pitch_detector.c PROPERTIES COMPILE_OPTIONS "-O2")
