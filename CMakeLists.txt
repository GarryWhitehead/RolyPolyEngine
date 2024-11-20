cmake_minimum_required(VERSION 3.16)

project(RolyPolyEngine LANGUAGES C CXX)

# IDE setup
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# set c standard versions to use throughout project
set(CMAKE_C_STANDARD                17)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              OFF)

# configuration or build type setup
set(configuration_types Debug Release RelWithDebInfo MinSizeRel CACHE STRING "Available configuration types." FORCE)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES ${configuration_types})
# If the user defines the build_type via the cmake cmd arg (or GUI), this value takes precedence
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# path to the asset directory. This defaults to the assets directory at
# the root of yave but can be changed by the user
if(NOT RPE_ASSETS_DIRECTORY)
    set(YAVE_ASSETS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" CACHE STRING "Path to the library assets.")
endif()

# global variables
set(RPE_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(RPE_SHADER_DIRECTORY "${RPE_ROOT_DIRECTORY}/shaders")
set(RPE_SHADER_SIPRV_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
set(RPE_CMAKE_INCLUDE_DIRECTORY "${RPE_ROOT_DIRECTORY}/cmake")
set(RPE_TEST_DIRECTORY "${CMAKE_BINARY_DIR}/Tests")

# find vulkan (hopefully!)
find_package(Vulkan REQUIRED)

# required dependencies for all libraries
find_package(glslang REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)
find_package(spirv-cross REQUIRED)
find_package(vulkan-validationlayers REQUIRED)
find_package(glfw3 REQUIRED)
find_package(cgltf REQUIRED)
find_package(jsmn REQUIRED)
find_package(stb REQUIRED)
find_package(log.c REQUIRED)
find_package(Ktx REQUIRED)
find_package(volk REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if (BUILD_TESTS)
    find_package(unity REQUIRED)
    enable_testing()
endif()

# Try and find the render-doc API library.
if (NOT RPE_RENDERDOC_ROOT_PATH)
    if (WIN32)
        set (RPE_RENDERDOC_ROOT_PATH "C:/renderdoc")
    else()
        set (RPE_RENDERDOC_ROOT_PATH "/usr/local/renderdoc")
    endif()
endif()

message("Looking for RenderDoc API libraries in ${RPE_RENDERDOC_ROOT_PATH}...")

if (WIN32)
    find_file(RPE_RENDERDOC_LIB_PATH renderdoc.dll 
        PATHS "${RPE_RENDERDOC_ROOT_PATH}/lib" "${RPE_RENDERDOC_ROOT_PATH}"
    )
else()
    find_library(RPE_RENDERDOC_LIB_PATH renderdoc
        PATHS "${RPE_RENDERDOC_ROOT_PATH}/lib" "${RPE_RENDERDOC_ROOT_PATH}"
    )
endif()

if (NOT RPE_RENDERDOC_LIB_PATH)
    message("RenderDoc API library not found.")
    set(RPE_FOUND_RENDERDOC 0)
else()
    set(RPE_FOUND_RENDERDOC 1)
endif()

# add libraries: 
add_subdirectory(shaders)
add_subdirectory(libs)
add_subdirectory(vulkan-api)
add_subdirectory(rpe)
add_subdirectory(apps)
