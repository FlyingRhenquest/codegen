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
 * This program reads some header files specified on the command
 * line and outputs a JSON file of data about classes and enums
 * that discovered in those files.
 */

#include <boost/program_options.hpp>
#include <fr/codegen/data.h>
#include <fr/codegen/drivers.h>
#include <fr/codegen/parser.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

void printHelp(boost::program_options::options_description &desc) {
  std::cout << "Usage: " << std::endl;
  std::cout << desc << std::endl << std::endl;
}

int main(int argc, char *argv[]) {

  std::vector<std::string> headers;
  std::string outputJson;
  
  boost::program_options::options_description desc("Options:");
  boost::program_options::variables_map vm;

  // We can add multiple headers and each one will just be pushed into
  // the vector
  
  desc.add_options()
    ("headers,h",
     boost::program_options::value<std::vector<std::string>>(&headers)->composing(),
     "Headers to process -- you can specify this option multiple times if you want to process more than one.")
    ("output,o",
     boost::program_options::value<std::string>(&outputJson),
     "JSON output file");

  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (!vm.count("headers") || !vm.count("output")) {
    printHelp(desc);
    exit(1);
  }

  std::map<std::string, std::shared_ptr<fr::codegen::EnumData>> enumMap;
  std::map<std::string, std::shared_ptr<fr::codegen::ClassData>> classMap;
  std::cout << "Parsing headers..." << std::endl;

  // Open, read and parse each header.
  for (const auto& header : headers) {
    std::string result;
    std::cout << "Parsing " << header << "... " << std::endl;
    std::ifstream inputStream(header);
    std::stringstream input;
    // Create new instances of drivers for each file
    fr::codegen::parser::ParserDriver parser;
    fr::codegen::EnumDriver enums;
    fr::codegen::ClassDriver classes;
    // Register enum and class drivers with parser
    enums.regParser(parser);
    classes.regParser(parser);
    // Set filename we're looking at
    enums.setCurrentFile(header);
    classes.setCurrentFile(header);
    // Subscribe to enum and class driver signals
    enums.enumAvailable.connect([&enumMap](const std::string& key, const fr::codegen::EnumData& data) {
      std::cout << "Adding enum " << key << std::endl;
      enumMap[key] = std::make_shared<fr::codegen::EnumData>(data);
    });
    classes.classAvailable.connect([&classMap](const std::string &key, const fr::codegen::ClassData& data) {
      std::cout << "Adding class " << key << std::endl;
      classMap[key] = std::make_shared<fr::codegen::ClassData>(data);
    });      
    input << inputStream.rdbuf();
    bool parseSuccess = parser.parse(input.view().begin(), input.view().end(), result);
    std::cout << (parseSuccess ? "Success" : "Failed" ) << std::endl;
  }
  std::cout << "Writing JSON..." << std::endl;
  std::ofstream json(outputJson);
  {
    cereal::JSONOutputArchive archive(json);
    archive(cereal::make_nvp("enums", enumMap));
    archive(cereal::make_nvp("classes", classMap));
  }
  json.close();
  std::cout << "Processing complete" << std::endl;
  
  exit(0);
  
}
