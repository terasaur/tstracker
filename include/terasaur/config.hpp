/**
 * Copyright 2012 ibiblio
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

// global config options
// TODO: refactor this into separate class
#include <map>
#include <string>

using std::map;
using std::string;

namespace terasaur {
namespace config {
    void parse_file(string config_file);
    string get_value(string const& key);
    void set_ot_global(char **option, string const& val_string);
} // namespace config
} // namespace terasaur

#endif
