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
#ifndef EVENT_HANDLER_HPP_INCLUDED
#define EVENT_HANDLER_HPP_INCLUDED

#include "trackerlogic.h" /* for PROTO_FLAG */
#include <libowfat/io.h> /* for int64 in ot_stats.h */
#include "ot_stats.h" /* for ot_status_event and events enum */

#ifdef __cplusplus
extern "C" {
#endif

void ts_handle_event(ot_status_event event, PROTO_FLAG proto, uintptr_t event_data);

#ifdef __cplusplus
} // extern

namespace terasaur {
class event_handler {
    void _handle_announce(PROTO_FLAG proto, uintptr_t event_data);
    void _handle_completed(PROTO_FLAG proto, uintptr_t event_data);
    void _handle_renew(PROTO_FLAG proto, uintptr_t event_data);
    void _print_event_type(ot_status_event event);

public:
    void handle(ot_status_event event, PROTO_FLAG proto, uintptr_t event_data);

}; // event_handler
} // namespace terasaur


#endif // __cplusplus





#endif
