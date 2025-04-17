
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

function(create_app)
    set(single_value_args APP_NAME)
    set(multi_value_args FILES)
    cmake_parse_arguments(
        "PARAM"
        ""
        "${single_value_args}"
        "${multi_value_args}"
        "${ARGN}"
    )

    if(NOT PARAM_APP_NAME)
        message(FATAL "No app name parameter specified.")
    endif()

    add_executable(${PARAM_APP_NAME} ${PARAM_FILES})
    target_include_directories(${PARAM_APP_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${PARAM_APP_NAME} PUBLIC RPE AppLib)
    set_target_properties(${PARAM_APP_NAME} PROPERTIES FOLDER Apps)
    target_compile_definitions(
        ${PARAM_APP_NAME}
        PRIVATE
        RPE_ASSETS_DIRECTORY="${RPE_ASSETS_DIRECTORY}"
    )

    rpe_add_compiler_flags(TARGET ${PARAM_APP_NAME})

    # group source and header files
    rpe_source_group(
        TARGET ${PARAM_APP_NAME}
        ROOT_DIR "${RPE_ROOT_DIRECTORY}/apps"
    )

endfunction()

create_app(APP_NAME hello_triangles FILES hello_triangle.c)
create_app(APP_NAME gltf_viewer FILES gltf_viewer.c)
create_app(APP_NAME cascade_shadow_test FILES cascade_shadow_test.c)

target_link_libraries(hello_triangles PUBLIC UtilityLib)
target_link_libraries(gltf_viewer PUBLIC UtilityLib GltfParser Threads::Threads parg::parg)
target_link_libraries(cascade_shadow_test PUBLIC UtilityLib GltfParser parg::parg)


