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
 * This code contains filters that emit lines they receive unless
 * they hit specific attributed keywords on a line by themselves.
 * If one of those lines is encountered, the filter will instead
 * emit all the functions necessary for that class.
 *
 * These functions need to subscribe to a lblMiniParser's
 * classPush and classPop methods, so they know what class they're
 * currently in. They also need to forward those signals on
 * in case downstream classes want to subscribe to them too.
 */

#pragma once
#include <algorithm>
#include <cctype>
#include <fr/codegen/data.h>
#include <fr/codegen/LblFilter.h>
#include <fr/codegen/LblMiniParser.h>
#include <iostream>
#include <map>
#include <string>

namespace fr::codegen {

  using ClassMap = std::map<std::string, std::shared_ptr<ClassData>>;

  // Base classes for other things that need to know class name
  // to subscribe to. All of these need to accept a ClassMap
  // as defined in IndedCode.cpp

  class LblMiniParserFilter : public LblFilter {
  protected:
    ClassMap _classes;
    std::shared_ptr<ClassData> _currentClass;
  public:
    
    boost::signals2::signal<void(const std::string&)> classPush;
    boost::signals2::signal<void()> classPop;
    
    LblMiniParserFilter(const ClassMap& classes) {
      // Reindex classes based on class name instead of fully qualified
      // name, as the lbl processor isn't looking at namespace.
      for(auto [name, classData] : classes) {
        _classes[classData->name] = classData;
      }
    }

    // A parent unsubscribes so we don't need to
    virtual ~LblMiniParserFilter() = default;

    // Some more subscribeTo functions. Since these don't have the
    // same parameter type as the previous ones, they should be
    // new functions and subscribing to emitters should still
    // work correctly for things that don't have a
    // classPush and classPop
    virtual void subscribeTo(fr::codegen::miniparser::LblMiniparser* emitter) {
      auto subscription = emitter->emit.connect([&](const std::string& toProcess) {
        process(toProcess);
      });
      auto classPushSub = emitter->classPush.connect([&](const std::string& className) {
        handleClassPush(className);
      });
      auto classPopSub = emitter->classPop.connect([&]() {
        handleClassPop();
      });
      _subscriptions.push_back(subscription);
      _subscriptions.push_back(classPushSub);
      _subscriptions.push_back(classPopSub);
    }

    // Since the LblMiniParser isn't a MiniParserFilter, I have to do the same
    // thing for MiniParserFilter (I could also do this with
    // concepts but the chains I'm building aren't really big enough
    // to write some.)

    virtual void subscribeTo(LblMiniParserFilter* emitter) {
      auto subscription = emitter->emit.connect([&](const std::string& toProcess) {
        process(toProcess);
      });
      auto classPushSub = emitter->classPush.connect([&](const std::string& className) {
        handleClassPush(className);
      });
      auto classPopSub = emitter->classPop.connect([&]() {
        handleClassPop();
      });
      _subscriptions.push_back(subscription);
      _subscriptions.push_back(classPushSub);
      _subscriptions.push_back(classPopSub);
    }
    
    virtual void subscribeTo(fr::codegen::miniparser::LblMiniparser& emitter) {
      subscribeTo(&emitter);
    }

    virtual void subscribeTo(std::shared_ptr<fr::codegen::miniparser::LblMiniparser> emitter) {
      subscribeTo(emitter.get());
    }

    virtual void subscribeTo(const std::unique_ptr<fr::codegen::miniparser::LblMiniparser> emitter) {
      subscribeTo(emitter.get());
    }

    virtual void subscribeTo(std::shared_ptr<LblMiniParserFilter> emitter) {
      subscribeTo(emitter.get());
    }

    virtual void subscribeTo(const std::unique_ptr<LblMiniParserFilter>& emitter) {
      subscribeTo(emitter.get());
    }

    virtual void subscribeTo(LblMiniParserFilter& emitter) {
      subscribeTo(&emitter);
    }

    // Handle class push from previopus filter
    void handleClassPush(const std::string& className) {
      if (_classes.contains(className)) {
        _currentClass = _classes[className];
      } else {
        std::cerr << "WARNING: Class " << className << " was not found in class data" << std::endl;
      }
      // Forward the signal to the next thing in the filter chain
      classPush(className);
    }

    // Handle class pop from previous filter
    void handleClassPop() {
      _currentClass.reset();
      classPop();
    }    
    
  };

  /**
   * When this class encounters a [[genGetSetMethods]] on a line by
   * itself , it will NOT emit that line and instead will emit all the
   * get/set methods for the current class.
   */
  
  class LblEmitGetSetMethods : public LblMiniParserFilter {
  public:

    LblEmitGetSetMethods(const ClassMap& classes) : LblMiniParserFilter(classes) {}

    virtual ~LblEmitGetSetMethods() = default;

    void emitGetMethods() {
      for (auto member : _currentClass->members) {
        if (member.generateGetter) {
          std::string out(member.type);
          out.append(" get");
          out.append(member.name);
          out.append("() const { return ");
          out.append(member.name);
          out.append("; }");
          emit(out);
        }
      }
    }

    void emitSetMethods() {
      for (auto member : _currentClass->members) {
        if (member.generateSetter) {
          std::string out("void set");
          out.append(member.name);
          out.append("(const ");
          out.append(member.type);
          out.append("& val) ");
          out.append("{ ");
          out.append(member.name);
          out.append(" = val; }");
          emit(out);
        }
      }
    }
    
    void process(const std::string& line) override {
      // Look for tag
      std::string lineCopy = line;
      // Remove all whitespace from line
      lineCopy.erase(std::remove_if(lineCopy.begin(),
                                    lineCopy.end(),
                                    ::isspace),
                     lineCopy.end());
      if (lineCopy == "[[genGetSetMethods]]") {
        if (_currentClass) {
          emitGetMethods();
          emitSetMethods();
        } else {
          std::cerr << "WARNING: [[genGetSetMethods]] encountered, but not in a class" << std::endl;
        }
      } else {
        emit(line);
      }
    }
  };

  /**
   * When this class encounters [[genCerealLoadSave]] on a line by
   * itself, it will NOT emit that line and will instead emit cereal
   * load/save functions for any members tagged with [[cereal]] or
   * all the members if the class is tagged with [[cereal]]
   */

  class LblEmitCerealMethods : public LblMiniParserFilter {
  public:

    LblEmitCerealMethods(const ClassMap& classes) : LblMiniParserFilter(classes) {}
    virtual ~LblEmitCerealMethods() = default;

    void emitSaveMethod() {
      emit("template <typename Archive>");
      emit("void save(Archive& ar) const {");
      for(auto member : _currentClass->members) {
        if (member.serializable | _currentClass->serializable) {
          // The make_nvp makes a nice text tag for your members
          // if you're serializing to json or xml.
          std::string out("ar(cereal::make_nvp(\"");
          out.append(member.name);
          out.append("\",");
          out.append(member.name);
          out.append("));");
          emit(out);
        }
      }
      emit("}");
    }

    void emitLoadMethod() {
      emit("template <typename Archive>");
      emit("void load(Archive& ar) {");
      for(auto member : _currentClass->members) {
        if (member.serializable | _currentClass->serializable) {
          // We don't need to use make_nvp on read though.
          std::string out("ar(");
          out.append(member.name);
          out.append(");");
          emit(out);
        }
      }
      emit("}");              
    }

    void process(const std::string& line) override {
      // Look for tag
      std::string lineCopy = line;
      // Remove all whitespace from line
      lineCopy.erase(std::remove_if(lineCopy.begin(),
                                    lineCopy.end(),
                                    ::isspace),
                     lineCopy.end());
      if (lineCopy == "[[genCerealLoadSave]]") {
        if (_currentClass) {
          emitSaveMethod();
          emitLoadMethod();
        } else {
          std::cerr << "WARNING: [[genCerealLoadSave]] encountered, but not in a class" << std::endl;
        }
      } else {
        emit(line);
      }
    }
    
  };
  
}
