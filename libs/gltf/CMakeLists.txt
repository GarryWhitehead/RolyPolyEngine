
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/library.cmake")
include ("${RPE_CMAKE_INCLUDE_DIRECTORY}/targets.cmake")

add_library(GltfParser STATIC)

target_include_directories(
    GltfParser
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${VulkanHeaders_INCLUDE_DIRS}
)

target_link_libraries(
    GltfParser
    PUBLIC
    jsmn::jsmn
    cgltf::cgltf
    UtilityLib
    stb::stb
    KTX::ktx
    log.c::log.c
    RPE
)

target_sources(
    GltfParser
    PUBLIC 
    include/gltf/gltf_asset.h
    include/gltf/resource_loader.h
    include/gltf/gltf_loader.h

    PRIVATE
    src/gltf_loader.c
    src/skin_instance.c
    src/ktx_loader.c
    src/stb_loader.c
    src/resource_loader.c

    src/skin_instance.h
    src/ktx_loader.h
    src/stb_loader.h
    src/resource_loader.h
)

# add common compiler flags
rpe_add_compiler_flags(TARGET GltfParser)

# group source and header files
rpe_source_group(
    TARGET GltfParser
    ROOT_DIR ${RPE_UTILITY_ROOT_PATH}
)
