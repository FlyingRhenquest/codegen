cmake_minimum_required(VERSION 3.25)

project(enumsExample)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT TARGET FR::codegen)
  find_package(FRcodegen CONFIG REQUIRED)
endif()

find_program(generate_code NAMES GenerateEnumFunctions)

if (NOT generate_code)
  message(FATAL_ERROR "Could not find GenerateEnumFunctions. Please insure it's built and found somehwere I can find it")
endif()

execute_process(
  COMMAND
  ${generate_code} -i ${CMAKE_CURRENT_SOURCE_DIR}/enums.h -h ${CMAKE_CURRENT_BINARY_DIR}/enumsOstreamOperators.h -c ${CMAKE_CURRENT_BINARY_DIR}/enumOstreamOperators.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/enums.h
  COMMENT "Generating ostream operators"
)

set(EXAMPLE_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/enums.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/enumOstreamOperators.cpp"
)

add_executable(enum_example ${EXAMPLE_SRC})
target_include_directories(enum_example PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
