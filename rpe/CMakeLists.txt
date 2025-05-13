
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
    include/rpe/engine.h
    include/rpe/object.h
    include/rpe/scene.h
    include/rpe/mesh.h
    include/rpe/material.h
    include/rpe/renderable_manager.h
    include/rpe/object_manager.h
    include/rpe/transform_manager.h
    include/rpe/light_manager.h
    include/rpe/camera.h
    include/rpe/renderer.h
    include/rpe/aabox.h
    include/rpe/ibl.h
    include/rpe/skybox.h
    include/rpe/settings.h
    include/rpe/shadow_manager.h
)

set (src_files
    src/engine.c
    src/material.c
    src/commands.c
    src/render_queue.c
    src/colour_pass.c
    src/light_pass.c
    src/shadow_pass.c
    src/scene.c
    src/frustum.c
    src/renderer.c
    src/compute.c
    src/camera.c
    src/ibl.c
    src/skybox.c
    src/vertex_buffer.c
    src/shadow_manager.c
    src/render_graph/render_graph.c
    src/render_graph/render_pass_node.c
    src/render_graph/resources.c
    src/render_graph/resource_node.c
    src/render_graph/rendergraph_resource.c
    src/render_graph/dependency_graph.c
    src/render_graph/backboard.c
    src/render_graph/graphviz.c
    src/managers/object_manager.c
    src/managers/renderable_manager.c
    src/managers/transform_manager.c
    src/managers/component_manager.c
    src/managers/light_manager.c
)

set (hdr_files
    src/material.h
    src/commands.h
    src/render_queue.h
    src/colour_pass.h
    src/light_pass.h
    src/shadow_pass.h
    src/scene.h
    src/engine.h
    src/frustum.h
    src/renderer.h
    src/compute.h
    src/camera.h
    src/ibl.h
    src/skybox.h
    src/vertex_buffer.h
    src/shadow_manager.h
    src/render_graph/render_graph.h
    src/render_graph/render_pass_node.h
    src/render_graph/resources.h
    src/render_graph/resource_node.h
    src/render_graph/rendergraph_resource.h
    src/render_graph/render_graph_handle.h
    src/render_graph/render_graph_pass.h
    src/render_graph/dependency_graph.h
    src/render_graph/backboard.h
    src/render_graph/graphviz.h
    src/managers/object_manager.h
    src/managers/renderable_manager.h
    src/managers/transform_manager.h
    src/managers/component_manager.h
    src/managers/light_manager.h
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
    stb::stb
    Threads::Threads
)

add_dependencies(RPE Shaders)

target_compile_definitions(
    RPE
    PRIVATE
    RPE_SHADER_DIRECTORY="${RPE_SHADER_DIRECTORY}"
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
        test/test_commands.c
        test/test_visibility.c
        test/test_compute.c
        test/vk_setup.h
        test/test_engine.c
    )

    add_executable(RpeTest ${test_srcs})
    target_link_libraries(RpeTest PRIVATE unity::unity RPE VulkanApi)
    set_target_properties(RpeTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RPE_TEST_DIRECTORY})
    rpe_add_compiler_flags(TARGET RpeTest)

    if (BUILD_GPU_TESTS)
        target_compile_definitions(
            RpeTest
            PUBLIC
            RPE_BUILD_GPU_TESTS=1
        )
    endif()

    add_test(
            NAME RpeTest
            COMMAND RpeTest
            WORKING_DIRECTORY ${RPE_TEST_DIRECTORY}
    )

endif()
