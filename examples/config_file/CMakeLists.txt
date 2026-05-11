cmake_minimum_required(VERSION 3.25)

project(ConfigExample)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(ConfigExample
  NB_STATIC STABLE_ABI FREE_THREADED
  ${CMAKE_CURRENT_SOURCE_DIR}/PythonApi.cpp
)

target_include_directories(ConfigExample PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

