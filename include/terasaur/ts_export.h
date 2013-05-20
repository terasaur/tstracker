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
#ifndef TSEXPORT_H_INCLUDED
#define TSEXPORT_H_INCLUDED

/**
 * Export c++ functions for use in the opentracker c code
 */

#include "trackerlogic.h" /* for ot_hash, ot_peerlist */

#ifdef __cplusplus
extern "C" {
#endif
int ts_torrentdb_hashisvalid(ot_hash* hash);
void ts_torrentdb_add_seedbanks(ot_hash* hash, ot_peerlist *peer_list);
void ts_update_torrent_stats(ot_torrent const* torrent, int increment_completed);
void ts_log_debug(const char* msg);
void ts_log_error(const char* msg);
#ifdef __cplusplus
}
#endif

#endif
