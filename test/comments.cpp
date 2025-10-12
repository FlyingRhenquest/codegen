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

TEST(CommentTest, IgnoreLineComment) {
  std::string data("The quick brown // something something\nwat?");
  std::string result;

  // Just consume all chars into result except for things ignored by
  // the single line comment rule
  bool r = boost::spirit::x3::phrase_parse(data.begin(), data.end(),
					   +boost::spirit::x3::char_,
					   fr::codegen::parser::singleLineComment,
					   result);

  ASSERT_TRUE(r);
  ASSERT_EQ(result, "The quick brown wat?");
}

TEST(CommentTest, IgnoreBlockComment) {
  std::string data("The quick brown/* wat? */ wat!");
  std::string result;

  bool r = boost::spirit::x3::phrase_parse(data.begin(), data.end(),
					   +boost::spirit::x3::char_,
					   fr::codegen::parser::blockComment,
					   result);
  ASSERT_TRUE(r);
  ASSERT_EQ(result, "The quick brown wat!");
}
