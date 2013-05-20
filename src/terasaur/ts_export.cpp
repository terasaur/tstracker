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

extern "C" {
#include "trackerlogic.h" // for ot_hash, ot_peerlist
#include <libowfat/ip6.h> // for scan_ip6
#include <libowfat/scan.h> // for scan_ushort
#include <libowfat/socket.h> /* for uint16 */
}

#include <limits> /* for numeric_limits */
#include "terasaur/peer_util.h"
#include <arpa/inet.h> /* for htons */
#include "terasaur/torrentdb.hpp"
#include <boost/shared_ptr.hpp>
#include "terasaur/converter.hpp"
#include "libtorrent/peer_id.hpp" // for sha1_hash
#include "terasaur/log_util.hpp"
#include "terasaur/torrent_acl.hpp"

using libtorrent::sha1_hash;
using std::endl;
using namespace terasaur;

#define UINT32_MAX std::numeric_limits<uint32_t>::max()

extern "C" int ts_torrentdb_hashisvalid(ot_hash* hash) {
    sha1_hash info_hash;
    ot_hash_to_sha1_hash(hash, info_hash);

    int is_valid;
    if (torrent_acl::hashisvalid(info_hash)) {
        is_valid = 1;
    } else {
        is_valid = 0;
    }
    return is_valid;
}

static bool _tuple_to_ot_peer(const string_int_tuple& ip_port, ot_peer* peer) {
    bool success = false;
    memset(peer, 0, sizeof(ot_peer));
    ot_ip6 tmp_addr;
    memset(tmp_addr, 0, sizeof(ot_ip6));

    string ip_addr = boost::tuples::get<0>(ip_port);
    uint16 tmp_port = htons(boost::tuples::get<1>(ip_port));

    if (scan_ip6(ip_addr.c_str(), tmp_addr)) {
        OT_SETIP(peer, tmp_addr);
        OT_SETPORT(peer, &tmp_port);
        OT_PEERFLAG(peer) = 0;
        OT_PEERFLAG(peer) |= PEER_FLAG_SEEDING;
        //OT_PEERFLAG(peer) |= PEER_FLAG_COMPLETED;
        OT_PEERTIME(peer) = 0;
        success = true;
    } else {
        success = false;
    }
    return success;
}

extern "C" void ts_torrentdb_add_seedbanks(ot_hash* hash, ot_peerlist *peer_list) {
    sha1_hash info_hash;
    ot_hash_to_sha1_hash(hash, info_hash);

    int exactmatch;
    ot_peer *peer_dest;
    ot_peer tmp_peer;

    // Get seed bank list for given torrent
    std::vector<string_int_tuple> sb_list = torrentdb::get_seedbanks(info_hash);
    std::vector<string_int_tuple>::iterator iter;
    for (iter = sb_list.begin(); iter != sb_list.end(); ++iter) {

        // Convert ip address/port tuple to ot_peer
        if (_tuple_to_ot_peer(*iter, &tmp_peer)) {
#ifdef _DEBUG
            log_util::debug() << "ts_export::ts_torrentdb_add_seedbanks: adding seed bank to peer list (" << boost::tuples::get<0>(*iter) << ":" << boost::tuples::get<1>(*iter) << ")" << endl;
#endif
            // Add ot_peer to peer list for the torrent
            exactmatch = 0;
            peer_dest = vector_find_or_insert_peer(&(peer_list->peers), &tmp_peer, &exactmatch);

            /**
             * Oddly, the find_or_insert function doesn't insert.  It only finds and makes a
             * provisional hole to put the new object in.
             *
             * If exactmatch is 1, the seed bank is already in the peer list.
             * If exactmatch is 0, the find function didn't find a match.  peer_dest is a pointer
             * to the location in memory where we should copy the new peer data.  That's not
             * at all confusing.
             */
            if (exactmatch == 0) {
                if (peer_dest) {
                    memcpy(peer_dest, &tmp_peer, sizeof(ot_peer));
                    // A seed is counted as both seed and peer
                    ++peer_list->seed_count;
                    ++peer_list->peer_count;
                } else {
                    log_util::error() << "ts_export::ts_torrentdb_add_seedbanks: got null from vector_find_or_insert_peer" << endl;
                }
            }
#ifdef _DEBUG
            else {
                log_util::debug() << "ts_export::ts_torrentdb_add_seedbanks: seed bank already in the peer list" << endl;
            }
#endif
        }
    }
/* TODO: need to reenable this */
#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_torrentdb_add_seedbanks: peer list" << endl;
    print_peers_peerlist(peer_list);
#endif

}

extern "C" void ts_update_torrent_stats(ot_torrent const* torrent, int increment_completed) {
    torrentdb::stats_update_t params;
    ot_hash_to_sha1_hash((ot_hash*)torrent->hash, params.info_hash);

#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_update_torrent_stats: start (" << params.info_hash << ", " << increment_completed << ")" << endl;
#endif

    ot_peerlist *peer_list = torrent->peer_list;

    // Convert from int to uint, need to prevent negative counts
    // Note that params peers and seeds are initialized to 0
    if (peer_list->peer_count > 0 && peer_list->peer_count < UINT32_MAX) {
        // Seeds in opentracker are considered both peers and seeds, which means
        // they're double counted.  Fix that here.  Perform a sanity check first,
        // though.
        if (peer_list->peer_count < peer_list->seed_count) {
            log_util::error() << "ts_export::ts_update_torrent_stats: peers (" << peer_list->peer_count << ") less than seeds (" << peer_list->seed_count << ")" << endl;
        } else {
            params.peers = peer_list->peer_count - peer_list->seed_count;
        }
    }
    if (peer_list->seed_count > 0 && peer_list->seed_count < UINT32_MAX) {
        params.seeds = peer_list->seed_count;
    }

    if (increment_completed > 0) {
        params.increment_completed = true;
    } else {
        params.increment_completed = false;
    }

#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_update_torrent_stats: peers: " << params.peers << endl;
    log_util::debug() << "ts_export::ts_update_torrent_stats: seeds: " << params.seeds << endl;
    log_util::debug() << "ts_export::ts_update_torrent_stats: increment_completed: " << params.increment_completed << endl;
#endif

    torrentdb::update_stats(params);

#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_update_torrent_stats: after torrentdb::update_stats" << endl;
#endif

/* TODO: need to reenable this */
#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_update_torrent_stats: peer list" << endl;
    print_peers_peerlist(peer_list);
#endif

#ifdef _DEBUG
    log_util::debug() << "ts_export::ts_update_torrent_stats: returning" << endl;
#endif
}

extern "C" void ts_log_debug(const char* msg) {
    log_util::debug() << msg << std::endl;
}

extern "C" void ts_log_error(const char* msg) {
    log_util::error() << msg << std::endl;
}
