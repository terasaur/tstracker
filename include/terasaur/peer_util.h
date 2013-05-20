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
#ifndef PEER_UTIL_H_INCLUDED
#define PEER_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Opentracker */
#include "trackerlogic.h"
#include "ot_vector.h"

void print_peer(ot_peer *peer);
void print_peers_vector(ot_vector *vector);
void print_peers_peerlist(ot_peerlist *peer_list);

#ifdef __cplusplus
}
#endif

#endif
