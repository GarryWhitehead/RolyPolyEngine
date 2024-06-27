
# create the source group for targets - gives better file listings
# for IDEs such as Visual Studio
function(rpe_source_group)
    set(single_value_args TARGET ROOT_DIR)
    cmake_parse_arguments(
        SG
        ""
        "${single_value_args}"
        ""
        "${ARGN}"
    )
    get_property(
        TARGET_SOURCE_FILES
        TARGET ${SG_TARGET}
        PROPERTY SOURCES
    )

    get_property(
        TARGET_HEADER_FILES
        TARGET ${SG_TARGET}
        PROPERTY HEADERS
    )

    source_group(
        TREE "${SG_ROOT_DIR}"
        FILES ${TARGET_SOURCE_FILES} ${TARGET_HEADER_FILES}
    )
endfunction()
