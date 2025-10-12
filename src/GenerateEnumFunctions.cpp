/**
 * Copyright 2025 Bruce Ide
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * This is a simple C++ application that searches a C++ file for enum
 * declarations and outputs a header file and a C++ file with ostream
 * operators and to_string functions for that enum.  This allows you
 * to output an enum identifier with << and have your program output
 * the identifier name rather than a number.
 *
 * Command line flags:
 *
 * -i C++ Filename - input file
 * -c Output .cpp file
 * -h Output .cpp header
 *
 * Error handling is more or less non-existent. Make sure your enum code
 * compiles prior to feeding it to this application.
 */

#include <boost/program_options.hpp>
#include <fr/codegen/data.h>
#include <fr/codegen/parser.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>


// generateHeader takes your enum driver, header stream and the name of your enum source file so it can include it
// and generates a header with function signatures

void generateHeader(fr::codegen::EnumDriver &enums, std::ofstream& stream, const std::string& enumSource) {
  stream << "/* This is generated code. Do not edit. Unless you really want to. */" << std::endl;
  stream << "#pragma once" << std::endl;
  stream << "#include <string>" << std::endl;
  stream << "#include <iostream>" << std::endl;
  stream << "#include <" << enumSource << ">" << std::endl << std::endl;
  for (auto it = enums.enums.begin(); it != enums.enums.end(); ++it) {
    // it->first contains the fully qualified enum name
    stream << "std::string to_string(const " << it->first << "& value); // Converts enum to a string representation" << std::endl;
    stream << "std::ostream& operator<<(std::ostream& stream, const " << it->first << "& value);" << std::endl;
  }
}

// generateSource takes your enum driver, source stream and the name of the enum header file and
// outputs the source for to_string and ostream operators

// TODO: Generated code is kind of awful right now. De-awfulfy when I get a moment

void generateSource(fr::codegen::EnumDriver &enums, std::ofstream& stream, const std::string& myHeader) {
  stream << "/* This is generated code. Do not edit. Unless you really want to. */" << std::endl;
  stream << "#include <" << myHeader << ">" << std::endl << std::endl;
  
  for (auto it = enums.enums.begin(); it != enums.enums.end(); ++it) {
    bool isClassEnum = it->second.isClassEnum;
    
    stream << "std::string to_string(const " << it->first << "& value) {" << std::endl;
    stream << " switch(value) {" << std::endl;

    for (auto idIt = it->second.identifiers.begin(); idIt != it->second.identifiers.end(); ++idIt) {
      stream << "   case ";
      if (isClassEnum) {
	stream << it->first << "::" << *idIt << ":";       
      } else {
	auto enumNamespace = it->second.enumNamespace();
	if (enumNamespace.size()) {
	  stream << enumNamespace << "::";
	}
	stream << *idIt << ":";
      }
      if (isClassEnum) {
	stream << std::endl << "      return \"" << it->first << "::" << *idIt << "\";" << std::endl;
      } else {
	stream << std::endl << "      return \"" << *idIt << "\";" << std::endl;
      }
    }
    stream << "   }" << std::endl << " return \"UNKNOWN VALUE\";" << std::endl << "}" << std::endl << std::endl;

    stream << "std::ostream& operator<<(std::ostream& stream, const " << it->first << " &value) { " << std::endl;
    stream << " switch(value) {" << std::endl;
    for (auto idIt = it->second.identifiers.begin(); idIt != it->second.identifiers.end(); ++idIt) {
      if (isClassEnum) {
	stream << "    case " << it->first << "::" << *idIt << ":" << std::endl;
      } else {	
	stream << "    case ";
	auto enumNamespace = it->second.enumNamespace();
	if (enumNamespace.size()) {
	  stream << enumNamespace << "::";
	}
	stream << *idIt << ":" << std::endl;
      }
      stream << "      stream << \"";
      if (isClassEnum) {
	stream << it->first << "::" << *idIt << "\"";
      } else {
	auto enumNamespace = it->second.enumNamespace();
	if (enumNamespace.size()) {
	  stream << enumNamespace << "::";
	}
	stream << *idIt << "\"";
      }
      stream << ";";
      stream  << std::endl << "      break;" << std::endl;
    }
    stream << "    default: " << std::endl;
    stream << "      stream << \"UNKNOWN VALUE\";" << std::endl;
    stream << "   }" << std::endl;
    stream << " return stream;" << std::endl;
    stream << "}" << std::endl << std::endl;
  }
}

int main(int argc, char* argv[]) {
  std::string inputFile;
  std::string outputCpp;
  std::string outputHeader;

  // Command line flag handling

  boost::program_options::options_description desc("Options");
  // Force all 3 options to be mandatory with ->required()
  desc.add_options()
    ("input,i", boost::program_options::value<std::string>(&inputFile)->required(), "Input file with enum declarations")
    ("cpp,c", boost::program_options::value<std::string>(&outputCpp)->required(), "Output .cpp file")
    ("header,h", boost::program_options::value<std::string>(&outputHeader)->required(), "Output header file")
    ;

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::EnumDriver enums;
  std::string result;

  std::ifstream inputStream(inputFile);
  std::ofstream sourceStream(outputCpp);
  std::ofstream headerStream(outputHeader);
  std::stringstream input;
  input << inputStream.rdbuf();

  // Register enum driver with parser
  enums.regParser(parser);
  bool parseSuccess = parser.parse(input.view().begin(), input.view().end(), result);
  if (parseSuccess) {
    generateHeader(enums, headerStream, inputFile);
    generateSource(enums, sourceStream, outputHeader);
  } else {
    std::cout << "Parse failed" << std::endl;
  }
}
