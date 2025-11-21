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

  /**
   * MethodData -- Stores info on one class/struct method
   *
   * * Return Type
   * * Method Name
   * * public flag - True If method is public
   * * protected flag - True if method is protected
   * * Virtual flag - If method is listed as virtual or is overridden
   * * Const flag - True if const
   *
   * Templated methods will currently be ignored.
   */


  struct MethodData {
    std::string returnType;
    std::string name;
    bool isPublic;
    bool isProtected;
    bool isVirtual;
    bool isConst;
    bool isStatic;
  };

  /**
   * MemberData -- Stores info on one member
   * * Type
   * * MemberName
   * * public flag - True if public
   * * protected flag - True if protected
   * * Const flag - True if const
   * * Static flag - true if static
   * * Ref flag - True if reference
   * * Ptr flag - True if pointer
   *
   * Notes: Shared/Unique pointers will have that for their type but ptr/ref
   *        flags will not be set unless they're pointers/references to shared/unique
   *        pointers.
   *
   *        I may decide to only support one level of pointer indirection.
   */

  struct MemberData {
    std::string type;
    std::string name;
    bool isPublic;
    bool isProtected;
    bool isConst;
    bool isStatic;
  };

  /**
   * ClassData - Stores info on a struct/class
   * * Namespaces vector (Same as enum)
   * * Class name
   * * Class parents - Vector of strings, empty if none (does not pay attention to public/protected/private)
   * * Vector of MethodData
   * * Vector of MemberData
   * * bool isStruct - True if struct
   */

  struct ClassData {
    std::vector<std::string> namespaces;
    std::string name;
    std::vector<std::string> parents;
    std::vector<MethodData> methods;
    std::vector<MemberData> members;
    bool isStruct;

    // Yes, this is the same function as EnumNamespace
    // and yes, I am OK with that. If I need one more
    // I'll refactor into a namespace object
    std::string classNamespace() {
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
      name.clear();
      parents.clear();
      methods.clear();
      members.clear();
      isStruct = false;
    }
  };


}
