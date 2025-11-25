cmake_minimum_required(VERSION 3.25)

project(FRCodegen)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.90 REQUIRED CONFIG REQUIRED COMPONENTS program_options)

option(BUILD_TESTS ON)

set(HEADER_DIR "include/fr/codegen")
set(INTERFACE_HEADERS
  "${HEADER_DIR}/parser.h"
  "${HEADER_DIR}/data.h"
)

add_library(frcodegen INTERFACE)

set_target_properties(frcodegen PROPERTIES
  FRAMEWORK TRUE
  PUBLIC_HEADER "${INTERFACE_HEADERS}"
)

target_include_directories(frcodegen
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_library(FR::codegen ALIAS frcodegen)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()

add_executable(GenerateEnumFunctions
  "${CMAKE_CURRENT_SOURCE_DIR}/src/GenerateEnumFunctions.cpp"
)

add_executable(IndexCode
  "${CMAKE_CURRENT_SOURCE_DIR}/src/IndexCode.cpp"
)

add_executable(OstreamOpsFromIndex
  "${CMAKE_CURRENT_SOURCE_DIR}/src/OstreamOpsFromIndex.cpp"
)

add_executable(GenerateFunctions
  "${CMAKE_CURRENT_SOURCE_DIR}/src/GenerateFunctions.cpp"
)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
  target_compile_definitions(GenerateEnumFunctions PUBLIC BOOST_SPIRIT_DEBUG BOOST_SPIRIT_X3_DEBUG)
  target_compile_definitions(IndexCode PUBLIC BOOST_SPIRIT_DEBUG BOOST_SPIRIT_X3_DEBUG)
  target_compile_definitions(OstreamOpsFromIndex PUBLIC BOOST_SPIRIT_DEBUG BOOST_SPIRIT_X3_DEBUG)
  target_compile_definitions(GenerateFunctions PUBLIC BOOST_SPIRIT_DEBUG BOOST_SPIRIT_X3_DEBUG)
endif()

target_link_libraries(GenerateEnumFunctions PUBLIC
  FR::codegen
  Boost::program_options
)

target_link_libraries(IndexCode PUBLIC
  FR::codegen
  Boost::program_options
)

target_link_libraries(OstreamOpsFromIndex PUBLIC
  FR::codegen
  Boost::program_options
)

target_link_libraries(GenerateFunctions PUBLIC
  FR::codegen
  Boost::program_options
)

# Install Instrumentation
set(frcodegen_VERSION_MAJOR 0)
set(frcodegen_VERSION_MINOR 2)
set(frcodegen_VERSION ${frcodegen_VERSION_MAJOR}.${frcodegen_VERSION_MINOR})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/FRcodegenConfigVersion.cmake"
  VERSION "${frcodegen_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/FRcodegenConfig.cmake"
  INSTALL_DESTINATION lib/cmake/FRcodegen
)

# Install Interface Library
install(TARGETS frcodegen GenerateEnumFunctions IndexCode OstreamOpsFromIndex GenerateFunctions
  EXPORT frcodegen_export
  PUBLIC_HEADER DESTINATION include/fr/codegen
  INCLUDES DESTINATION include/fr/codegen
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)

install(EXPORT frcodegen_export
  FILE FRCodegenTargets.cmake
  NAMESPACE FR::
  DESTINATION lib/cmake/FRcodegen
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/FRcodegenConfigVersion.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/FRcodegenConfig.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CodegenFunctions.cmake"
  DESTINATION lib/cmake/FRcodegen
)
