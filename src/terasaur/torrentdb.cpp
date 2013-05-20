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

#include "terasaur/torrentdb.hpp"
#include "terasaur/string_map.hpp"
#include "terasaur/log_util.hpp"
#include "terasaur/date.hpp"
#include <vector>

using std::endl;
typedef std::map<int, string_int_tuple> seedbank_map_t;

using boost::scoped_ptr;
using mongo::ScopedDbConnection;
using mongo::DBClientBase;
using mongo::DBClientCursor;
using mongo::BSONObj;
using mongo::BSONElement;

namespace terasaur {
namespace torrentdb {

string_map _param_map;
void set_param(string key, string val) {
    _param_map[key] = val;
}

seedbank_map_t _seedbank_map;

/**
 * Perform initial connection and other tasks for startup sequence.
 */
bool init_conn() {
    bool okay = false;
    try {
        scoped_ptr<ScopedDbConnection> scoped_conn(ScopedDbConnection::getScopedDbConnection(_param_map["connection_string"]));
        DBClientBase* conn = scoped_conn->get();
        conn->ensureIndex(_param_map["torrentdb_ns"], mongo::fromjson("{info_hash:1}"), true);
        if (conn->getLastError().empty()) {
            okay = true;
        } else {
            log_util::error() << conn->getLastError() << endl;
        }
        scoped_conn->done();
    } catch (mongo::DBException &e) {
        log_util::error() << "mongodb connection failed: " << e.what() << endl;
    }
    return okay;
}

/**
 * Update the database with the given seed and peer counts.  If increment_completed is true,
 * also increment the completed downloads counter.  Data structure:
 *
 *  info_hash:
 *     seeds: <int>
 *     peers: <int>
 *     completed: <int>
 *     updated: <datetime>
 */
void update_stats(stats_update_t const& params) {
#ifdef _DEBUG
    log_util::debug() << "torrentdb::update_stats: start" << endl;
#endif

    scoped_ptr<ScopedDbConnection> scoped_conn(ScopedDbConnection::getScopedDbConnection(_param_map["connection_string"]));
    try {
        _execute_update_stats(scoped_conn, params);
    } catch (mongo::UserException &e) {
        log_util::error() << "(update_stats): bson exception: " << e.what() << endl;
    } catch (mongo::DBException &e) {
        log_util::error() << "(update_stats): mongodb exception: " << e.what() << endl;
    } catch (std::exception& e) {
        log_util::error() << "(update_stats): mongodb query failed: " << e.what() << endl;
    }
#ifdef _DEBUG
    log_util::debug() << "torrentdb::update_stats: calling scoped_conn.done()" << endl;
#endif
    scoped_conn->done();

#ifdef _DEBUG
    log_util::debug() << "torrentdb::update_stats: returning" << endl;
#endif
}

void _execute_update_stats(scoped_ptr<ScopedDbConnection> const& scoped_conn, stats_update_t const& params) {
    char ih_hex[41];
    to_hex((char const*)&params.info_hash[0], sha1_hash::size, ih_hex);

    DBClientBase* conn = scoped_conn->get();
    if (conn->isFailed()) {
        log_util::error() << "mongodb connection failed trying to update stats (torrent: " << ih_hex << ", inc_completed: " << params.increment_completed << ")" << endl;
    } else {
#ifdef _DEBUG
        log_util::debug() << "torrentdb::_execute_update_stats: Running mongodb update query (" << ih_hex << ")" << endl;
#endif

        mongo::Date_t now = terasaur::date::get_now_mongo();
        mongo::Query query = QUERY("info_hash" << ih_hex);
        BSONObj update_bson;

        if (params.increment_completed) {
#ifdef _DEBUG
        log_util::debug() << "torrentdb::_execute_update_stats: increment completed true" << endl;
#endif
            update_bson = BSON("$set" << BSON("seeds" << params.seeds << "peers" << params.peers << "updated" << now)
                               << "$inc" << BSON( "completed" << 1));
        } else {
#ifdef _DEBUG
        log_util::debug() << "torrentdb::_execute_update_stats: increment completed false" << endl;
#endif
            update_bson = BSON("$set" << BSON("seeds" << params.seeds << "peers" << params.peers << "updated" << now));
        }

#ifdef _DEBUG
        log_util::debug() << "torrentdb::_execute_update_stats: query: " << query << endl;
        log_util::debug() << "torrentdb::_execute_update_stats: update bson: " << update_bson << endl;
        log_util::debug() << "torrentdb::_execute_update_stats: calling mongodb update()" << endl;
#endif
        conn->update(_param_map["torrentdb_ns"], query, update_bson);

#ifdef _DEBUG
        log_util::debug() << "torrentdb::_execute_update_stats: calling mongodb getLastError()" << endl;
#endif
        string err = conn->getLastError();
        bool success = err.empty();
        if (success == false) {
            log_util::error() << "torrentdb::_execute_update_stats: mongodb update returned error (" << err << ")" << endl;
        }
#ifdef _DEBUG
        else {
            log_util::debug() << "torrentdb::_execute_update_stats: mongodb update successful" << endl;
        }
#endif
    }

#ifdef _DEBUG
    log_util::debug() << "torrentdb::_execute_update_stats: returning" << endl;
#endif
}

boost::shared_ptr<terasaur::torrent> look_up_info_hash(sha1_hash const& info_hash)
{
#ifdef _DEBUG
    log_util::debug() << "torrentdb::look_up_info_hash: start (" << info_hash << ")" << endl;
#endif
    terasaur::torrent* torrent = NULL;
#ifdef _DEBUG
    log_util::debug() << "torrentdb::look_up_info_hash: getting scoped_conn" << endl;
#endif
    scoped_ptr<ScopedDbConnection> scoped_conn(ScopedDbConnection::getScopedDbConnection(_param_map["connection_string"]));

    try {
        BSONObj torrent_record = _look_up_info_hash(scoped_conn, info_hash);
        torrent = new terasaur::torrent(torrent_record);
    } catch (mongo::UserException &e) {
        log_util::error() << "torrentdb::look_up_info_hash: bson exception: " << e.what() << endl;
    } catch (mongo::DBException &e) {
        log_util::error() << "torrentdb::look_up_info_hash: mongodb exception: " << e.what() << endl;
    } catch (std::exception& e) {
        log_util::error() << "torrentdb::look_up_info_hash: mongodb query failed: " << e.what() << endl;
    }

#ifdef _DEBUG
    log_util::debug() << "torrentdb::look_up_info_hash: calling scoped_conn->done()" << endl;
#endif
    scoped_conn->done();
#ifdef _DEBUG
    log_util::debug() << "torrentdb::look_up_info_hash: returning (" << info_hash << ")" << endl;
#endif
    return boost::shared_ptr<terasaur::torrent>(torrent);
}

BSONObj _look_up_info_hash(scoped_ptr<ScopedDbConnection> const& scoped_conn, sha1_hash const& info_hash)
{
    BSONObj torrent_record;
    char ih_hex[41];
    to_hex((char const*)&info_hash[0], sha1_hash::size, ih_hex);

    DBClientBase* conn = scoped_conn->get();
    if (conn->isFailed()) {
        log_util::error() << "torrentdb::_look_up_info_hash: mongodb connection failed" << endl;
    } else {
#ifdef _DEBUG
        log_util::debug() << "torrentdb::_look_up_info_hash: running mongodb query (" << ih_hex << ")" << endl;
#endif
        std::auto_ptr<DBClientCursor> cursor = conn->query(_param_map["torrentdb_ns"], QUERY("info_hash" << ih_hex));
        bool found_results = false;

        //if (conn->getLastError().empty()) {
            while (cursor->more()) {
                // TODO: verify no more than one record returned?
                torrent_record = cursor->next();
                found_results = true;
            }
        //}
#ifdef _DEBUG
        if (!found_results) {
            log_util::debug() << "torrentdb::_look_up_info_hash: torrent not found" << endl;
        }
#endif
    }
    return torrent_record;
}

// TODO: should this return list<tuple> instead?
std::vector<string_int_tuple> get_seedbanks(sha1_hash const& info_hash) {
#ifdef _DEBUG
    log_util::debug() << "torrentdb::get_seedbanks: start (" << info_hash << ")" << endl;
#endif

    std::vector<string_int_tuple> sb_list;
#ifdef _DEBUG
    log_util::debug() << "torrentdb::get_seedbanks: getting scoped_conn" << info_hash << endl;
#endif
    scoped_ptr<ScopedDbConnection> scoped_conn(ScopedDbConnection::getScopedDbConnection(_param_map["connection_string"]));

    try {
        sb_list = _get_seedbanks(scoped_conn, info_hash);
    } catch (mongo::UserException &e) {
        log_util::error() << "torrentdb::get_seedbanks: bson exception: " << e.what() << endl;
    } catch (mongo::DBException &e) {
        log_util::error() << "torrentdb::get_seedbanks: mongodb exception: " << e.what() << endl;
    } catch (std::exception& e) {
        log_util::error() << "torrentdb::get_seedbanks: mongodb query failed: " << e.what() << endl;
    }
#ifdef _DEBUG
    log_util::debug() << "torrentdb::get_seedbanks: calling scoped_conn->done()" << endl;
#endif
    scoped_conn->done();
    return sb_list;
}

std::vector<string_int_tuple> _get_seedbanks(scoped_ptr<ScopedDbConnection> const& scoped_conn, sha1_hash const& info_hash) {
#ifdef _DEBUG
    log_util::debug() << "torrentdb::_get_seedbanks: start (scoped_conn, " << info_hash << ")" << endl;
#endif
    std::vector<string_int_tuple> sb_list;
    BSONObj torrent_record;
    int seedbank_id;
    string_int_tuple ip_port;

    torrent_record = _look_up_info_hash(scoped_conn, info_hash);
    if (torrent_record["info_hash"].ok() && torrent_record["seedbanks"].ok() && !torrent_record["seedbanks"].isNull()) {
        std::vector<BSONElement> seedbanks = torrent_record["seedbanks"].Array();
        for (std::vector<BSONElement>::iterator iter = seedbanks.begin() ; iter != seedbanks.end(); ++iter) {
            seedbank_id = ((BSONElement)(*iter)).Int();
            ip_port = _get_seedbank_for_id(scoped_conn, seedbank_id);
        }
        sb_list.push_back(ip_port);
    }
#ifdef _DEBUG
    log_util::debug() << "torrentdb::_get_seedbanks: returning" << endl;
#endif
    return sb_list;
}

string_int_tuple _get_seedbank_for_id(scoped_ptr<ScopedDbConnection> const& scoped_conn, int seedbank_id) {
#ifdef _DEBUG
    log_util::debug() << "torrentdb::_get_seedbank_for_id: start (" << seedbank_id << ")" << endl;
#endif

    string_int_tuple ip_port;
    seedbank_map_t::iterator iter;
    iter = _seedbank_map.find(seedbank_id);

    if (iter == _seedbank_map.end()) {
        BSONObj sb_record = _look_up_seedbank(scoped_conn, seedbank_id);
        ip_port = boost::tuples::make_tuple(sb_record["ip_address"].String(), (uint16)sb_record["ip_port"].Int());
        _seedbank_map[seedbank_id] = ip_port;

#ifdef _DEBUG
        log_util::debug() << "torrentdb::_get_seedbank_for_id: found mongodb record for seedbank (" << sb_record["ip_address"].String() << ":" << (uint16)sb_record["ip_port"].Int() << ")" << endl;
#endif
    } else {
        ip_port = iter->second;
    }
    return ip_port;
}

BSONObj _look_up_seedbank(scoped_ptr<ScopedDbConnection> const& scoped_conn, int seedbank_id)
{
    BSONObj sb_record;
    DBClientBase* conn = scoped_conn->get();
    if (conn->isFailed()) {
        log_util::error() << "torrentdb::_look_up_seedbank: mongodb connection failed" << endl;
    } else {
#ifdef _DEBUG
        log_util::debug() << "torrentdb::_look_up_seedbank: running mongodb query (" << seedbank_id << ")" << endl;
#endif
        std::auto_ptr<DBClientCursor> cursor = conn->query(_param_map["seedbankdb_ns"], QUERY("seedbank_id" << seedbank_id));
        bool found_results = false;

        //if (conn->getLastError().empty()) {
            while (cursor->more()) {
                // TODO: verify no more than one record returned?
                sb_record = cursor->next();
                found_results = true;
            }
        //}
        if (!found_results) {
            log_util::error() << "torrentdb::_look_up_seedbank: mongodb result not found" << endl;
        }
    }

    return sb_record;
}

} // namespace torrentdb
} // namespace terasaur
