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

#include <iostream>
#include <string>
#include <enumsOstreamOperators.h>
#include <vector>

int main(int argc, char *argv[]) {

  std::cout << "Some colors: " << std::endl;
  std::vector<Colors> colors{red, green, blue};
  for (auto i : colors) {
    std::cout << i << std::endl;
  }

  std::cout << "Some animals: " << std::endl;
  std::vector<animals::Animals> someAnimals {animals::Animals::dog, animals::Animals::cat, animals::Animals::llama};
  for (auto i : someAnimals) {
    std::cout << i << std::endl;
  }
  std::cout << "Some trees:" << std::endl;
  using namespace foo::bar;
  std::vector<Trees> trees { Trees::theLarch, Trees::larch, Trees::Larch, Trees::theHorseChestnut};
  for (auto i : trees) {
    std::cout << i << std::endl;
  }
}
