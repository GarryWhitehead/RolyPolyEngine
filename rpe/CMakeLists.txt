
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(RPE STATIC)

target_include_directories(
    RPE 
    PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    
    PRIVATE
    ${glfw3_INCLUDE_DIRS}
    ${VulkanHeaders_INCLUDE_DIRS}
)

set (public_headers 
    include/rpe/engine.h)

set (src_files
    src/private_engine.c
    src/render_graph/render_graph.c
    src/render_graph/render_pass_node.c
    src/render_graph/resources.c
    src/render_graph/resource_node.c
    src/render_graph/rendergraph_resource.c
    src/render_graph/dependency_graph.c
    src/render_graph/backboard.c
)

set (hdr_files
    src/private_engine.h
    src/render_graph/render_graph.h
    src/render_graph/render_pass_node.h
    src/render_graph/resources.h
    src/render_graph/resource_node.h
    src/render_graph/rendergraph_resource.h
    src/render_graph/render_graph_handle.h
    src/render_graph/render_graph_pass.h
    src/render_graph/dependency_graph.h
    src/render_graph/backboard.h
)

target_sources(
    RPE
    PUBLIC
    ${public_headers}
    PRIVATE
    ${src_files}
    ${hdr_files}
)

target_link_libraries(
    RPE
    PUBLIC
    VulkanApi
    UtilityLib
    log.c::log.c
)

target_compile_definitions(
    RPE
    PRIVATE
    RPE_SHADER_DIRECTORY="$RPE_SHADER_DIRECTORY}"
    RPE_VERBOSE_OUTPUT=${VERBOSE_OUTPUT}
    "$<$<PLATFORM_ID:Windows>:_USE_MATH_DEFINES;NOMINMAX;_CRT_SECURE_NO_WARNINGS>"  
)

# add common compile flags
rpe_add_compiler_flags(TARGET RPE)

# group source and header files
rpe_source_group(
    TARGET RPE
    ROOT_DIR "${RPE_ROOT_DIRECTORY}/rpe"
)

if (BUILD_TESTS)

    set (test_srcs
        test/test_main.c
        test/test_render_graph.c
    )

    add_executable(RpeTest ${test_srcs})
    target_link_libraries(RpeTest PRIVATE unity::unity RPE VulkanApi)
    set_target_properties(RpeTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RPE_TEST_DIRECTORY})
    rpe_add_compiler_flags(TARGET RpeTest)

    add_test(
            NAME RpeTest
            COMMAND RpeTest
            WORKING_DIRECTORY ${RPE_TEST_DIRECTORY}
    )

endif()
