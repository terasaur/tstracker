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
#ifndef STRING_MAP_HPP_INCLUDED
#define STRING_MAP_HPP_INCLUDED

#include <map>
#include <string>

namespace terasaur {

typedef std::map<std::string, std::string> string_map;
typedef string_map::const_iterator string_map_iter;

} // namespace terasaur

#endif




