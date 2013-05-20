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

#include "terasaur/torrent_acl.hpp"
#include "terasaur/date.hpp"
#include "terasaur/log_util.hpp"
#include "terasaur/torrentdb.hpp"
//#include <boost/thread/mutex.hpp>

using mongo::BSONObj;
using std::endl;

namespace terasaur {
namespace torrent_acl {

bool hashisvalid(sha1_hash const& info_hash) {
#ifdef _DEBUG
    log_util::debug() << "torrent_acl::hashisvalid (" << info_hash << ")" << endl;
#endif

    BSONObj torrent_record;
    bool is_valid = false;
    try {
#ifdef _DEBUG
        log_util::debug() << "torrent_acl::hashisvalid: getting torrent" << endl;
#endif
        boost::shared_ptr<terasaur::torrent> torrent = torrentdb::look_up_info_hash(info_hash);
#ifdef _DEBUG
        log_util::debug() << "torrent_acl::hashisvalid: checking info hash match" << endl;
#endif

        if (info_hash == torrent->info_hash) {
#ifdef _DEBUG
            log_util::debug() << "torrent_acl::hashisvalid: checking published date" << endl;
#endif
            is_valid = torrent->is_published();
#ifdef _DEBUG
            if (!is_valid) {
                log_util::debug() << "Access denied for torrent -- published date is in the future" << endl;
            }
#endif
        }
#ifdef _DEBUG
        else {
            log_util::debug() << "torrent_acl::hashisvalid: info_hash != torrent->info_hash" << endl;
        }
#endif

    } catch (mongo::DBException &e) {
        log_util::error() << "(hashisvalid): mongodb exception: " << e.what() << endl;
    } catch (std::exception& e) {
        log_util::error() << "(hashisvalid): mongodb query failed: " << e.what() << endl;
    }

    return is_valid;
}

} // namespace torrent_acl
} // namespace terasaur
