import os
import platform
import sys

import sysconfig
from setuptools import setup
from setuptools.extension import Extension



class get_pybind_include(object):
    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


def include_dir_files(folder):
    from os import walk
    files = []
    for (dirpath, _, filenames) in walk(folder):
        for fn in filenames:
            if os.path.splitext(fn)[1] in {'.h', '.cc', '.cpp', '.hpp'}:
                files.append(os.path.join(dirpath, fn))
    return files



setup(
    name='spdlog',
    version='1.0.4',
    author='Gergely Bod',
    author_email='bodgergely@hotmail.com',
    description='python wrapper around C++ spdlog logging library (https://github.com/bodgergely/pyspdlog)',
    license='MIT',
    long_description='python wrapper (https://github.com/bodgergely/pyspdlog) around C++ spdlog (http://github.com/gabime/spdlog.git) logging library.',
    setup_requires=['pytest-runner'],
    install_requires=['pybind11>=2.2'],
    tests_require=['pytest'],
    data_files=include_dir_files('spdlog'),
    ext_modules=[
        Extension(
            'spdlog',
            ['src/pyspdlog.cpp'],
            include_dirs=[
                'spdlog/include/',
                get_pybind_include(),
                get_pybind_include(user=True)
            ],
            libraries=['stdc++'],
            extra_compile_args=["-std=c++11", "-v"],
            language='c++11'
        )
    ],
    zip_safe=False,
)

