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
#ifndef TORRENT_HPP_INCLUDED
#define TORRENT_HPP_INCLUDED

#include <mongo/bson/bson.h> // for Date_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include "libtorrent/peer_id.hpp" // for sha1_hash

using boost::posix_time::ptime;

namespace terasaur {

// Used to pass data out of torrentdb
struct torrent {
    libtorrent::sha1_hash info_hash;
    ptime published;

    torrent();
    torrent(mongo::BSONObj const& torrent_record);
    virtual ~torrent() {}
    bool is_published();
};

} // namespace terasaur

#endif
