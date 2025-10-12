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
 * Here are a couple of silly little example enums which we will automatically
 * generate operators for.
 */

#pragma once

// Classic example
enum Colors {
  red, green, blue
};

namespace animals {
  enum Animals {
    dog, cat, llama
  };
}

namespace foo::bar {

  enum class Trees {
    /* No. 1 */ theLarch,
      /* No. 3 */ larch, Larch,
      theHorseChestnut
  };
  
}
