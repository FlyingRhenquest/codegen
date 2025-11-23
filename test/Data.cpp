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
#include <fr/codegen/drivers.h>

using namespace fr::codegen;

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

  std::string result;
  fr::codegen::parser::ParserDriver parser;
  fr::codegen::EnumDriver driver;
  std::map<std::string, fr::codegen::EnumData> data;
  driver.enumAvailable.connect([&data](const std::string& key, const EnumData& value) {
    data[key] = value;
  });
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  fr::codegen::EnumData e;
  try {
    e = data.at("foo::bar::Color");
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
  fr::codegen::EnumDriver driver;
  std::map<std::string, EnumData> data;
  driver.enumAvailable.connect([&data](const std::string& key, const EnumData& value) {
    data[key] = value;
  });
  std::string result;
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  fr::codegen::EnumData e;
  try {
    e = data.at("foo::bar::Color");
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
  fr::codegen::EnumDriver driver;
  std::map<std::string, EnumData> data;
  std::string result;
  driver.enumAvailable.connect([&data](const std::string& key, const EnumData& value) {
    data[key] = value;
  });
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(enumCode.begin(), enumCode.end(), result));
  ASSERT_EQ(data.size(), 2);
  fr::codegen::EnumData e;

  try {
    e = data.at("foo::bar::Color");
  } catch (std::exception &e) {
    FAIL() << "Enum foo::bar::Color not found";
  }
  ASSERT_EQ(e.identifiers[0], "red");
  ASSERT_EQ(e.identifiers[1], "green");
  ASSERT_EQ(e.identifiers[2], "blue");
  ASSERT_TRUE(e.isClassEnum);
  e.clear();
  try {
    e = data.at("fish");
  } catch (std::exception &e) {
    FAIL() << "Enum fish not found (go fish)";
  }
  ASSERT_FALSE(e.isClassEnum);
  ASSERT_EQ(e.identifiers[0], "goldfish");
  ASSERT_EQ(e.identifiers[1], "snapper");
  ASSERT_EQ(e.identifiers[2], "trout");
  ASSERT_EQ(e.identifiers[3], "larch");
}

TEST(ParsingData,HowAboutAClass) {
  const std::string classCode(
    "namespace monkey::bagel {"
    "class Wibble {"
    "public:"
    "  std::string wobble(); "
    "  int wibblewobble;"
    "};");

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::ClassDriver driver;
  std::map<std::string, ClassData> data;
  std::string className;
  driver.classAvailable.connect([&className, &data](const std::string &key, const ClassData& value) {
    className = key;
    data[className] = value;
  });
  std::string result;
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(classCode.begin(), classCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  ASSERT_EQ(data[className].name, "Wibble");
  ASSERT_FALSE(data[className].serializable);
  ASSERT_EQ(data[className].members.size(), 1);
  ASSERT_EQ(data[className].methods.size(), 1);
  ASSERT_EQ(data[className].members[0].name, "wibblewobble");
  ASSERT_EQ(data[className].methods[0].name, "wobble");
  ASSERT_FALSE(data[className].members[0].serializable);
  ASSERT_FALSE(data[className].members[0].generateGetter);
  ASSERT_FALSE(data[className].members[0].generateSetter);
}

TEST(ParsingData,ClassWithInlineMethod) {
  const std::string classCode(
    "namespace monkey::bagel {"
    "class Wibble {"
    "public:"
    "  int wibblewobble;"
    "  std::string wobble() { "
    "     std::string ret = \"This should be ignored by the parser\";"
    "     if (wibblewobble) {"
    "        std::cout << ret << std::endl;"
    "      }"
    "  }"
    "};");

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::ClassDriver driver;
  std::map<std::string, ClassData> data;
  std::string className;
  driver.classAvailable.connect([&className, &data](const std::string& key, const ClassData& value) {
    className = key;
    data[className] = value;
  });
  std::string result;
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(classCode.begin(), classCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  ASSERT_EQ(data[className].name, "Wibble");
  ASSERT_EQ(data[className].members.size(), 1);
  ASSERT_EQ(data[className].methods.size(), 1);
  ASSERT_EQ(data[className].members[0].name, "wibblewobble");
  ASSERT_EQ(data[className].methods[0].name, "wobble");
}

TEST(ParsingData,ClassTemplateMethod) {
  const std::string classCode(
    "namespace monkey::bagel {"
    "class Wibble {"
    "public:"
    "  int wibblewobble;"
    "  template <typename T>"
    "  std::string wobble(T& pleh) { "
    "     std::string ret = \"This should be ignored by the parser\";"
    "     if (wibblewobble) {"
    "        std::cout << ret << std::endl;"
    "      }"
    "  }"
    "};");

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::ClassDriver driver;
  std::map<std::string, ClassData> data;
  std::string className;
  driver.classAvailable.connect([&className, &data](const std::string& key, const ClassData& value) {
    className = key;
    data[className] = value;
  });
  std::string result;
  driver.regParser(parser);
  ASSERT_TRUE(parser.parse(classCode.begin(), classCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  ASSERT_EQ(data[className].name, "Wibble");
  ASSERT_EQ(data[className].members.size(), 1);
  ASSERT_EQ(data[className].methods.size(), 1);
  ASSERT_EQ(data[className].members[0].name, "wibblewobble");
  ASSERT_EQ(data[className].methods[0].name, "wobble");
}

TEST(ParsingData,ClassAnnotation) {
  const std::string classCode(
    "namespace monkey::bagel {"
    "[[cereal]] class Wibble {"
    "public:"
    "  [[cereal,get,set]] int wibblewobble;"
    "  std::string wobble() { "
    "     std::string ret = \"This should be ignored by the parser\";"
    "     if (wibblewobble) {"
    "        std::cout << ret << std::endl;"
    "      }"
    "  }"
    "};");

  fr::codegen::parser::ParserDriver parser;
  fr::codegen::ClassDriver driver;
  std::string result;
  std::map<std::string, ClassData> data;
  std::string className;
  driver.regParser(parser);

  driver.classAvailable.connect([&data, &className](const std::string& key, const ClassData& value) {
    className = key;
    data[className] = value;
  });
  
  ASSERT_TRUE(parser.parse(classCode.begin(), classCode.end(), result));
  ASSERT_EQ(data.size(), 1);
  ASSERT_EQ(data[className].name, "Wibble");
  ASSERT_TRUE(data[className].serializable);
  ASSERT_EQ(data[className].members.size(), 1);
  ASSERT_EQ(data[className].methods.size(), 1);
  ASSERT_EQ(data[className].members[0].name, "wibblewobble");
  ASSERT_EQ(data[className].methods[0].name, "wobble");
  ASSERT_TRUE(data[className].members[0].serializable);
  ASSERT_TRUE(data[className].members[0].generateGetter);
  ASSERT_TRUE(data[className].members[0].generateSetter);
}

// Make sure we can parse a class with a constructor/destructor
// TODO: test colin constructor initialization syntax and
// multiple comma separated member decls.
TEST(ParsingData, ConstructorDestructorBasic) {
  const std::string classCode(
    "/* Comment */"
    "#pragma once"
    "#include <pleh/pleh/pleh.h>"
    "struct MyClass : public shabizzle<MyClass> {"
    "  int _foo = 2;"
    "  int _bar;"
    "  foo::bar::baz bizzle;"
    "public:"
    "  using MyType = int;"
    "  MyClass() = default;"
    "  MyClass(int foo, int bar) { _foo = foo; _bar = bar }"
    "  virtual ~MyClass() = default;"
    "  void sayHello(const std::string& h) const { std::print(\"{}\",h);}"
    "};"
  );
  fr::codegen::parser::ParserDriver parser;
  fr::codegen::ClassDriver driver;
  std::string result;
  ClassData data;
  bool gotAClass = false;
  driver.regParser(parser);

  driver.classAvailable.connect([&data, &gotAClass](const std::string &className, const ClassData& value) {
    gotAClass = true;
    data = value;
  });

  parser.parse(classCode.begin(), classCode.end(), result);
  
  ASSERT_TRUE(gotAClass);  
}
