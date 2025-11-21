cmake_minimum_required(VERSION 3.25)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest REQUIRED)
include(GoogleTest)

if (NOT TARGET FR::codegen)
  find_package(FRcodegen CONFIG REQUIRED)
endif()

set(TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/comments.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scope.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ParserSignalTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Data.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/templates.cpp
)

add_executable(CodegenTests
  ${TEST_SRC}
)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
  target_compile_definitions(CodegenTests PUBLIC BOOST_SPIRIT_DEBUG BOOST_SPIRIT_X3_DEBUG)
endif()

target_include_directories(CodegenTests PUBLIC
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(CodegenTests PUBLIC
  GTest::GTest
  GTest::Main
  FR::codegen
)
