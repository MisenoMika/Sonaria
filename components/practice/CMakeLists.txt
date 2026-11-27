idf_component_register(SRCS "score_catalog.c" "practice_session.c" "note_event_detector.c" "practice_engine.c" "practice_control.c" "sweep_demo.c"
                       INCLUDE_DIRS "include" REQUIRES pitch_analysis)
set(score_dir "${CMAKE_CURRENT_LIST_DIR}/../../scores")
file(GLOB score_files CONFIGURE_DEPENDS "${score_dir}/*.json")
idf_build_get_property(python PYTHON)
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated_scores.c")
add_custom_command(OUTPUT "${generated}"
    COMMAND "${python}" "${score_dir}/compile_scores.py" "${score_dir}" "${generated}"
    DEPENDS "${score_dir}/compile_scores.py" ${score_files} VERBATIM)
target_sources(${COMPONENT_LIB} PRIVATE "${generated}")
