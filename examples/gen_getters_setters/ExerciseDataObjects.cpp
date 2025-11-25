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
 */

#include "DataObjects.h"
#include <iostream>
#include <sstream>

using namespace fr::codegen;

int main(int argc, char *argv[]) {
  IndividualSerializableAddress address;
  address.set_name("Emily Who");
  address.set_address1("1 Who Street");
  address.set_city("Whoville");
  address.set_state("Whosizona");
  address.set_zip("12345");

  std::stringstream addressJson;
  {
    cereal::JSONOutputArchive archive(addressJson);
    archive(address);
  }
  std::cout << "Address JSON: " << addressJson.str() << std::endl;

  AnimalSays wat;
  wat.says = "wat?!";
  wat.animal = "Wombat";

  std::stringstream watJson;
  {
    cereal::JSONOutputArchive archive(watJson);
    archive(wat);
  }

  std::cout << "Animal JSON: " << watJson.str() << std::endl;

  WeirdClass weird("Wombat,Aardvark");
  weird.set_firstName("Throatwobbler");
  weird.set_lastName("Mangrove");
  weird.setName();
  weird.set_eieio("wat?!");
  std::cout << "weird tostring: " << weird.to_string() << std::endl;
  
}
