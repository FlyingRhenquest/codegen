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

#include <iostream>

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

  x3::rule<class StructKeyword> const structKeyword = "struct_keyword";
  auto const structKeyword_def = x3::lit("struct");

  x3::rule<class TemplateKeyword> const templateKeyword = "template_keyword";
  auto const templateKeyword_def = x3::lit("template");

  x3::rule<class ConstKeyword> const constKeyword = "const_keyword";
  auto const constKeyword_def = x3::lit("const");

  x3::rule<class StaticKeyword> const staticKeyword = "static_keyword";
  auto const staticKeyword_def = x3::lit("static");

  x3::rule<class PublicKeyword> const publicKeyword = "public_keyword";
  auto const publicKeyword_def = x3::lit("public");

  x3::rule<class PrivateKeyword> const privateKeyword = "private_keyword";
  auto const privateKeyword_def = x3::lit("private");

  x3::rule<class ProtectedKeyword> const protectedKeyword = "protected_keyword";
  auto const protectedKeyword_def = x3::lit("protected");

  x3::rule<class VirtualKeyword> const virtualKeyword = "virtual_keyword";
  auto const virtualKeyword_def = x3::lit("virtual");

  x3::rule<class OverrideKeyword> const overrideKeyword = "override_keyword";
  auto const overrideKeyword_def = x3::lit("override");
  
  BOOST_SPIRIT_DEFINE(namespaceKeyword, enumKeyword, classKeyword, structKeyword,
		      templateKeyword, constKeyword, staticKeyword, publicKeyword,
		      privateKeyword, protectedKeyword, virtualKeyword, overrideKeyword);

  // Annotations -- I'm adding a few that C++ should ignore but that I can use to tag
  // classes or members serializable, to tag members to generate getters and setters
  // and to put places tags in the code that can be replaced later on.

  x3::rule<class Annotation, std::string> const annotation = "annotation";
  auto const annotation_def = x3::lit("[[") >> *(x3::alnum | x3::space | x3::char_(",_()")) >> x3::lit("]]");

  BOOST_SPIRIT_DEFINE(annotation);
  
  // Keywords I don't particularly care about (right now) but that you're likely
  // to encounter before you hit your enum declarations

  x3::rule<class PragmaKeyword> const pragmaKeyword = "pragma_keyword";
  auto const pragmaKeyword_def = x3::lit("#pragma");
  
  x3::rule<class IncludeKeyword> const includeKeyword = "include_keyword";
  auto const includeKeyword_def = x3::lit("#include");

  // Template guts -- ignore everything from < to >
  // We do have to pay attention to < level in the template, which
  // will basically be identified as "more template guts."
  // We are ignoring everything in a template declaration for the
  // foreseeable future

  x3::rule<class TemplateGuts> const templateGuts = "template_guts";
  auto const templateGuts_def = x3::lexeme[x3::char_("<") >>
       *(x3::char_ - x3::char_("<>")) >> *templateGuts | x3::char_(">")];

  // Ignore the next scope and all the scopes inside it
  x3::rule<class IgnoreScopes> const ignoreScopes = "ignore_scopes";
  auto const ignoreScopes_def =
    x3::lexeme[x3::char_('{') >>
               *((x3::char_ - x3::char_("{}")) >> *ignoreScopes) >>
               x3::char_("}")];
  
  BOOST_SPIRIT_DEFINE(pragmaKeyword, includeKeyword, templateGuts, ignoreScopes);
  
  // Identifier

  x3::rule<class Identifier,std::string> const identifier = "identifier";
  auto const identifier_def = x3::lexeme[(x3::alpha | x3::char_('_')) >> *(x3::alnum | x3::char_('_'))];

  // Identifier with maybe namespace and maybe template stuff
  x3::rule<class EnhancedIdentifier,std::string> const enhancedIdentifier = "enhanced_identifier";
  auto const enhancedIdentifier_def = x3::lexeme[(x3::alpha | x3::char_('_')) >> *(x3::alnum | x3::char_("_<>:&*"))];

  BOOST_SPIRIT_DEFINE(identifier, enhancedIdentifier);
  
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
    // Called when we encounter a non-template class declaration. The
    // callback parameters are the class name and the current scope depth.
    boost::signals2::signal<void(const std::string&, int)> classPush;
    // Signals that we're done with the current class we're parsing
    boost::signals2::signal<void()> classPop;
    // Called when we encounter a non-template struct declaraction.
    boost::signals2::signal<void(const std::string&, int)> structPush;
    // Called for a class parent
    boost::signals2::signal<void(const std::string&)> privateClassParent;
    boost::signals2::signal<void(const std::string&)> protectedClassParent;
    boost::signals2::signal<void(const std::string&)> publicClassParent;
    // Signals for changing current privacy level in a class
    boost::signals2::signal<void()> privateInClass;
    boost::signals2::signal<void()> protectedInClass;
    boost::signals2::signal<void()> publicInClass;
    // Signals that we've discovered a member
    // parameters are inClassConst, inClassStatic, member type, member name
    boost::signals2::signal<void(bool, bool, const std::string&, const std::string&)> memberFound;
    // Signals that we've discovered a method
    // parameters are inClassConst, inClassStatic, inClassVirtual, returnType, methodName
    boost::signals2::signal<void(bool, bool, bool, const std::string&, const std::string&)> methodFound;
    // We encountered an annotation - parameter passed in is the annotation we encountered
    boost::signals2::signal<void(const std::string&)> annotationFound;

    // Some things to track keywords inside a class. These will be set/reset
    // when we run across things like "const", "static", "virtual" or "override"
    bool inClassConst;
    bool inClassStatic;
    bool inClassVirtual;
    bool inClassStruct;

    // Store the name of a member or method type
    std::string inClassEnhancedIdentifier;
    // Store the name of a member or method
    std::string inClassIdentifier;

    void resetInClassFlags() {
      inClassConst = false;
      inClassStatic = false;
      inClassVirtual = false;
      inClassStruct;
      inClassEnhancedIdentifier = "";
      inClassIdentifier = "";
    }

    // Shovels data into x3's parser and fires signals. Result will contain any
    // leftover characters. This function works similarly to the boost::spirit::x3
    // parser and can use any iterator the x3 parser will accept.
    template <typename Iterator>
    bool parse(Iterator first, Iterator last, std::string& result) {
      int scopeDepth = 0;
      resetInClassFlags();
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

      auto handleClassPush = [&](auto& ctx) {
        if (!inClassStruct) {
          classPush(x3::_attr(ctx), scopeDepth);
        } else {
          structPush(x3::_attr(ctx), scopeDepth);
        }
	x3::_attr(ctx) = "";
      };

      auto handleClassPop = [&]() {
	classPop();
      };

      auto handlePrivateClassParent = [&](auto& ctx) {
	privateClassParent(x3::_attr(ctx));
	x3::_attr(ctx) = "";
      };

      auto handleProtectedClassParent = [&](auto& ctx) {
	protectedClassParent(x3::_attr(ctx));
	x3::_attr(ctx) = "";
      };

      auto handlePublicClassParent = [&](auto& ctx) {
	publicClassParent(x3::_attr(ctx));
	x3::_attr(ctx) = "";
      };

      auto handlePublicInClass = [&]() {
	publicInClass();
      };

      auto handleProtectedInClass = [&] () {
	protectedInClass();
      };

      auto handlePrivateInClass = [&]() {
	privateInClass();
      };

      auto handleStaticMember = [&]() {
	inClassStatic = true;
      };

      auto handleVirtualMember = [&](){
	inClassVirtual = true;
      };

      auto handleConstMember = [&]() {
	inClassConst = true;
      };
				      
      auto handleInClassEnhancedIdentifier = [&](auto& ctx) {
	inClassEnhancedIdentifier = x3::_attr(ctx);
	x3::_attr(ctx) = "";
      };
      
      auto handleInClassIdentifier = [&](auto& ctx) {
	inClassIdentifier = x3::_attr(ctx);
	x3::_attr(ctx) = "";
      };

      auto handleMemberFound = [&]() {
	memberFound(inClassConst, inClassStatic, inClassEnhancedIdentifier, inClassIdentifier);
	resetInClassFlags();
      };

      auto handleMethodFound = [&](){
	methodFound(inClassConst, inClassStatic, inClassVirtual, inClassEnhancedIdentifier, inClassIdentifier);
	resetInClassFlags();
      };

      auto handleStructKeyword = [&](){
        inClassStruct = true;
      };

      auto handleAnnotation = [&](auto& ctx){
        annotationFound(x3::_attr(ctx));
        x3::_attr(ctx) = "";
      };

      // Some grammars I'm ignoring right now
      auto const pragmaOnceGrammar =
	pragmaKeyword >> x3::lit("once");

      auto const includeGrammar =
	includeKeyword >>
           x3::char_("<\"") >>
           *(x3::char_ - x3::char_(">\"")) >>
          x3::char_(">\"");
	  
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

      // Template class grammar -- identify a template class such that we can just straight
      // up ignore it. You should really not point this parser at a file that has them and
      // just just keep your data objects that you want to generate code for separate from
      // more complex things, but I'll try to handle what I can.
      auto const templateClassGrammar =
	templateKeyword >>
	templateGuts >>
	(classKeyword | structKeyword) >>
	identifier >>
	// Don't want to trigger a scope push here
	x3::lit("{") >> 
	// Now we'll just ignore everything we see down to the "};" This strategy won't work
	// if you're putting classes and enums in your template classes, but that's what you
	// get for not listening to me about separating your data objects out.
	*(x3::char_ - "};") >>
	x3::lit("};");

      auto const privateClassParentGrammar =
	-privateKeyword >>
	enhancedIdentifier [handlePrivateClassParent];

      auto const protectedClassParentGrammar =
	protectedKeyword >>
	enhancedIdentifier [handleProtectedClassParent];

      auto const publicClassParentGrammar =
	publicKeyword >>
	enhancedIdentifier [handlePublicClassParent];

      auto const enhancedIdIdGrammar =
	enhancedIdentifier [handleInClassEnhancedIdentifier] >>
	identifier [handleInClassIdentifier];

      // Simply eat everything in the parameter list. It wouldn't be terribly difficult
      // to parse out, I just don't particualrly need it for anything.
      auto const parameterGrammar = x3::char_('(') >> *(x3::char_ - x3::char_(')')) >> x3::char_(')');

      auto const ignoreUsing = x3::lit("using") >> *(x3::char_ - x3::char_(';')) >> x3::char_(';');
      
      auto const defaultMethod = x3::char_('=') >> x3::lit("default");

      auto constructorDestructor =
        *virtualKeyword >>
        -x3::char_('~') >>
        identifier >>
        parameterGrammar >>
        *(ignoreScopes |
          defaultMethod |
          x3::char_(';'));

      auto const methodOrMember =
        *(staticKeyword [handleStaticMember] | constKeyword [handleConstMember] | virtualKeyword [handleVirtualMember] ) >>
	enhancedIdIdGrammar >> *(x3::char_('=') >> *(x3::char_ - x3::char_(';'))) >>
        -x3::char_(';') [handleMemberFound] | (
             parameterGrammar  >>
             *(overrideKeyword [handleVirtualMember] |
               constKeyword [handleConstMember]) >>
             -(x3::char_(';') | ignoreScopes)[handleMethodFound]);
        
      auto const classGrammar =
        *annotation [handleAnnotation] >>
        (classKeyword | structKeyword [handleStructKeyword]) >>
	identifier [handleClassPush] >>
	-(x3::lit(":") >> +(privateClassParentGrammar | protectedClassParentGrammar | publicClassParentGrammar >> *x3::lit(","))) >>
	x3::lit("{") >>
	* (
           // For the purposes of this parser I can pretty much just ignore templates.
           // Template methods will still show up in methods
           annotation [handleAnnotation] |
           constructorDestructor |
           (templateKeyword >> templateGuts) |
	   (publicKeyword [handlePublicInClass] >> x3::lit(":")) |
	   (protectedKeyword [handleProtectedInClass] >> x3::lit(":")) |
	   (privateKeyword [handlePrivateInClass] >> x3::lit(":")) |          
	   methodOrMember
	   ) >>
	x3::lit("};") [handleClassPop];
	
		
      auto const programGrammar = * (
                                     includeGrammar |
                                     pragmaOnceGrammar |
                                     ignoreUsing |
				     scopePush |
				     namespaceGrammar |
				     enumGrammar |
				     enumClassGrammar |
				     templateClassGrammar |
				     classGrammar |
				     scopePop);
      
      auto const ignoreStuff =
	blockComment |
	singleLineComment |
	x3::ascii::space;

      // Parse all the things      
      return x3::phrase_parse(first, last,
			      programGrammar,
			      ignoreStuff,
			      result);      
    }

  };
  
  
  
}
