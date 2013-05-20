
#ifndef OPENTRACKER_H_INCLUDED
#define OPENTRACKER_H_INCLUDED

#include <libowfat/socket.h> /* for uint16 */
#include "trackerlogic.h" /* for ot_ip6 and PROTO_FLAG */

int opentracker_main();
int64_t ot_try_bind( ot_ip6 ip, uint16_t port, PROTO_FLAG proto );

#endif
