
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(AppLib STATIC)

target_include_directories(
    AppLib
    PUBLIC
    ${RPE_UTILITY_ROOT_PATH}
)

target_link_libraries(
    AppLib
    PUBLIC
    UtilityLib
    VulkanApi
    RPE
    stb::stb
    KTX::ktx
    log.c::log.c
    nuklear::nuklear
    Tracy::TracyClient
    ${glfw3_LIBRARIES}
)

set(util_defs 
    RPE_SHADER_DIRECTORY="${RPE_SHADER_DIRECTORY}" 
    RPE_ASSETS_DIRECTORY="${RPE_ASSETS_DIRECTORY}"
)
if (MSVC)
    list(APPEND util_defs "_USE_MATH_DEFINES=1 NOMINMAX=1 _CRT_SECURE_NO_WARNINGS=1")
endif()

target_compile_definitions(
    AppLib
    PRIVATE
    ${util_defs}
)

target_sources(
    AppLib
    PRIVATE
    window.c
    window.h
    app.c
    app.h
    camera_view.c
    camera_view.h
    ibl_helper.c
    ibl_helper.h
    nk_helper.c
    nk_helper.h
)

# add common compiler flags
rpe_add_compiler_flags(TARGET AppLib)

# group source and header files
rpe_source_group(
    TARGET AppLib
    ROOT_DIR ${RPE_UTILITY_ROOT_PATH}
)

