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

set(CODE_INDEX "${CMAKE_CURRENT_BINARY_DIR}/index.json")

set(EXAMPLE_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/enums.h"
)

add_executable(example
  ${CMAKE_CURRENT_SOURCE_DIR}/enums.cpp
)

# Read enums.h and generate an index file
codegen_index_objects(INDEX "${CMAKE_CURRENT_BINARY_DIR}/index.json"
  HEADERS "${EXAMPLE_HEADERS}"
)

# This generates ostream operators for any enums that were found.
# I can just cruise on defaults for everything but target
# This generates ops.cpp and ops.h in the CMAKE_CURRENT_BINARY_DIR
# The function accepts parameters to put them elsewhere if you want to.
#
# I could see potentially running the OstreamOpsFromIndex as a on-off
# and including the generated code with your project too, in which
# case you can edit it if you really want to (the includes for headers
# in the generated source are currently a bit crap, but they work.)

codegen_ostream_operators(TARGET example
)

# I need to point the compiler at the binary dir to pick up ops.h
target_include_directories(example PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
