import os
import platform
import sys

import sysconfig
from setuptools import setup
from setuptools.extension import Extension
from distutils.command.install_headers import install_headers

def is_posix():
    return platform.os.name == "posix"

def link_libs():
    libs = []
    if is_posix():
        libs.append("stdc++")
    return libs

class get_pybind_include(object):
    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


def include_dir_files(folder):
    """Find all C++ header files in folder"""
    from os import walk
    files = []
    for (dirpath, _, filenames) in walk(folder):
        for fn in filenames:
            if os.path.splitext(fn)[1] in {'.h', '.hpp'}:
                files.append(os.path.join(dirpath, fn))
    return files

class install_headers_subdir(install_headers):
    """Install headers and keep subfolder structure"""
    def run(self):
        headers = self.distribution.headers or []
        for header in headers:
            submod_dir = os.path.dirname(os.path.relpath(header, 'spdlog/include/spdlog'))
            install_dir = os.path.join(self.install_dir, submod_dir)
            self.mkpath(install_dir)
            (out, _) = self.copy_file(header, install_dir)
            self.outfiles.append(out)

setup(
    name='spdlog',
    version='2.0.4',
    author='Gergely Bod',
    author_email='bodgergely@hotmail.com',
    description='python wrapper around C++ spdlog logging library (https://github.com/bodgergely/spdlog-python)',
    license='MIT',
    long_description='python wrapper (https://github.com/bodgergely/spdlog-python) around C++ spdlog (http://github.com/gabime/spdlog.git) logging library.',
    setup_requires=['pytest-runner'],
    install_requires=['pybind11>=2.2'],
    tests_require=['pytest'],
    ext_modules=[
        Extension(
            'spdlog',
            ['src/pyspdlog.cpp'],
            include_dirs=[
                'spdlog/include/',
                get_pybind_include(),
                get_pybind_include(user=True)
            ],
            libraries=link_libs(),
            extra_compile_args=["-std=c++11", "-v"],
            language='c++11'
        )
    ],
    headers=include_dir_files('spdlog/include/spdlog'),
    cmdclass={'install_headers': install_headers_subdir},
    zip_safe=False,
)
