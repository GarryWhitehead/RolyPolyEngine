
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(UtilityLib STATIC)

target_include_directories(
    UtilityLib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(UtilityLib PUBLIC log.c::log.c -lm)

set_target_properties(UtilityLib PROPERTIES LINKER_LANGUAGE C)

if (NOT RPE_DEBUG_ARENA)
    set(RPE_DEBUG_ARENA 0)
endif()

target_compile_definitions(UtilityLib PUBLIC ENABLE_DEBUG_ARENA=${RPE_DEBUG_ARENA})

target_sources(
    UtilityLib
    PUBLIC
    src/utility/compiler.h
    src/utility/aligned_alloc.h
    src/utility/array.h
    src/utility/array.c
    src/utility/vector.h
    src/utility/vector.c
    src/utility/arena.c
    src/utility/arena.h
    src/utility/maths.h
    src/utility/array_utility.c
    src/utility/array_utility.h
    src/utility/hash_set.c
    src/utility/hash_set.h
    src/utility/job_queue.c
    src/utility/job_queue.h
    src/utility/random.h
    src/utility/work_stealing_queue.c
    src/utility/work_stealing_queue.h
    src/utility/hash.h
    src/utility/string.c
    src/utility/string.h
    src/utility/filesystem.h
    src/utility/filesystem.c
)

# add common compiler flags
rpe_add_compiler_flags(TARGET UtilityLib)

# group source and header files
rpe_source_group(
    TARGET UtilityLib
    ROOT_DIR ${RPE_UTILITY_ROOT_PATH}
)

if (BUILD_TESTS)

    set (test_srcs
        test/test_array.c
        test/test_vector.c
        test/test_arena.c
        test/test_hash_set.c
        test/test_job_queue.c
        test/test_work_stealing_queue.c
        test/test_math.c
        test/test_main.c
        test/test_string.c
        test/test_filesystem.c
    )

    add_executable(UtilityTest ${test_srcs})
    target_link_libraries(UtilityTest PRIVATE unity::unity UtilityLib)
    set_target_properties(UtilityTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RPE_TEST_DIRECTORY})
    set_target_properties(UtilityTest PROPERTIES LINKER_LANGUAGE C)
    rpe_add_compiler_flags(TARGET UtilityTest)

    add_test(
            NAME UtilityTest
            COMMAND UtilityTest
            WORKING_DIRECTORY ${RPE_TEST_DIRECTORY}
    )

endif()