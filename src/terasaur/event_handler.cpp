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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "terasaur/event_handler.hpp"
#include "terasaur/torrentdb.hpp"
#include "terasaur/converter.hpp"

using std::cout;
using std::endl;

terasaur::event_handler* _event_handler = NULL;

namespace terasaur {

void event_handler::handle(ot_status_event event, PROTO_FLAG proto, uintptr_t event_data) {
#ifdef _DEBUG
    _print_event_type(event);
#endif

    // TODO: remove this
    _print_event_type(event);

    if (!event_data) {
        cout << "ERROR: missing event data in event_handler::handle" << endl;
        return;
    }

    if (event == EVENT_ANNOUNCE) {
        _handle_announce(proto, event_data);
    } else if (event == EVENT_COMPLETED) {
        _handle_completed(proto, event_data);
    } else if (event == EVENT_COMPLETED) {
        _handle_renew(proto, event_data);
    }
}

void event_handler::_handle_announce(PROTO_FLAG proto, uintptr_t event_data) {


}

void event_handler::_handle_completed(PROTO_FLAG proto, uintptr_t event_data) {
    ot_workstruct *ws = (ot_workstruct *)event_data;

    cout << "checking ws" << endl;
    if (!ws) {
        cout << "ERROR: got null workstruct from event_data" << endl;
        return;
    }

    cout << "checking ws->hash" << endl;
    if (!ws->hash) {
        cout << "ERROR: got null info hash from event_data" << endl;
        return;
    }

    sha1_hash info_hash;
    ot_hash* hash = ws->hash;
    ot_hash_to_sha1_hash(hash, info_hash);

    cout << "printing info_hash" << endl;
    cout << "info_hash: " << info_hash << endl;

}

void event_handler::_handle_renew(PROTO_FLAG proto, uintptr_t event_data) {


}

void event_handler::_print_event_type(ot_status_event event) {
    string event_str;
    if (event == EVENT_ACCEPT) {
        event_str = "EVENT_ACCEPT";
    } else if (event == EVENT_READ) {
        event_str = "EVENT_READ";
    } else if (event == EVENT_CONNECT) {
        event_str = "EVENT_CONNECT";
    } else if (event == EVENT_ANNOUNCE) {
        event_str = "EVENT_ANNOUNCE";
    } else if (event == EVENT_COMPLETED) {
        event_str = "EVENT_COMPLETED";
    } else if (event == EVENT_RENEW) {
        event_str = "EVENT_RENEW";
    } else if (event == EVENT_SYNC) {
        event_str = "EVENT_SYNC";
    } else if (event == EVENT_SCRAPE) {
        event_str = "EVENT_SCRAPE";
    } else if (event == EVENT_FULLSCRAPE_REQUEST) {
        event_str = "EVENT_FULLSCRAPE_REQUEST";
    } else if (event == EVENT_FULLSCRAPE_REQUEST_GZIP) {
        event_str = "EVENT_FULLSCRAPE_REQUEST_GZIP";
    } else if (event == EVENT_FULLSCRAPE) {
        event_str = "EVENT_FULLSCRAPE";
    } else if (event == EVENT_FAILED) {
        event_str = "EVENT_FAILED";
    } else if (event == EVENT_BUCKET_LOCKED) {
        event_str = "EVENT_BUCKET_LOCKED";
    } else if (event == EVENT_WOODPECKER) {
        event_str = "EVENT_WOODPECKER";
    } else if (event == EVENT_CONNID_MISSMATCH) {
        event_str = "EVENT_CONNID_MISSMATCH";
    } else {
        event_str = "UNKNOWN";
    }
    cout << event_str << endl;
}

} // namespace terasaur

/**
 * Expose event handler to opentracker
 */
extern "C" void ts_handle_event(ot_status_event event, PROTO_FLAG proto, uintptr_t event_data) {
    // TODO
    if (_event_handler == NULL) {
        _event_handler = new terasaur::event_handler();
    }
    _event_handler->handle(event, proto, event_data);
}
