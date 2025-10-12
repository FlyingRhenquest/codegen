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

#pragma once

#include <boost/signals2.hpp>
#include <boost/spirit/home/x3.hpp>
#include <string>
#include <vector>

namespace fr::codegen::parser {

  namespace x3 = boost::spirit::x3;

  // Ignore rules for comments

  x3::rule<class SingleLineComment> const singleLineComment = "singleline_comment";
  auto const singleLineComment_def =
    x3::lexeme[x3::lit("//") >> *~x3::char_("\r\n") >> x3::eol];

  x3::rule<class BlockComment> const blockComment = "block_comment";
  auto const blockComment_def = x3::lexeme["/*" >> *(x3::char_ - "*/") >> "*/"];

  BOOST_SPIRIT_DEFINE(singleLineComment, blockComment);

  // Keywords I care about
  
  x3::rule<class NamespaceKeyword> const namespaceKeyword = "namespace_keyword";
  auto const namespaceKeyword_def = x3::lit("namespace");

  x3::rule<class EnumKeyword> const enumKeyword = "enum_keyword";
  auto const enumKeyword_def = x3::lit("enum");

  x3::rule<class ClassKeyword> const classKeyword = "class_keyword";
  auto const classKeyword_def = x3::lit("class");
  
  BOOST_SPIRIT_DEFINE(namespaceKeyword, enumKeyword, classKeyword);

  // Keywords I don't particularly care about (right now) but that you're likely
  // to encounter before you hit your enum declarations

  x3::rule<class PragmaKeyword> const pragmaKeyword = "pragma_keyword";
  auto const pragmaKeyword_def = x3::lit("#pragma");
  
  x3::rule<class IncludeKeyword> const includeKeyword = "include_keyword";
  auto const includeKeyword_def = x3::lit("#include");

  x3::rule<class IncludePath> const includePath = "include_path";
  auto const includePath_def = +(x3::lit("/") | x3::lit(".") | x3::alnum);

  BOOST_SPIRIT_DEFINE(pragmaKeyword, includeKeyword, includePath);
  
  // Identifier

  x3::rule<class Identifier,std::string> const identifier = "identifier";
  auto const identifier_def = x3::lexeme[x3::alpha >> *(x3::alnum | '_')];

  BOOST_SPIRIT_DEFINE(identifier);
  
  // Scope stuff

  x3::rule<class ScopePush> const scopePush = "scope_push";
  auto const scopePush_def = x3::lit("{");

  x3::rule<class ScopePop> const scopePop = "scope_pop";
  auto const scopePop_def = x3::lit("}");

  BOOST_SPIRIT_DEFINE(scopePush, scopePop);

  // Driver for parsing. This exposes some boost signal callbacks you can
  // subscribe to be provided information about what the parser is collecting.

  class ParserDriver {
  public:
    // Called when scope increments
    boost::signals2::signal<void()> incScope;
    // Called when scope decrements
    boost::signals2::signal<void()> decScope;

    // There aren't specific pops for namespaces and enums. You get a scope push
    // when one of those is declared and a scope pop when the scope is exited.
    
    // Called when a new namespace is created. You can get multiple calls for
    // this at the scame scope level if the namespace is nested. Parameters passed
    // are the namespace name and the current scope depth. If you're keeping track
    // of namespaces, you may want to make sure that scopePop cleans up namespaces
    // that have gone out of scope
    boost::signals2::signal<void(const std::string&, int)> namespacePush;
    // Called when we hit an enum declaration. The following scope will
    // contain the enum definition. The parameters are the enum name and
    // the current scope. Anonymous enums will be ignored.
    boost::signals2::signal<void(const std::string&, int)> enumPush;
    // We need to be able to differentiate enum classes from moldy old enums
    // so we have a different callback for them.
    boost::signals2::signal<void(const std::string&, int)> enumClassPush;
    // Enum identifier signal. This supplies the enum name and the identifier
    // name we just parsed. It would easy to extend this to include the
    // value as well, but I'm not doing anything with those currently
    boost::signals2::signal<void(const std::string&, const std::string&)> enumIdentifier;

    // Shovels data into x3's parser and fires signals. Result will contain any
    // leftover characters. This function works similarly to the boost::spirit::x3
    // parser and can use any iterator the x3 parser will accept.
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, std::string& result) {
      int scopeDepth = 0;
      // If we're in an enum scope, currentEnumName will be the name of the
      // current enum.
      std::string currentEnumName;

      // Lambdas to handle various events
      
      auto handleNamespaceIdentifier = [&](auto& ctx){
	namespacePush(x3::_attr(ctx), scopeDepth);
	// If we don't clear context each time, the next time the method is called,
	// we'll receive all the previous data from the context in addition to the stuff
	// we're interested in. There's not much info on the internet about why this
	// happens and the proposed solutions to deal with them from stackoverflow et al
	// are incorrect.
	x3::_attr(ctx) = "";
      };

      auto handleScopePush = [&](auto& ctx) {
	incScope();
	scopeDepth++;
      };

      auto handleScopePop = [&](auto& ctx) {
	decScope();
	scopeDepth--;
	currentEnumName = "";
      };
      
      auto handleEnumPush = [&](auto& ctx) {
	enumPush(x3::_attr(ctx), scopeDepth);
	currentEnumName = x3::_attr(ctx);
	x3::_attr(ctx) = "";
      };

      auto handleEnumIdentifier = [&](auto& ctx) {
	enumIdentifier(currentEnumName, x3::_attr(ctx));
	x3::_attr(ctx) = "";
      };

      auto handleEnumClassPush = [&](auto& ctx) {
	enumClassPush(x3::_attr(ctx), scopeDepth);
	x3::_attr(ctx) = "";
      };

      // Some grammars I'm ignoring right now
      auto const pragmaOnceGrammar =
	x3::lexeme[ pragmaKeyword >> x3::lit(" ") >> x3::lit("once")];

      auto const includeGrammar =
	x3::lexeme[includeKeyword >>
		   x3::lit(" ") >>
		   (x3::lit("<") | x3::lit("\"")) >>
	  includePath >>
		   (x3::lit(">") | x3::lit("\""))];
	  
      // Wire up grammars to signal events
      auto const namespaceGrammar =
	namespaceKeyword >>
	identifier [ handleNamespaceIdentifier ] >>
	*(x3::lit("::") >> identifier [ handleNamespaceIdentifier ]) >>
	scopePush [ handleScopePush ];

      auto const enumGrammar =
	enumKeyword >> !classKeyword >> 
	identifier [ handleEnumPush ] >>
	scopePush [ handleScopePush ] >>
	// We are now in the enum scope, where we set up identifiers and optionally
	// assign them to values
	*(identifier [ handleEnumIdentifier ] >> -(x3::lit("=") >> +x3::alnum) >> -(x3::lit(","))) >>
	scopePop [ handleScopePop ] >> x3::lit(";");

      auto const enumClassGrammar =
	enumKeyword >>
	classKeyword >>
	identifier [ handleEnumClassPush ] >>
	scopePush [ handleScopePush ] >>
	*(identifier [ handleEnumIdentifier ] >> -(x3::lit("=") >> +x3::alnum) >> -(x3::lit(","))) >>
	scopePop [ handleScopePop ] >>
	x3::lit(";");

      auto const programGrammar = * (
				    scopePush |
				    namespaceGrammar |
				    enumGrammar |
				    enumClassGrammar |
				    scopePop);

      auto const ignoreStuff =
	blockComment |
	singleLineComment |
	pragmaOnceGrammar |
	includeGrammar |
	x3::ascii::space;

      // Parse all the things      
      return x3::phrase_parse(first, last,
			      programGrammar,
			      ignoreStuff,
			      result);      
    }

  };
  
  
  
}
