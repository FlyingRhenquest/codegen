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

#include <gtest/gtest.h>
#include <fr/codegen/data.h>
#include <fr/codegen/parser.h>

TEST(ParsingData, FullEnumData) {
  // Yes, this is legit -- strings separated by nothing concatinate
  const std::string enumCode(
    "namespace foo::bar {"
    "enum Color {"
    "  red,"
    "  green,"
    "  blue"
    "};}"
  );

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::EnumDriver data;
  std::string result;
  data.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.enums.size(), 1);
  fr::codegen::EnumData e;
  try {
    e = data.enums.at("foo::bar::Color");
  } catch (std::exception &e) {
    FAIL() << "Enum foo::bar::Color not found";
  }
  ASSERT_EQ(e.identifiers[0], "red");
  ASSERT_EQ(e.identifiers[1], "green");
  ASSERT_EQ(e.identifiers[2], "blue");
  ASSERT_FALSE(e.isClassEnum);
}

TEST(ParsingData, EnumClassData) {
  const std::string enumCode(
    "namespace foo::bar {"
    "enum class Color {"
    "  red,"
    "  green,"
    "  blue"
    "};}"
  );

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::EnumDriver data;
  std::string result;
  data.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.enums.size(), 1);
  fr::codegen::EnumData e;
  try {
    e = data.enums.at("foo::bar::Color");
  } catch (std::exception &e) {
    FAIL() << "Enum foo::bar::Color not found";
  }
  ASSERT_EQ(e.identifiers[0], "red");
  ASSERT_EQ(e.identifiers[1], "green");
  ASSERT_EQ(e.identifiers[2], "blue");
  ASSERT_TRUE(e.isClassEnum);
}


TEST(ParsingData, MultiEnum) {
  const std::string enumCode(
    "namespace foo::bar {"
    "enum class Color {"
    "  red,"
    "  green,"
    "  blue"
    "};}"
    // Why would I even put that in the text?! lol
    "/* This is outside the namespace */"
    "// Also guess what happens if you don't put a CR here...\n" // Because single line comment matching needs a cr at the end
    "enum fish {"
    "  goldfish,"
    "  snapper," 
    "  trout," 
    "  larch" // Wait that's not a fish!
    "};"
  );

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::EnumDriver data;
  std::string result;
  data.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.enums.size(), 2);
  fr::codegen::EnumData e;

  try {
    e = data.enums.at("foo::bar::Color");
  } catch (std::exception &e) {
    FAIL() << "Enum foo::bar::Color not found";
  }
  ASSERT_EQ(e.identifiers[0], "red");
  ASSERT_EQ(e.identifiers[1], "green");
  ASSERT_EQ(e.identifiers[2], "blue");
  ASSERT_TRUE(e.isClassEnum);
  e.clear();
  try {
    e = data.enums.at("fish");
  } catch (std::exception &e) {
    FAIL() << "Enum fish not found (go fish)";
  }
  ASSERT_FALSE(e.isClassEnum);
  ASSERT_EQ(e.identifiers[0], "goldfish");
  ASSERT_EQ(e.identifiers[1], "snapper");
  ASSERT_EQ(e.identifiers[2], "trout");
  ASSERT_EQ(e.identifiers[3], "larch");
}
