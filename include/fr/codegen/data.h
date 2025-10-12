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

#include <map>
#include <string>
#include <vector>
#include <fr/codegen/parser.h>

namespace fr::codegen {

  struct NamespaceEntry {
    // Namespace name
    std::string name;
    int scopeDepth; // ScopeDepth for this namespace
  };
  
  /**
   * Data for tracking the current namespace
   */

  class NamespaceData {
  public:
    // current scope depth
    int scopeDepth;
    std::vector<boost::signals2::connection> subscriptions;
    std::vector<NamespaceEntry> namespaceStack;

    void clear() {
      unsubscribe();
      scopeDepth = 0;
      namespaceStack.clear();
    };

    void unsubscribe() {
      for (auto subscription : subscriptions) {
	subscription.disconnect();
      }
      subscriptions.clear();
    }

    // Run on scope pop, finds all namespaces with scope greater than
    // current scope and removes them from the stack.
    void cleanUpNamespaces() {
      while(namespaceStack.size() > 0 && namespaceStack.back().scopeDepth >= scopeDepth) {	
	namespaceStack.pop_back();
      }
    }
    
    // Register with Parser
    void regParser(::fr::codegen::parser::ParserDriver &parser) {
      clear();
      auto scopePushSub = parser.incScope.connect([&](){ scopeDepth++; });
      auto scopePopSub = parser.decScope.connect([&](){
	scopeDepth--;
	cleanUpNamespaces();
      });
      auto namespacePushSub = parser.namespacePush.connect([&](const std::string& name, int depth){
	NamespaceEntry entry;
	entry.name = name;
	// When namespace is parsed, scope is 1 less than it will be inside the namespace, so
	// we add one to the depth for cleanup recognition purposes.
	entry.scopeDepth = depth + 1;
	namespaceStack.push_back(entry);
      });
      subscriptions.push_back(scopePushSub);
      subscriptions.push_back(scopePopSub);
      subscriptions.push_back(namespacePushSub);
    }
  };
  
  /**
   * Data for enums collected from the parser in parser.h
   *
   * TODO: Write a cereal archiver for enumdata so I can serialize it and not have to
   *       parse enums every time.
   */
  class EnumData {
  public:
    // A vector of namespace names. Concat them all together with :: to get the enum namespace.
    std::vector<std::string> namespaces;
    // Enum name
    std::string name;
    // Tracks if this is a class enum or an old timey one
    bool isClassEnum;
    // identifiers in the enum
    std::vector<std::string> identifiers;

    // Returns the C++ formatted namespace for this enum
    std::string enumNamespace() {
      std::string ret;
      for (auto n = namespaces.begin(); n != namespaces.end(); ++n) {
	if (ret.size()) {
	  ret.append("::");
	}
	ret.append(*n);
      }
      return ret;
    }
    
    void clear() {
      namespaces.clear();
      identifiers.clear();
      name.clear();
      isClassEnum = false;
    }
  };

  // Handles collecting enum and namespace data from the parser

  // Note that currently this code is not tracking classes, so if your enum is in a class
  // the generated code won't reference it correctly

  class EnumDriver {

    void copyNamespaces() {
      for (auto it = namespaces.namespaceStack.begin(); it != namespaces.namespaceStack.end(); ++it) {	
	currentEnum.namespaces.push_back(it->name);
      }
    }
    
    void handleStandardEnum(const std::string& name, int scopeDepth) {
      copyNamespaces();
      currentEnum.name = name;
    }

    void handleEnumClass(const std::string &name, int scopeDepth) {
      copyNamespaces();	     
      currentEnum.name = name;
      currentEnum.isClassEnum = true;
    }

    void handleEnumIdentifier(const std::string& enumName, const std::string& identifier) {
      currentEnum.identifiers.push_back(identifier);
    }
    
  public:
    NamespaceData namespaces;
    // Temporary storage for the current working enum
    EnumData currentEnum;
    std::vector<boost::signals2::connection> subscriptions;

    // Map of enums keyed by their fully qualified enum name (namespaces
    // included.)
    std::map<std::string, EnumData> enums;

    // Register with the parser
    void regParser(::fr::codegen::parser::ParserDriver &parser) {
      // Reset the current enum to default values
      currentEnum.clear();
      
      namespaces.regParser(parser);
      // This is triggered when an enum (not enum class) is encountered
      auto enumSub = parser.enumPush.connect([&](const std::string &name, int scopeDepth) {
	handleStandardEnum(name, scopeDepth);
      });
      auto enumClassSub = parser.enumClassPush.connect([&](const std::string &name, int scopeDepth) {
	handleEnumClass(name, scopeDepth);
      });
      // Due to the way the parser is structured, this will always run after we've set up a
      // currentEnum.
      auto enumIdentifierSub = parser.enumIdentifier.connect([&](const std::string &enumName,
								 const std::string &identifier) {
	handleEnumIdentifier(enumName, identifier);
      });
      // We need to check if we're in an enum whenever we see a scope pop and
      // finalize our enum if we were working on one. There's nothing magic about scope
      // pops after enums, it's just a standard decScope, so we need to check whenever we receive one.

      // Note we don't have to do anything with namespaces though, as the namespace class also subscribes
      // to the parser signal and does its own scopeDec handling.
      auto enumScopePopSub = parser.decScope.connect([&] {
	if (!currentEnum.name.empty()) {
	  // Construct key for the map
	  std::string namespaceKey;
	  for (auto it = currentEnum.namespaces.begin(); it != currentEnum.namespaces.end(); ++it) {
	    namespaceKey.append(*it);
	    namespaceKey.append("::");
	  }
	  namespaceKey.append(currentEnum.name);
	  enums[namespaceKey] = currentEnum;
	  currentEnum.clear();
	}
      });
      subscriptions.push_back(enumSub);
      subscriptions.push_back(enumClassSub);
      subscriptions.push_back(enumIdentifierSub);
      subscriptions.push_back(enumScopePopSub);
    }

    // You have to be careful to unsubscribe with boost signals2 -- if this object
    // goes out of scope with subscriptions enabled, you could get segfaults on
    // future attempts to run the parser. So it's generally a good idea to
    // track subscriptions, have an unsubscribe method like this and call it from
    // the destructor.
    void unsubscribe() {
      for (auto subscription : subscriptions) {
	subscription.disconnect();
      }
      subscriptions.clear();
    }

    // Clear all the storage -- handy to reset the object for another parser run
    void clear() {
      unsubscribe();
      namespaces.clear();
      currentEnum.clear();
      enums.clear();
    }
    
  };
  
}
