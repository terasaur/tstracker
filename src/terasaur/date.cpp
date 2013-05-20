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

#include "terasaur/date.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>

using boost::posix_time::ptime;
using boost::posix_time::time_duration;

namespace terasaur {
namespace date {

ptime get_now_utc() {
    return boost::posix_time::microsec_clock::universal_time();
}

ptime get_now_local() {
    return boost::posix_time::microsec_clock::local_time();
}

ptime get_epoch() {
    return ptime(boost::gregorian::date(1970,1,1));
}

/**
 * Returns current datetime in UTC for use in a mongodb query.
 *
 * This is a complicated way of going about getting milliseconds since epoch, but
 * it's a reliable way to get UTC instead local time.
 */
mongo::Date_t get_now_mongo() {
    ptime epoch = get_epoch();
    time_duration const diff = get_now_utc() - epoch;
    uint64_t now_ms = diff.total_milliseconds();
    mongo::Date_t mongo_now = mongo::Date_t(now_ms);
    return mongo_now;
}

} // namespace date
} // namespace terasaur
