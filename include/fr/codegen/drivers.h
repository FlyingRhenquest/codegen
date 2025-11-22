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
 * This file contains drivers which register for signals from parser.h
 * and use them to accumulate data into the structures in data.h.
 */

#pragma once
#include <fr/codegen/data.h>

namespace fr::codegen {

  /**
   * Driver to handle namespace information
   */

  class NamespaceDriver {
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
  
  // Handles collecting enum and namespace data from the parser
  
  // We're not checking inside classes for enums, so if your enum is in a class
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
    boost::signals2::signal<void(const std::string&, const EnumData&)> enumAvailable;
    NamespaceDriver namespaces;
    // Temporary storage for the current working enum
    EnumData currentEnum;
    std::vector<boost::signals2::connection> subscriptions;

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
          enumAvailable(namespaceKey, currentEnum);
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
    }
    
  };

  /**
   * Class driver -- this collects data on classes the parser encounters, just like
   * EnumDriver does for enums.
   */

  class ClassDriver {

    void nowPrivate() {
      isPrivate = true;
      isPublic = false;
      isProtected = false;
    }

    void nowPublic() {
      isPrivate = false;
      isPublic = true;
      isProtected = false;
    }

    void nowProtected() {
      isPrivate = false;
      isPublic = false;
      isProtected = true;
    }
    
    void copyNamespaces() {
      for (auto it = namespaces.namespaceStack.begin(); it != namespaces.namespaceStack.end(); ++it) {
	currentClass.namespaces.push_back(it->name);
      }
    }

  public:
    // Expose a singal that subscribers can subscribe to
    // Callback parameters are
    // string - Fully qualified name of class
    // ClassData - Copy of the class data found
    boost::signals2::signal<void(const std::string&, const ClassData&)> classAvailable;
    
    std::vector<boost::signals2::connection> subscriptions;
    NamespaceDriver namespaces;
    ClassData currentClass;
    bool isPublic;
    bool isPrivate;
    bool isProtected;
    // Indicates that we're currently parsing a class or struct
    bool inClass;
    // Some method flags that an annotation might trigger
    bool generateGetter;
    bool generateSetter;
    // Method-specific serialization flag
    bool serializable;

    void handleClass(const std::string&name, int scopeDepth) {
      copyNamespaces();
      currentClass.name = name;
      nowPrivate();
      currentClass.isStruct = false;
      inClass = true;
    }

    void handleStruct(const std::string& name, int scopeDepth) {
      copyNamespaces();
      currentClass.name = name;
      nowPublic();
      currentClass.isStruct = true;
      inClass = true;
    }

    void handlePublic() {
      nowPublic();
    }

    void handlePrivate() {
      nowPrivate();
    }

    void handleProtected() {
      nowProtected();
    }

    void handleClassPop() {
      classAvailable(currentClass.fullClassName(), currentClass);
      currentClass.clear();
      inClass = false;
    };    

    void handleMemberFound(bool inClassConst, bool inClassStatic, const std::string& type, const std::string& name) {
      MemberData md;
      md.type = type;
      md.name = name;
      md.isPublic = isPublic;
      md.isProtected = isProtected;
      md.isConst = inClassConst;
      md.isStatic = inClassStatic;
      md.serializable = serializable;
      md.generateGetter = generateGetter;
      md.generateSetter = generateSetter;
      currentClass.members.push_back(md);
      serializable = false; // Reset for next method
      generateGetter = false;
      generateSetter = false;
    }

    void handleMethodFound(bool inClassConst, bool inClassStatic, bool inClassVirtual, const std::string& type, const std::string& name) {
      MethodData md;
      md.returnType = type;
      md.name = name;
      md.isPublic = isPublic;
      md.isProtected = isProtected;
      md.isConst = inClassConst;
      md.isStatic = inClassStatic;
      md.isVirtual = inClassVirtual;
      currentClass.methods.push_back(md);
    }

    /**
     * Some annotation-specific keywords we look for
     * cereal (before class keyword) - All members in the class should be marked serializable
     * cereal (before member definition) - This member should be part of autogenerated serialziation functions
     * get (before member definition) - Autogenerate a getter for this member
     * set (before member definition) - Autogenerate a setter for this member
     *
     * Since I'm just finding in string text, you can do stuff like
     * [[cereal,get,set]] int thingy; and all three flags will get set. Or you
     * can do [[cereal]] [[get]] int thingy; if you prefer, but why would you?
     *
     * If the class is tagged with [[cereal]], all members will be included in autogenerated serialization
     * functions, so you don't need to tag individual members too.
     *
     * These only request this functionality. The tags to actually invoke it are
     * [[generateCerealArchiver]] or [[generateCerealSave]] and [[generateCerealLoad]] to
     * generate cereal functions and [[generatePythonAPI]] and [[generateEmscriptenAPI]] to
     * generate APIs. These should be on lines by themselves. Refer to the README and
     * examples for details.
     */
    
    void handleAnnotation(const std::string& text) {
      if (text.find("cereal") != std::string::npos) {
        if (!inClass) {
          // Mark the whole class as serializable (currentClass doesn't get reset prior to the classPop,
          // so this is legit if mildly sketchy
          currentClass.serializable = true;
        } else {
          // Next method is serializable
          serializable = true;
        }
      }
      if (text.find("get") && inClass) {
        generateGetter = true;
      }
      if (text.find("set") && inClass) {
        generateSetter = true;
      }
    }

    // Register with parser
    void regParser(::fr::codegen::parser::ParserDriver& parser) {
      clear();
      currentClass.clear();
      namespaces.regParser(parser);
      inClass = false;
      auto classPushSub = parser.classPush.connect([&](const std::string& name, int scopeDepth) {
	handleClass(name, scopeDepth);
      });
      auto classPopSub = parser.classPop.connect([&]() {
        handleClassPop();
      });
      auto structPushSub = parser.structPush.connect([&](const std::string& name, int scopeDepth) {
	handleStruct(name, scopeDepth);
      });
      auto privateClassParentSub = parser.privateClassParent.connect([&](const std::string& name) {
	currentClass.parents.push_back(name);      
      });
      auto protectedClassParentSub = parser.protectedClassParent.connect([&](const std::string& name) {
	currentClass.parents.push_back(name);
      });
      auto publicClassParentSub = parser.publicClassParent.connect([&](const std::string& name) {
	currentClass.parents.push_back(name);
      });
      auto privateInClassSub = parser.privateInClass.connect([&](){ handlePrivate(); });
      auto protectedInClassSub = parser.protectedInClass.connect([&]() { handleProtected(); });
      auto publicInClassSub = parser.publicInClass.connect([&]() {handlePublic(); });
      auto memberFoundSub = parser.memberFound.connect([&](bool inClassConst, bool inClassStatic, const std::string& type, const std::string& name) {
	handleMemberFound(inClassConst, inClassStatic, type, name);
      });
      auto methodFoundSub = parser.methodFound.connect([&](bool inClassConst, bool inClassStatic, bool inClassVirtual, const std::string& type, const std::string& name) {
	handleMethodFound(inClassConst, inClassStatic, inClassVirtual, type, name);
      });
      auto annotationFoundSub = parser.annotationFound.connect([&](const std::string& annotationText) {
        handleAnnotation(annotationText);
      });
      subscriptions.push_back(classPushSub);
      subscriptions.push_back(structPushSub);
      subscriptions.push_back(privateClassParentSub);
      subscriptions.push_back(protectedClassParentSub);
      subscriptions.push_back(publicClassParentSub);
      subscriptions.push_back(privateInClassSub);
      subscriptions.push_back(protectedInClassSub);
      subscriptions.push_back(publicInClassSub);
      subscriptions.push_back(memberFoundSub);
      subscriptions.push_back(methodFoundSub);
      subscriptions.push_back(annotationFoundSub);
    }

    void unsubscribe() {
      for (auto subscription : subscriptions) {
	subscription.disconnect();
      }
      subscriptions.clear();
    }

    void clear() {
      unsubscribe();
      namespaces.clear();
      currentClass.clear();
      nowPrivate();
      inClass = false;
      generateGetter = false;
      generateSetter = false;
      serializable = false;
    }
    
  };


}
