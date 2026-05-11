/**
 * Copyright 2026 Bruce Ide
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
 * This code contains filters that emit a nanobind Python API for
 * a class or set of classes in a header file.
 */


#pragma once
#include <algorithm>
#include <boost/signals2.hpp>
#include <fr/codegen/data.h>
#include <fr/codegen/LblFilter.h>
#include <fr/codegen/LblMiniParser.h>
#include <fr/codegen/LblEmitFunctions.h>
#include <iostream>
#include <stdexcept>

namespace fr::codegen {

  /**
   * When this class encounters a [[StartModule (ModuleName)]] it will emit
   * a NB_MODULE with that name. The annotation model seems to work reasonably
   * well for code generation, so I'm just going to run with it.
   */

  class LblEmitModuleStart : public LblMiniParserFilter {

    void emitModuleStart(const std::string moduleName) {
      std::string out;
      startModule(moduleName);
      out.append(" NB_MODULE(");
      out.append(moduleName);
      out.append(", m) {");
      emit(out);
    }

  public:
    LblEmitModuleStart(const ClassMap &classes) : LblMiniParserFilter(classes) {}
    virtual ~LblEmitModuleStart() = default;
   
    boost::signals2::signal<void(const std::string&)> startModule;
    
    void process(const std::string &line) override {
      // Look for StartModule annotation
      std::string lineCopy = line;
      std::string moduleName;
      // Remove all whitespace from line
      lineCopy.erase(std::remove_if(lineCopy.begin(),
                                    lineCopy.end(),
                                    ::isspace),
                     lineCopy.end());
      if (lineCopy.find("[[StartModule") != std::string::npos) {
        auto paren = lineCopy.find("(");
        auto unparen = lineCopy.find(")");
        if (std::string::npos == paren ||
            std::string::npos == unparen) {
          throw std::runtime_error("Mismatched or no parantheses found in [[StartModule]] annotation -- [[StartModule]] must contain \"(ModuleName)\" so I know what to name the module.");          
        }
        if (unparen < paren) {
          // You know, I should just fix it...
          std::swap(paren, unparen);
        }
        // Reads from 1 past the "(" to 1 before the ")"
        moduleName = lineCopy.substr(paren + 1, (unparen - paren) - 1);
        emitModuleStart(moduleName);
      } else {
        emit(line);
      }
    }
  };

  /**
   * This iterates through all the classes in ClassMap and emits a nanobind
   * class_ for each one. It's triggered by [[PythonApi]].
   */
  
  class LblEmitPythonApi : public LblMiniParserFilter {

    void emitClassHeader(std::shared_ptr<ClassData> data) {
      processingClass(data->name);
      std::string out("  nanobind::class_<");
      out.append(data->name);
      // Nanobind only allows single inheritance chains
      // so I'm only going to check for one and use parents[0]
      // if I find any.
      if (data->parents.size() > 0) {
        out.append(", ");
        out.append(data->parents[0]);
      }
      out.append(">(m, \"");
      out.append(data->name);
      out.append("\")");
      emit(out);
    }

    void emitConstructor(const MethodData& method) {
      std::string out("    .def(nanobind::init<");
      // First parameter -- we'll use this to move comma emission
      // to the beginning of the for loop for parameters after the first
      // one
      processingConstructor();
      bool first = true;
      for(const auto &parameter : method.parameters) {
        if (!first) {
          out.append(", ");
        } else {
          first = false;
        }
        if (parameter.typeConst) {
          out.append("const ");
        }
        // Note that if you have * or & on the parameter name instead of the parameter
        // type, this will get it wrong. I should go back and clean that up at some point.        
        out.append(parameter.type);        
      }
      out.append(">())");
      emit(out);
    }
    
    void emitMethods(std::shared_ptr<ClassData> data) {
      for (const auto &method : data->methods) {
        std::string out;

        // We only do public ones
        if (method.isPublic) {
          if (method.name == "constructor") {
            emitConstructor(method);
          } else if (method.name == "destructor") {
            // Ignore
          } else {
            processingMethod(method.name);
            out.append("    .def(\"");
            out.append(method.name);
            out.append("\", ");
            // We're just overload casting all the functions whether
            // they have overloads or not. This should be fine.
            bool first = true;
            out.append("nanobind::overload_cast<");
            for (const auto &parameter : method.parameters) {
              if (!first) {
                out.append(", ");
              } else {
                first = false;
              }
              if (parameter.typeConst) {
                out.append("const ");
              }
              out.append(parameter.type);
            }
            out.append(">(&");
            // Class name
            out.append(data->name);
            out.append("::");
            out.append(method.name);
            out.append("))");
            emit(out);
          }
        }
      }
    }

    void emitMembers(std::shared_ptr<ClassData> data) {
      for (const auto &member : data->members) {
        std::string out;

        if (member.isPublic && !member.isStatic) {
          processingMember(member.name);
          out.append("    .def_rw(\"");
          out.append(member.name);
          out.append("\", ");
          out.append("&");
          // class name
          out.append(data->name);
          out.append("::");
          out.append(member.name);
          out.append(")");
          emit(out);
        }
      }
    }

    void emitEndClass() {
      // This just goes at the end of a bunch of .defs.
      emit("    ;");
    }

    void emitAllClasses() {
      for (auto [name, data] : _classes) {
        emitClassHeader(data);
        // Todo: emitMethods handle static methods correctly
        emitMethods(data);
        emitMembers(data);
        emitEndClass();
        emit("\n");
      }
    }

  public:
    LblEmitPythonApi(const ClassMap &classes) : LblMiniParserFilter(classes) {}
    virtual ~LblEmitPythonApi() = default;

    boost::signals2::signal<void(const std::string&)> processingClass;
    boost::signals2::signal<void()> processingConstructor;
    boost::signals2::signal<void(const std::string&)> processingMethod;
    boost::signals2::signal<void(const std::string&)> processingMember;

    void process(const std::string& line) override {
      // Look for tag
      std::string lineCopy = line;
      lineCopy.erase(std::remove_if(lineCopy.begin(),
                                    lineCopy.end(),
                                    ::isspace),
                     lineCopy.end());
      if (lineCopy == "[[PythonApi]]") {
        emitAllClasses();
      } else {
        emit(line);
      }
    }
    
  };
  
}
