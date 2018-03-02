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



setup(
    name='spdlog',
    version='1.0.0',
    author='Gergely Bod',
    author_email='bodgergely@hotmail.com',
    description='python wrapper around C++ spdlog logging library',
    license='MIT',
    long_description='',
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
            libraries=['stdc++'],
            extra_compile_args=["-std=c++11", "-v"],
            language='c++11'
        )
    ],
    zip_safe=False,
)

