/**
 * Copyright 2013 ibiblio
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

#include "terasaur/log_util.hpp"
#include "terasaur/date.hpp"
#include <pthread.h>
#include <stdio.h>
#include <boost/date_time/posix_time/posix_time_io.hpp>

namespace terasaur {
namespace log_util {

using std::ostream;

// static locale
boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S.%f");
std::locale _time_locale(std::wcout.getloc(), facet);

/**
 * Get local datetime, formatted for log messages
 */
std::string _get_now() {
    boost::posix_time::ptime now = terasaur::date::get_now_local();
    std::stringstream ss;
    ss.imbue(_time_locale);
    ss << now;
    return ss.str();
}

ostream& debug() {
    std::cout << _get_now() << " [" << pthread_self() << "] ";
    return std::cout;
}

ostream& error() {
#ifdef _DEBUG
    std::cout << _get_now() << " [" << pthread_self() << "] ";
#endif
    std::cout << "ERROR: ";
    return std::cout;
}

} // namespace terasaur
} // namespace log_util
