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

#include "terasaur/torrent.hpp"
#include "terasaur/log_util.hpp"
#include "terasaur/date.hpp"
#include "libtorrent/escape_string.hpp" // from_hex

namespace terasaur {

torrent::torrent()
    : info_hash(0),
      published() {}

/**
 * Initialize from mongodb query results
 */
torrent::torrent(mongo::BSONObj const& torrent_record) {
    if (!torrent_record["info_hash"].ok()) {
        log_util::error() << "torrent::torrent(BSONObj): invalid torrent record" << std::endl;
        torrent();
        return;
    }

    std::string ih_hex = torrent_record["info_hash"].str();
    if (!libtorrent::from_hex(ih_hex.c_str(), 40, (char*)&info_hash[0])) {
        log_util::error() << "torrent::torrent(BSONObj): error converting hex to sha1_hash" << std::endl;
    }

    // Convert mongodb to boost ptime
    mongo::Date_t mongo_pub = torrent_record["published"].Date();
    uint64_t millis = mongo_pub.millis;
    published = boost::posix_time::from_time_t(millis / 1000);
}

/**
 * Returns true if published datetime is before the current time.
 */
bool torrent::is_published() {
    boost::posix_time::time_duration const diff = date::get_now_utc() - published;
    return !diff.is_negative();
}

} // namespace terasaur
