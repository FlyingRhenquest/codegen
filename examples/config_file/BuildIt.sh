#!/bin/bash
#
# Runs all the commands to generate the cereal archive functions
# and python API for the Config example
#
# Assumes you've installed the various Generate* programs build
# by the top level project, or have them in your path somewhere.

# Generates the Index that the generators use
IndexCode -h Config.h.in -o index.json

# Reads the Config.h.in file and generates a Config.h with the
# Cereal Load/Save functions in place
GenerateFunctions -h Config.h.in -i index.json -o Config.h

# Reads the PythonApi.cpp.in template file and generates the
# Nanobind Python API
GeneratePythonApi -s PythonApi.cpp.in -i index.json -o PythonApi.cpp

mkdir build
cd build
cmake ..
make

echo "If that didn't give you any errors you should be able to cd to build,"
echo "Run python and import ConfigExample (You may need '.' in your python"
echo "path.) Then try declaring a variable as a ConfigExample.config and"
echo "calling its to_json() method."
