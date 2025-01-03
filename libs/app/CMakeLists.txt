
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
    ${glfw3_LIBRARIES}
)

target_compile_definitions(
    AppLib
    PRIVATE
    RPE_SHADER_DIRECTORY="${RPE_SHADER_DIRECTORY}"
    RPE_ASSETS_DIRECTORY="${RPE_ASSETS_DIRECTORY}"
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
)

# add common compiler flags
rpe_add_compiler_flags(TARGET AppLib)

# group source and header files
rpe_source_group(
    TARGET AppLib
    ROOT_DIR ${RPE_UTILITY_ROOT_PATH}
)
