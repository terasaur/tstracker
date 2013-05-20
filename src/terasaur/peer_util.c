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

#include "trackerlogic.h"
#include "ot_vector.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> /* for htons */

void print_peer(ot_peer *peer) {
    uint16_t port = htons(*(uint16_t*)(peer->data+OT_IP_SIZE));
    printf("peer: %u.%u.%u.%u:%hu\n",
            (uint8_t)peer->data[0],
            (uint8_t)peer->data[1],
            (uint8_t)peer->data[2],
            (uint8_t)peer->data[3],
            port
            );
}

void print_peers_vector(ot_vector *vector) {
    ot_peer* tmp = (ot_peer*)vector->data;
    size_t i = 0;

    while (i < vector->size) {
        print_peer(tmp++);
        ++i;
    }
}

void print_peers_peerlist(ot_peerlist *peer_list) {
    printf("#---------- peer list begin ----------#\n");
    printf("has buckets: %i\n", OT_PEERLIST_HASBUCKETS(peer_list));
    printf("seed_count: %lu\n", peer_list->seed_count);
    printf("peer_count: %lu\n", peer_list->peer_count);
    printf("down_count: %lu\n", peer_list->down_count);

    unsigned int bucket, num_buckets = 1;
    ot_vector* bucket_list = &peer_list->peers;

    if (OT_PEERLIST_HASBUCKETS(peer_list)) {
        num_buckets = bucket_list->size;
        bucket_list = (ot_vector *)bucket_list->data;
    }

    for (bucket = 0; bucket < num_buckets; ++bucket) {
        if (num_buckets > 1) {
            printf("#---------- bucket %u\n", bucket);
        }
        print_peers_vector(&bucket_list[bucket]);
    }

    printf("#---------- peer list end   ----------#\n");
}
