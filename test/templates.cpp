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
#include <fr/codegen/parser.h>
#include <string>

using namespace fr::codegen::parser;

// Basically just test that we can just parse a template as a rule
// and have nothing left over. We're mostly ignoring them so
// we just need to make sure we can ignore them (But they don't
// go in an ignore list because we also want to ignore templated
// classes and member functions.

TEST(Templates,TemplateParse) {
  auto const templateGrammar = templateKeyword >> templateGuts;

  std::string data("template <typename flibble, flabble<flabble>> class aardvark");
  std::string::const_iterator iter = data.begin();
  std::string::const_iterator end = data.end();
  
  bool r = boost::spirit::x3::phrase_parse(iter, end,
					   templateGrammar,
					   x3::ascii::space);

  std::string remains(iter, end);
  ASSERT_TRUE(r);
  ASSERT_EQ(remains, "class aardvark");
}
