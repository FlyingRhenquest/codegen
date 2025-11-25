# Instrumentation Functions for CMake

#-----------------------------------------------------------------
# codegen_index_objects reads a set of C++ Headers and creates a
# JSON index of the structs, classes and enums that it finds.
#
# Arguments:
# HEADERS keyword followed by a list of headers
# INDEX Followed by the JSON file to write to
#
# INDEX is optional and will default to
# "${CMAKE_CURRENT_BINARY_DIR}/index.json"
#
# example:
# codegen_index_objects(INDEX classes.json HEADERS ${HEADER_LIST})
#-----------------------------------------------------------------

function(codegen_index_objects)

  # Parse Arguments
  
  set(INDEX_FILE "${CMAKE_CURRENT_BINARY_DIR}/index.json")
  set(HEADER_LIST "")
  set(options "")
  set(oneValueArgs "INDEX")
  set(multiValueArgs "HEADERS")
  cmake_parse_arguments(PARSE_ARGV 0 arg
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
  )

  # Process Arguments
  
  if (arg_INDEX)
    set(INDEX_FILE "${arg_INDEX}")
  endif()
  if (arg_HEADERS)
    set(HEADER_LIST "${arg_HEADERS}")
  else()
    message(FATAL_ERROR "No headers were provided to codegen_index_object")
  endif()

  # Generate Command Line
  find_program(INDEX_CODE IndexCode)
  set(COMMAND_LINE "${INDEX_CODE}")
  list(APPEND COMMAND_LINE "-o" "${INDEX_FILE}")
  foreach (HEADER_FILE IN LISTS HEADER_LIST)
    list(APPEND COMMAND_LINE "-h" "${HEADER_FILE}")
  endforeach()
  message(STATUS "Command line: ${COMMAND_LINE}")
  execute_process(
    COMMAND ${COMMAND_LINE}
  )
  
endfunction()

#--------------------------------------------------------------------
# codegen_ostream_operators geneates to_string functions and ostream
# operators found in a json index created by codegen_index_objects.
#
# Arguments:
# INDEX - Index to read (Will default to same index as
#         codegen_index_objects)
# HEADER - Header to generate (Defaults to
#         ${CMAKE_CURRENT_BINARY_DIR}/ops.h
# SOURCE - cpp file to generate (Defaults to
#         ${CMAKE_CURRENT_BINARY_DIR}/ops.cpp
# TARGET - Target to add CPP file to
# -------------------------------------------------------------------

function(codegen_ostream_operators)
  # defaults
  set(INDEX_FILE "${CMAKE_CURRENT_BINARY_DIR}/index.json")
  set(HEADER "${CMAKE_CURRENT_BINARY_DIR}/ops.h")
  set(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/ops.cpp")
  set(options "")
  set(oneValueArgs INDEX HEADER SOURCE TARGET)
  set(multiValueArgs "")
  # Parse args
  cmake_parse_arguments(PARSE_ARGV 0 arg
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
  )

  if (arg_INDEX)
    set(INDEX_FILE "${arg_INDEX}")
  endif()
  if (arg_HEADER)
    set(HEADER "${arg_HEADER}")
  endif()
  if (arg_SOURCE)
    set(SOURCE "${arg_SOURCE}")
  endif()
  if(arg_TARGET)
    set(TARGET ${arg_TARGET})
  endif()

  # generate command line
  find_program(OPS_GEN OstreamOpsFromIndex)
  set(COMMAND_LINE "${OPS_GEN}")
  list(APPEND COMMAND_LINE "-i" "${INDEX_FILE}")
  list(APPEND COMMAND_LINE "-h" "${HEADER}")
  list(APPEND COMMAND_LINE "-c" "${SOURCE}")
  message(STATUS "Command line: ${COMMAND_LINE}")
  execute_process(
    COMMAND ${COMMAND_LINE}
  )
  if (TARGET)
    target_sources(${TARGET} PRIVATE "${SOURCE}")
  endif()
  
endfunction()

#--------------------------------------------------------------------
# codegen_generate_methods generates getters, setters and cereal
# archive functions based on annotations in the source code. You
# can annotate class declarations with [[cereal]] prior to the
# "class" keyword to set all members in the class to be archived,
# or put [[cereal]] before each member you want to be serialized.
# You can also put [[get]] and [[set]] prior to each member you
# want to generate a getter or setter for. Any combination of these
# keywwords can be placed in one annotation and separated by
# commas like so: [[cereal,get,set]].
#
# This function requires an index of the code generated with
# IndexCode. It's designed to do line-by-line reading and
# writing from the source class, so it's a good idea to put
# your source file in a .h.in file and rewrite it to a .h file.
#
# Arguments:
# INDEX - Index json to read (will default to the same one
#         as codegen_index_objects)
# SOURCE - Source file to read
# DESTINATION - Destination file to write
#------------------------------------------------------------------
function(codegen_generate_methods)
  # defaults
  set(INDEX_FILE "${CMAKE_CURRENT_BINARY_DIR}/index.json")
  set(SOURCE_FILE "")
  set(DESTINATION_FILE "")
  # Set up options
  set(oneValueArgs INDEX SOURCE DESTINATION)
  set(multiValueArgs "")
  # Parse Args
  cmake_parse_arguments(PARSE_ARGV 0 arg
    "${options}" "${oneValueArgs}" "${multiValueArgs}"
  )

  if (arg_INDEX)
    set(INDEX_FILE "${arg_INDEX}")
  endif()
  
  if (arg_SOURCE)
    set(SOURCE_FILE "${arg_SOURCE}")
  else()
    message(FATAL_ERROR "You must specify a source file for codegen_generate_methods")
  endif()

  if (arg_DESTINATION)
    set(DESTINATION_FILE "${arg_DESTINATION}")
  else()
    message(FATAL_ERROR "You must specify a destiatnion file for codegen_generate_methods")
  endif()

  # Generate Command Line
  find_program(OPS_GEN "GenerateFunctions")
  set(COMMAND_LINE "${OPS_GEN}")
  list(APPEND COMMAND_LINE "-i" "${INDEX_FILE}")
  list(APPEND COMMAND_LINE "-h" "${SOURCE_FILE}")
  list(APPEND COMMAND_LINE "-o" "${DESTINATION_FILE}")
  message(STATUS "codegen_generate_methods: Command Line: ${COMMAND_LINE}")
  execute_process(COMMAND ${COMMAND_LINE})
endfunction()
