import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake


class Conan(ConanFile):
    name = "hycan"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("xtr/2.1.2")
        self.requires("libnl/3.9.0")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
