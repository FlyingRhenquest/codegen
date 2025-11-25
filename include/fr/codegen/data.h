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

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
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
    // Filename this enum is defined in (Must be set in EnumDriver)
    std::string definedIn;
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

    // Cereal serialziation functions
    
    template <typename Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp("namespaces", namespaces));
      ar(cereal::make_nvp("name", name));
      ar(cereal::make_nvp("isClassEnum", isClassEnum));
      ar(cereal::make_nvp("definedIn", definedIn));
      ar(cereal::make_nvp("identifiers", identifiers));
    }

    template <typename Archive>
    void load(Archive& ar) {
      ar(namespaces);
      ar(name);
      ar(isClassEnum);
      ar(definedIn);
      ar(identifiers);
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

    // Cereal serialization functions
    
    template <typename Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp("returnType", returnType));
      ar(cereal::make_nvp("name", name));
      ar(cereal::make_nvp("isPublic", isPublic));
      ar(cereal::make_nvp("isProtected", isProtected));
      ar(cereal::make_nvp("isVirtual", isVirtual));
      ar(cereal::make_nvp("isConst", isConst));
      ar(cereal::make_nvp("isStatic", isStatic));
    }

    template <typename Archive>
    void load(Archive &ar) {
      ar(returnType);
      ar(name);
      ar(isPublic);
      ar(isProtected);
      ar(isVirtual);
      ar(isConst);
      ar(isStatic);
    }
    
  };

  /**
   * MemberData -- Stores info on one member
   * * Type
   * * MemberName
   * * public flag - True if public
   * * protected flag - True if protected
   * * Const flag - True if const
   * * Static flag - true if static
   * * serializable - A tag that indicates that we want to include this member in
   *                  serialization functions. If the class is marked serializable,
   *                  all the members in the class will be serializable and you don't
   *                  need to tag each member.
   * * generateGetter - Generate a getter function for this member
   * * generateSetter - Generate a setter function for this member
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
    bool serializable;
    bool generateGetter;
    bool generateSetter;

    template <typename Archive>
    void save(Archive& ar) const {
      ar(cereal::make_nvp("type", type));
      ar(cereal::make_nvp("name", name));
      ar(cereal::make_nvp("isPublic", isPublic));
      ar(cereal::make_nvp("isProtected", isProtected));
      ar(cereal::make_nvp("isConst", isConst));
      ar(cereal::make_nvp("isStatic", isStatic));
      ar(cereal::make_nvp("serializable", serializable));
      ar(cereal::make_nvp("generateGetter", generateGetter));
      ar(cereal::make_nvp("generateSetter", generateSetter));
    }

    template <typename Archive>
    void load(Archive& ar) {
      ar(type);
      ar(name);
      ar(isPublic);
      ar(isProtected);
      ar(isConst);
      ar(isStatic);
      ar(serializable);
      ar(generateGetter);
      ar(generateSetter);
    }
  };

  /**
   * ClassData - Stores info on a struct/class
   * * Namespaces vector (Same as enum)
   * * Class name
   * * Class parents - Vector of strings, empty if none (does not pay attention to public/protected/private)
   * * Vector of MethodData
   * * Vector of MemberData
   * * bool isStruct - True if struct
   * * serializable - A tag that indicates that we want to include this member in
   *                  serialization functions. If the class is marked serializable,
   *                  all the members will be serialized without needing to tag each
   *                  individual one.
   */

  struct ClassData {
    // Filename class is defined in
    std::string definedIn;
    std::vector<std::string> namespaces;
    std::string name;
    std::vector<std::string> parents;
    std::vector<MethodData> methods;
    std::vector<MemberData> members;
    bool isStruct;
    bool serializable;

    ClassData() : isStruct(false), serializable(false) {}

    std::string fullClassName() {
      std::string ret;
      for (auto& ns : namespaces) {
        ret.append(ns);
        ret.append("::");
      }
      ret.append(name);
      return ret;
    }

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
      serializable = false;
    }

    template <typename Archive>
    void save(Archive &ar) const {
      ar(cereal::make_nvp("definedIn", definedIn));
      ar(cereal::make_nvp("namespaces", namespaces));
      ar(cereal::make_nvp("name", name));
      ar(cereal::make_nvp("parents", parents));
      ar(cereal::make_nvp("methods", methods));
      ar(cereal::make_nvp("members", members));
      ar(cereal::make_nvp("isStruct", isStruct));
      ar(cereal::make_nvp("serializable", serializable));
    }

    template <typename Archive>
    void load(Archive &ar) {
      ar(definedIn);
      ar(namespaces);
      ar(name);
      ar(parents);
      ar(methods);
      ar(members);
      ar(isStruct);
      ar(serializable);
    }
  };


}
