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
 * Line by line processing for fun and profit.
 */

#pragma once

#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fr::codegen {

  /**
   * This class provides an emit callback which will contain one line of
   * data to emit. You can chain multiple emitters along.
   */
  
  class LblEmitter {
  public:
    boost::signals2::signal<void(const std::string&)> emit;   
    LblEmitter() = default;
    virtual ~LblEmitter() = default;
  };

  /**
   * This class provides a subscribe method to subscribe to
   * an emitter. You can MI LblSubscriber and LblEmitter
   * if you want, which could be handy if you're building
   * a chain of filters.
   */

  class LblSubscriber {
  protected:
    std::vector<boost::signals2::connection> _subscriptions;
  public:
    LblSubscriber() = default;
    virtual ~LblSubscriber() { unsubscribe(); }

    /**
     * Override process to process strings received from
     * the emitter.
     */

    virtual void process(const std::string& toProcess) = 0;

    /**
     * Drop all subscriptions
     */

    void unsubscribe() {
      for(auto subscription : _subscriptions) {
        subscription.disconnect();
      }
      _subscriptions.clear();
    }

    /**
     * subscribeTo methods for raw pointers, references,
     * shared and unique ptrs to emitters
     *
     */

    virtual void subscribeTo(LblEmitter* emitter) {
      auto subscription = emitter->emit.connect([&](const std::string& toProcess) {
        process(toProcess);
      });
      _subscriptions.push_back(subscription);
    }

    virtual void subscribeTo(LblEmitter& emitter) {
      subscribeTo(&emitter); // degrade to raw ptr version
    }

    virtual void subscribeTo(const std::unique_ptr<LblEmitter>& ptr) {
      subscribeTo(ptr.get());
    }

    virtual void subscribeTo(std::shared_ptr<LblEmitter> ptr) {
      subscribeTo(ptr.get());
    }
  };

  /**
   * Reads a file line by line when process() is called.
   */

  class LblReader : public LblEmitter {
    std::string _filename;

  public:
    LblReader(const std::string& filename) : _filename(filename) {      
    }

    ~LblReader() = default;

    void process() {
      std::ifstream input(_filename);
      std::string line;
      while (std::getline(input,line)) {
        emit(line);
      }
    }
  };

  /**
   * Generic filter -- Subscribes to an emitter and has an emitter.
   * This rejoins LblEmitter and LblSubscriber so don't inherit either
   * of those from a class that inherits from this.
   *
   * You must override the process method of any object that inherits
   * from this. This is inherited from LblSubscriber.
   */

  class LblFilter : public LblEmitter, public LblSubscriber {
  public:
    LblFilter() = default;
    virtual ~LblFilter() = default;
  };

  /**
   * Writer, the end of the road
   *
   * This should be the end of a chain of emitters and filters
   * It will just write any emits that find their way to
   * it to a file.
   */

  class LblWriter : public LblSubscriber {
    std::string _filename;
    std::ofstream stream;
  public:
    LblWriter(const std::string& filename) : _filename(filename), stream(filename) {
    }

    virtual ~LblWriter() = default;

    void process(const std::string& line) {
      stream << line << std::endl;
    }
  };
  
}
