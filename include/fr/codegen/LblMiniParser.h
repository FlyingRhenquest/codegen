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
 * This file contains a mini-parser designed to work as a LblFilter.
 * Some of our LblFilters are going to need to know the namespace
 * and class that they're currently in as they're processing.
 * This aims to provide that information, without having to invoke
 * the very complex parser in parser.h.
 */

#pragma once
#include <boost/signals2.hpp>
#include <boost/spirit/home/x3.hpp>
#include <fr/codegen/LblFilter.h>
#include <string>

namespace fr::codegen::miniparser {

  namespace x3 = boost::spirit::x3;

  x3::rule<class SingleLineComment> const singleLineComment = "singleline_comment";
  auto const singleLineComment_def =
    x3::lexeme[x3::lit("//") >> *~x3::char_("\r\n") >> x3::eol];

  x3::rule<class Annotation> const annotation = "annotation";
  auto const annotation_def =
    x3::lexeme[x3::lit("[[") >>
               *(x3::char_ - x3::char_("]"))
               >> x3::lit("]]")];

  BOOST_SPIRIT_DEFINE(singleLineComment, annotation);

  x3::rule<class ClassKeyword> const classKeyword = "class_keyword";
  auto const classKeyword_def = x3::lit("class");

  x3::rule<class StructKeyword> const structKeyword = "struct_keyword";
  auto const structKeyword_def = x3::lit("struct");
  
  x3::rule<class PublicKeyword> const publicKeyword = "public_keyword";
  auto const publicKeyword_def = x3::lit("public");

  x3::rule<class PrivateKeyword> const privateKeyword = "private_keyword";
  auto const privateKeyword_def = x3::lit("private");

  x3::rule<class ProtectedKeyword> const protectedKeyword = "protected_keyword";
  auto const protectedKeyword_def = x3::lit("protected");

  x3::rule<class Identifier,std::string> const identifier = "identifier";
  auto const identifier_def = x3::lexeme[(x3::alpha | x3::char_('_')) >> *(x3::alnum | x3::char_('_'))];

  BOOST_SPIRIT_DEFINE(classKeyword, structKeyword, publicKeyword, privateKeyword,
                      protectedKeyword, identifier)

  /**
   * Miniparser receives a line from a LblEmitter and emits one or more signals
   * based on that line.
   *
   * emit - forward the line to the next object.
   * classPush - We entered a class. Provides the class name downstream
   * classPop - We exited a class.
   *
   * If the line contains something that would cause another signal to be
   * emitted, those signals will be called *prior* to emit.
   */
  class LblMiniparser : public LblFilter {
  public:
    LblMiniparser() = default;
    virtual ~LblMiniparser() = default;

    boost::signals2::signal<void(const std::string&)> classPush;
    boost::signals2::signal<void()> classPop;
    
    void process(const std::string& line) {
      auto handleClassPush = [&](auto ctx) {
        classPush(x3::_attr(ctx));
        x3::_attr(ctx) = "";
      };      

      auto handleClassPop = [&](){
        classPop();
      };
                                   
      auto const classGrammar = (classKeyword | structKeyword) >>
        identifier [handleClassPush];

      auto const unclassGrammar = x3::lit("};") [handleClassPop];

      auto const watchForGrammar =
        classGrammar | unclassGrammar;

      auto const ignoreGrammar = singleLineComment |
        x3::ascii::space |
        annotation;

      x3::phrase_parse(line.begin(), line.end(), watchForGrammar, ignoreGrammar);

      emit(line);      
    }

  };

}
