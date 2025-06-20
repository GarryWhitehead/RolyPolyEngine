cmake_minimum_required(VERSION 3.16)

project(RolyPolyEngine LANGUAGES C CXX)

# IDE setup
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# set c standard versions to use throughout project
set(CMAKE_C_STANDARD                17)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)

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
set(RPE_ASSETS_DIRECTORY "${RPE_ROOT_DIRECTORY}/assets")
set(RPE_SHADER_SIPRV_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
set(RPE_CMAKE_INCLUDE_DIRECTORY "${RPE_ROOT_DIRECTORY}/cmake")
set(RPE_TEST_DIRECTORY "${CMAKE_BINARY_DIR}/Tests")
set(RPE_BENCHMARK_DIRECTORY "${CMAKE_BINARY_DIR}/Benchmarks")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-recover=address")

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
find_package(parg REQUIRED)
find_package(nuklear REQUIRED)
find_package(Tracy REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if (BUILD_TESTS)
    find_package(unity REQUIRED)
    enable_testing()
endif()


# add libraries: 
add_subdirectory(shaders)
add_subdirectory(libs)
add_subdirectory(vulkan-api)
add_subdirectory(rpe)
add_subdirectory(apps)
