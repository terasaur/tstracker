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
#ifndef TORRENTDB_HPP_INCLUDED
#define TORRENTDB_HPP_INCLUDED

extern "C" {
#include "trackerlogic.h" // for ot_hash
#include <libowfat/socket.h> /* for uint16 */
}
#include "terasaur/torrent.hpp"
#include "libtorrent/peer_id.hpp" // for sha1_hash
#include <vector>
#include <mongo/client/dbclient.h> // mongodb client
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <mongo/client/connpool.h>
using boost::scoped_ptr;
using mongo::ScopedDbConnection;

using libtorrent::sha1_hash;
using std::string;
using mongo::BSONObj;

#include <map>
typedef std::map<ot_hash, bool> hash_map;

#include <boost/tuple/tuple.hpp>
typedef boost::tuples::tuple<string, uint16> string_int_tuple;

namespace terasaur {
namespace torrentdb {

struct stats_update_t {
    sha1_hash info_hash;
    uint32_t seeds;
    uint32_t peers;
    bool increment_completed;
    stats_update_t() : info_hash(0), seeds(0), peers(0), increment_completed(false) {}
};

// public declarations
void set_param(string key, string val);
bool init_conn();
boost::shared_ptr<terasaur::torrent> look_up_info_hash(sha1_hash const& info_hash);
std::vector<string_int_tuple> get_seedbanks(sha1_hash const& info_hash);
void update_stats(stats_update_t const& params);

// private declarations
BSONObj _look_up_info_hash(scoped_ptr<ScopedDbConnection> const& scoped_conn, sha1_hash const& info_hash);
std::vector<string_int_tuple> _get_seedbanks(scoped_ptr<ScopedDbConnection> const& scoped_conn, sha1_hash const& info_hash);
string_int_tuple _get_seedbank_for_id(scoped_ptr<ScopedDbConnection> const& scoped_conn, int seedbank_id);
BSONObj _look_up_seedbank(scoped_ptr<ScopedDbConnection> const& scoped_conn, int seedbank_id);
void _execute_update_stats(scoped_ptr<ScopedDbConnection> const& scoped_conn, stats_update_t const& params);

} // namespace torrentdb
} // namespace terasaur

#endif
