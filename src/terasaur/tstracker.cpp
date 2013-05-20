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
#include <getopt.h>
#include "terasaur/daemonize.hpp"
#include "terasaur/config.hpp"
#include "terasaur/torrentdb.hpp"
#include "terasaur/log_util.hpp"
#include <boost/lexical_cast.hpp>
#include <iostream>

extern "C" {
#include <libowfat/io.h> // for int64, io_block
#include <libowfat/ip6.h> // for scan_ip6
#include "opentracker.h"
#include "ot_udp.h" // for udp_init
#include "ot_accesslist.h" // for accesslist_blessip
}

extern char * g_serverdir; // next 2 vars for drop privs in opentracker.c
extern char * g_serveruser;
extern char   *g_stats_path; // see ot_http.c
extern char *g_redirecturl; // see opentracker.c

using std::endl;
using namespace terasaur;

void _usage(void) {
    printf ("Usage: terasaur_tracker [options]\n\n");
    printf ("Options:\n");
    printf ("    -c\n");
    printf ("        Specify configuration file.\n");
    printf ("    -F\n");
    printf ("        Run in foreground (do not fork).\n");
    printf ("    -D\n");
    printf ("        Enable debug message output.\n");
    printf ("    -h\n");
    printf ("        Display this help and usage information.\n");
}

void _set_ot_config_options() {
    // For drop privs support
    config::set_ot_global(&g_serverdir, config::get_value("main.rootdir"));
    config::set_ot_global(&g_serveruser, config::get_value("main.user"));

    // stats
    config::set_ot_global(&g_stats_path, config::get_value("main.stats_url_path"));
    config::set_ot_global(&g_redirecturl, config::get_value("main.redirect_url"));
}

void _set_ot_stats_acl() {
    string addr = config::get_value("main.access_stats");
    ot_ip6 tmp_addr;
    memset( tmp_addr, 0, sizeof(ot_ip6) );
    string tmp;

    if (scan_ip6(addr.c_str(), tmp_addr)) {
        accesslist_blessip(tmp_addr, OT_PERMISSION_MAY_STAT);
    } else {
        log_util::error() << "Invalid stats access IP address in config file (" << addr << ")" << endl;
    }
}

bool _torrentdb_init() {
    std::stringstream ss;
    ss << config::get_value("torrent_db.db_host") << ":" << config::get_value("torrent_db.db_port");
    torrentdb::set_param("connection_string", ss.str());
    ss.str("");
    ss.clear();
    ss << config::get_value("torrent_db.db_name") << "." << config::get_value("torrent_db.torrent_collection_name");
    torrentdb::set_param("torrentdb_ns", ss.str());
    ss.str("");
    ss.clear();
    ss << config::get_value("torrent_db.db_name") << "." << config::get_value("torrent_db.seedbank_collection_name");
    torrentdb::set_param("seedbankdb_ns", ss.str());
    return torrentdb::init_conn();
}

bool _bind_socket(const string& addr, const string& port, const string& proto) {
    bool success = true;
    PROTO_FLAG flag;
    ot_ip6 tmp_addr;
    memset( tmp_addr, 0, sizeof(ot_ip6) );
    uint16_t tmp_port;
    int64_t bind_socket;

    if (proto == "tcp") {
        flag = FLAG_TCP;
    } else if (proto == "udp") {
        flag = FLAG_UDP;
    } else {
        log_util::error() << "Invalid bind socket protocol string (" << addr << ")" << endl;
        success = false;
    }

    if (success && !scan_ip6(addr.c_str(), tmp_addr)) {
        log_util::error() << "Invalid IP address in config file (" << addr << ")" << endl;
        success = false;
    }

    if (success) {
        try {
            tmp_port = boost::lexical_cast<uint16_t>(port);
        } catch (boost::bad_lexical_cast const&) {
            log_util::error() << "Invalid port in config file (" << port << ")" << endl;
            success = false;
        }
    }

    if (success) {
        log_util::debug() << "Binding to " << proto << " socket (" << addr << ":" << port << ")" << endl;
        bind_socket = ot_try_bind(tmp_addr, tmp_port, flag);
        //log_util::debug() << "bind_result: ]" << result << "[" << endl;
    }

    if ((flag == FLAG_UDP) && bind_socket) {
        // Initialize udp worker pool
        int64 udp_workers = boost::lexical_cast<int64>(config::get_value("main.udp_workers"));
        log_util::debug() << "Initializing UDP worker pool with " << udp_workers << " workers" << endl;
        io_block(bind_socket);
        udp_init(bind_socket, udp_workers);
    }

    return success;
}

/**
 * Core runtime main function.  Parse args, possibly fork, then call bnbtmain()
 */
int main(int argc, char *argv[])
{
    int scanon = 1;
    bool do_fork = true;
    bool debug = false;
    bool okay_to_run = true;
    string config_file;

    // TODO: implement verbose option
    // TODO: implement debug option

    while( scanon ) {
        switch( getopt(argc, argv, ":c:vhFD") ) {
            case -1 : scanon = 0; break;
            case 'c':
                config_file = string(optarg);
                break;
            case 'F':
                do_fork = false;
                break;
            case 'D':
                debug = true;
                break;
            case 'h':
            case '?':
            default:
                _usage();
                exit(0);
        }
    }

    if (config_file.empty()) {
        log_util::error() << "No configuration file given, exiting." << endl << endl;;
        _usage();
        exit(1);
    }

    config::parse_file(config_file);

    // Set config variables used by parts of opentracker
    _set_ot_config_options();

    // Setup for stats ACLs
    _set_ot_stats_acl();

    // MongoDB init
    if (!_torrentdb_init()) {
        log_util::error() << "MongoDB init failed.  Exiting." << endl;
        exit(1);
    }

    // Listen on TCP
    if (okay_to_run) {
        okay_to_run = _bind_socket(config::get_value("main.bind_tcp_address"),
                                   config::get_value("main.bind_tcp_port"),
                                   "tcp");
    }

    // Listen on UDP
    if (okay_to_run) {
        okay_to_run = _bind_socket(config::get_value("main.bind_udp_address"),
                                   config::get_value("main.bind_udp_port"),
                                   "udp");
    }

    if (!okay_to_run) {
        exit(1);
    }

    if (do_fork) {
        daemonize();
    }

    // handoff to opentracker code
    return opentracker_main();
}
