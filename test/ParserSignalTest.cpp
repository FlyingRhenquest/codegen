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
 * Tests that various signals from the ParserDriver work as expected
 */

#include <gtest/gtest.h>
#include <fr/codegen/parser.h>
#include <vector>

// Check enum and enum class parsing (basic)

TEST(ParserSignals, Enums) {
  std::string enumName;
  std::string enumBasic = "enum Colors { red, green, blue };";
  std::string classEnumBasic = "enum class Colors { red, green, blue };";
  std::vector<std::string> colors;
  
  fr::codegen::parser::ParserDriver parser;
  parser.enumPush.connect([&enumName](const std::string &name, int /* notused */) {
    enumName = name;
  });

  parser.enumIdentifier.connect([&colors](const std::string& name,
					  const std::string& identifier) {
    colors.push_back(identifier);
  });

  std::string result;
  // Check that basic enum works
  ASSERT_TRUE(parser.parse(enumBasic.begin(), enumBasic.end(), result));
  ASSERT_EQ(enumName, "Colors");
  ASSERT_EQ(colors.size(), 3);
  ASSERT_EQ(colors[0], "red");
  ASSERT_EQ(colors[1], "green");
  ASSERT_EQ(colors[2], "blue");

  // Setup for the enum class version
  enumName = "";
  colors.clear();

  // The color handler is still connected so I just need a enum class
  // handler

  bool enumClass = false; // A little extra validation that we took the enum class path and not the enum path

  parser.enumClassPush.connect([&enumName, &enumClass](const std::string& name, int /* Notused */) {
    enumName = name;
    enumClass = true;
  });

  ASSERT_TRUE(parser.parse(classEnumBasic.begin(), classEnumBasic.end(), result));
  ASSERT_EQ(enumName, "Colors");
  ASSERT_TRUE(enumClass);
  ASSERT_EQ(colors.size(), 3);
  ASSERT_EQ(colors[0], "red");
  ASSERT_EQ(colors[1], "green");
  ASSERT_EQ(colors[2], "blue");  
}

// Try a class enum with namepace. For this test we're not clearing the namespace vector
// out when the namespace goes out of scope.

TEST(ParserSignals, NamespacedEnum) {
  std::string enumName;
  std::vector<std::string> enums{
    "namespace foo::bar { enum class Colors { red = 3, green, blue }; }",
    "namespace foo { namespace bar { enum class Colors { red, green, blue }; }}"
  };
  std::vector<std::string> namespaceVector;
  std::vector<std::string> colors;
  std::string result;
  // Note that if you're tracking namespace, your namespace scope will generally want to be the scope
  // the callback sends you plus 1, since the scope is created after the namespace is declared.
  int namespaceScopeDepth = -1; // Should get set to 0 when we hit the namespace directive

  fr::codegen::parser::ParserDriver parser;

  parser.namespacePush.connect([&namespaceVector, &namespaceScopeDepth](auto& name, auto depth) {
    namespaceVector.push_back(name);
    namespaceScopeDepth = depth;
  });

  parser.enumClassPush.connect([&enumName](auto& name, auto /* notused */) {
    enumName = name;
  });

  parser.enumIdentifier.connect([&colors](auto& name, auto& identifier) {
    colors.push_back(identifier);
  });

  for(std::string namespacedEnum : enums) {
    ASSERT_TRUE(parser.parse(namespacedEnum.begin(), namespacedEnum.end(), result));
    ASSERT_GT(namespaceScopeDepth, -1);
    ASSERT_EQ(namespaceVector.size(),2);
    ASSERT_EQ(namespaceVector[0], "foo");
    ASSERT_EQ(namespaceVector[1], "bar");
    ASSERT_EQ(colors.size(),3);
    ASSERT_EQ(colors[0], "red");
    ASSERT_EQ(colors[1], "green");
    ASSERT_EQ(colors[2], "blue");
    // Clear out the vectors for the next run
    namespaceVector.clear();
    colors.clear();
  }
}

// Verify a template class doesn't throw a monkey wrench into
// our parsing
TEST(ParserSignals, TemplateClassIgnored) {

  std::string enumName;
  std::vector<std::string> colors;
  std::string code =
 "namespace fun { template <typename Wombat> class OZAnimals { void help() { std::cout << \"HELP WOMBAT\" << std::endl ; }}; enum WombatColors { red, green, blue};}";

  std::string result;

  fr::codegen::parser::ParserDriver parser;

  parser.enumPush.connect([&enumName](auto& name, auto /* notused */) {
    enumName = name;
  });

  parser.enumIdentifier.connect([&colors](auto &name, auto &identifier) {
    colors.push_back(identifier);
  });

  ASSERT_TRUE(parser.parse(code.begin(), code.end(), result));
  ASSERT_EQ(enumName, "WombatColors");
  ASSERT_EQ(colors.size(), 3);
  ASSERT_EQ(colors[0], "red");
  ASSERT_EQ(colors[1], "green");
  ASSERT_EQ(colors[2], "blue");
}
