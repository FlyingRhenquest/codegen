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

# Preprocess our header file

# writes default index.json in current binary dir
codegen_index_objects(
  HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/DataObjects.h.in"
)

# reads default index.json in current binary dir
#
# It's probably not a good idea to autogenerate stuff to
# your current source dir, I'll have to consider the process
# here a bit and see if I can come up with something that works
# reasonably well in all cases.
codegen_generate_methods(
  SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/DataObjects.h.in"
  DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/DataObjects.h"  
)

add_executable(Demo
  "${CMAKE_CURRENT_SOURCE_DIR}/ExerciseDataObjects.cpp"
)

# You get a lot of warnings from the attributes unless you disable
# attributes
target_compile_options(Demo PRIVATE -Wno-attributes)
