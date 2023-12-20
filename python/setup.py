# -*- coding: utf-8 -*-
import configparser
import os
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name):
        Extension.__init__(self, name, sources=[])


class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        build_directory = os.path.abspath(self.build_temp)

        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + build_directory,
            "-DPYTHON_EXECUTABLE=" + sys.executable,
            "-DBUILD_CLI=OFF",
            "-DBUILD_TEST=OFF",
            "-DBUILD_DOC=OFF",
        ]

        self.config = "Debug" if self.debug else "Release"
        build_args = ["--config", self.config]

        cmake_args += ["-DCMAKE_BUILD_TYPE=" + self.config]

        self.build_args = build_args

        env = os.environ.copy()
        env["CXXFLAGS"] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get("CXXFLAGS", ""), self.distribution.get_version()
        )  # type: ignore
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        # CMakeLists.txt is in the parent directory of this setup.py file
        cmake_list_dir = os.path.abspath(
            os.path.join(os.path.dirname(__file__), os.pardir)
        )
        print("-" * 10, "Running CMake prepare", "-" * 40)
        subprocess.check_call(
            ["cmake", cmake_list_dir] + cmake_args, cwd=self.build_temp, env=env
        )

        print("-" * 10, "Building extensions", "-" * 40)
        cmake_cmd = ["cmake", "--build", ".", "-j 2"] + self.build_args
        subprocess.check_call(cmake_cmd, cwd=self.build_temp)

        # Move from build temp to final position
        for ext in self.extensions:
            self.move_output(ext)

    def move_output(self, ext):
        build_temp = Path(self.build_temp).resolve()
        dest_path = Path(self.get_ext_fullpath(ext.name)).resolve()

        if (build_temp / self.config).exists():
            build_temp = build_temp / self.config

        source_path = build_temp / self.get_ext_filename(ext.name)
        dest_directory = dest_path.parents[0]
        dest_directory.mkdir(parents=True, exist_ok=True)
        self.copy_file(str(source_path), str(dest_path))


def find_version():
    version_script = os.path.abspath(
        os.path.join(
            os.path.dirname(__file__), os.pardir, "cmake", "VersionSource.cmake"
        )
    )
    version = (
        subprocess.run(["cmake", "-P", version_script], capture_output=True, text=True)
        .stderr.split(":")[1]
        .strip()
    )
    print(f"Version: {version}")
    return version


# The information here can also be placed in setup.cfg - better separation of
# logic and declaration, and simpler if you include description/version in a file.

# For now, just parse from config file
config = configparser.ConfigParser()
config.read("project.ini")

project = config["base"]["project"]
copyright = config["base"]["copyright"]
author = config["base"]["author"]
url = config["base"]["url"]
title = config["base"]["title"]
description = config["base"]["description"]
tag = config["base"]["tag"]
version = find_version()

setup(
    name=project,
    version=version,
    author=author,
    url=url,
    author_email="",
    description=description,
    packages=["exactextract"],
    package_dir={"exactextract": "src/exactextract"},
    ext_modules=[CMakeExtension("_exactextract")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=">=3.7",
    install_requires=["pybind11>=2.2"],
)
