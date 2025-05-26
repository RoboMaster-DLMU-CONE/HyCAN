import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy


class HyCANRecipe(ConanFile):
    name = "HyCAN"
    version = "0.1.0"  # Set your library's version
    license = "BSD-3-Clause"  # Specify your project's license
    author = "MoonFeather moonfeather120@outlook.com"  # Your name and email
    url = "https://github.com/RoboMaster-DLMU-CONE/HyCAN"  # URL to your project
    description = "Modern high-performance Linux C++ CAN communication protocol library"
    topics = ("canbus", "linux", "c++23", "network")
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*", "example/*", "README.md"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("xtr/2.1.2")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "23"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(self, "README.md", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.libs = ["HyCAN"]
