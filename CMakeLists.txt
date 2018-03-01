cmake_minimum_required(VERSION 2.8.12)
project(spdlog)

add_subdirectory(pybind11)
include_directories(spdlog/include)
pybind11_add_module(spdlog src/pyspdlog.cpp)
