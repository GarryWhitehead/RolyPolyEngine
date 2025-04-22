from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import collect_libs

import os


class VkSceneEditor3dPackage(ConanFile):
    name = "roly-poly-engine"
    license = "MIT"
    author = "garry"
    url = ""
    description = "A Vulkan 3D graphics engine."
    topics = ("Vulkan", "graphics", "3D")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_validation_layers": [True, False],
        "build_tests" : [True, False],
        "verbose": [True, False]
    }
    default_options = {
        "shared": False, 
        "fPIC": True, 
        "with_validation_layers": True, 
        "build_tests": True, 
        "verbose": False,
        }

    generators = "CMakeDeps"
    _cmake = None

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.build_tests:
            self.options["unity"].fixture_extension = True

    def requirements(self):
        self.requires("vulkan-headers/1.3.236.0")
        self.requires("vulkan-loader/1.3.236.0")
        self.requires("vulkan-memory-allocator/3.0.1")
        self.requires("vulkan-validationlayers/1.3.236.0")
        self.requires("volk/1.3.268.0")
        self.requires("glfw/3.3.8")
        self.requires("glslang/1.3.236.0")
        self.requires("spirv-cross/1.3.236.0")
        self.requires("stb/cci.20240531")
        self.requires("cgltf/1.12")
        self.requires("jsmn/1.1.0")
        self.requires("log.c/cci.20200620")
        self.requires("ktx/4.0.0")
        self.requires("parg/1.0.3")
        self.requires("nuklear/4.12.0")

        if self.options.build_tests:
            self.requires("unity/2.6.0")

    def layout(self):
        self.folders.source = "./src"
        self.folders.build = os.getcwd()
        self.folders.generators = f"{self.folders.build}/Conan"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["WITH_VALIDATION_LAYERS"] = self.options.with_validation_layers
        tc.variables["BUILD_SHARED"] = self.options.shared
        tc.variables["VERBOSE_OUTPUT"] = self.options.verbose
        tc.variables["BUILD_TESTS"] = self.options.build_tests
        tc.generate()

    def build(self):
        self._cmake = CMake(self)
        self._cmake.configure()
        self._cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses")

        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.configure()
        self._cmake.install()

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
