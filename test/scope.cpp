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
#include <vector>

TEST(ScopeTests, ScopePush) {
  int scopeDepth = 0;
  
  // Now let's feed it some
  std::string somescopes("{{{{{");
  // Remaining characters (there should be none)
  std::string result;
  // We don't need phrase_parse here since we don't need an ignore rule
  bool r =
    boost::spirit::x3::parse(somescopes.begin(), somescopes.end(),
			     +fr::codegen::parser::scopePush [([&scopeDepth](auto& ctx) { scopeDepth++; })],
			     result);
  ASSERT_TRUE(r);
  ASSERT_EQ(scopeDepth, 5);
}

TEST(ScopeTests, ScopePop) {
  int scopeDepth = 0;
  std::string somescopes("{} {{}{{{}{}{{{}{{}}}}}}}");
  std::string result;
  auto const grammar = +(fr::codegen::parser::scopePush [([&scopeDepth](auto& ctx) { scopeDepth++; })] |
			 fr::codegen::parser::scopePop [([&scopeDepth](auto& ctx) { scopeDepth--; })]);
  bool r = boost::spirit::x3::parse(somescopes.begin(), somescopes.end(),
				    grammar,
				    result);
  ASSERT_TRUE(r);
  // Between the pushes and the pops we should be back at 0
  ASSERT_EQ(0, scopeDepth);
}

TEST(ScopeTests, NamespacePush) {
  int scopeDepth = 0;
  std::string input("namespace foo::bar::baz {");
  std::vector<std::string> namespacesGoHere;
  auto const grammar = fr::codegen::parser::namespaceKeyword >>
    fr::codegen::parser::identifier
    [ // Still trying to figure out how to make these readable
      ([&namespacesGoHere](auto& ctx) { namespacesGoHere.push_back(boost::spirit::x3::_attr(ctx)); })
      ] >>
    *(boost::spirit::x3::lit("::") >> fr::codegen::parser::identifier)
    [
     ([&namespacesGoHere](auto& ctx) { namespacesGoHere.push_back(boost::spirit::x3::_attr(ctx));
       // Unless you want to concatinate all your namespaces by the end
       // of the vector, you need to clear the context each time this
       // lambda is called (Solutions on stackoverflow suggesting using
       // x3::as are incorrect -- x3::as does not in fact exist. x3::omit
       // also does not do what you want it to.
       _attr(ctx) = "";
     })
     ] >> 
    fr::codegen::parser::scopePush
    [
     ([&scopeDepth](auto& ctx) {scopeDepth++;})
     ];
       
  std::string result;
  // Have the parser eat spaces so my code can have them
  bool r = boost::spirit::x3::phrase_parse(input.begin(), input.end(),
					   grammar,
					   boost::spirit::x3::ascii::space,
					   result);
  ASSERT_TRUE(r);
  ASSERT_EQ(scopeDepth, 1);
  ASSERT_EQ(namespacesGoHere.size(), 3);
  ASSERT_EQ(namespacesGoHere[0], "foo");
  ASSERT_EQ(namespacesGoHere[1], "bar");
  ASSERT_EQ(namespacesGoHere[2], "baz");
}
