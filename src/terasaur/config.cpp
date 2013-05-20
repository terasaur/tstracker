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

#include "terasaur/config.hpp"
#include "terasaur/log_util.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using std::string;

namespace terasaur {
namespace config {

map<string, string> _config_options;

void parse_file(string config_file) {
#ifdef _DEBUG
    log_util::debug() << "Parsing config file: " << config_file << std::endl;
#endif
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(config_file, pt);

    _config_options["main.rootdir"] = pt.get<string>("main.rootdir", "");
    _config_options["main.user"] = pt.get<string>("main.user", "apache");
    _config_options["main.bind_tcp_address"] = pt.get<string>("main.bind_tcp_address", "0.0.0.0");
    _config_options["main.bind_tcp_port"] = pt.get<string>("main.bind_tcp_port", "6969");
    _config_options["main.bind_udp_address"] = pt.get<string>("main.bind_udp_address", "0.0.0.0");
    _config_options["main.bind_udp_port"] = pt.get<string>("main.bind_udp_port", "6969");
    _config_options["main.udp_workers"] = pt.get<string>("main.udp_workers", "4");
    _config_options["main.access_stats"] = pt.get<string>("main.access_stats", "127.0.0.1");
    _config_options["main.stats_url_path"] = pt.get<string>("main.stats_url_path", "stats");
    _config_options["main.redirect_url"] = pt.get<string>("main.redirect_url", "");

    // MongoDB params
    _config_options["torrent_db.db_host"] = pt.get<string>("torrent_db.db_host", "localhost");
    _config_options["torrent_db.db_port"] = pt.get<string>("torrent_db.db_port", "27017");
    _config_options["torrent_db.db_user"] = pt.get<string>("torrent_db.db_user", "");
    _config_options["torrent_db.db_pass"] = pt.get<string>("torrent_db.db_pass", "");
    _config_options["torrent_db.db_name"] = pt.get<string>("torrent_db.db_name", "tstracker");
    _config_options["torrent_db.torrent_collection_name"] = pt.get<string>("torrent_db.torrent_collection_name", "torrent");
    _config_options["torrent_db.seedbank_collection_name"] = pt.get<string>("torrent_db.seedbank_collection_name", "seedbank");
}

string get_value(string const& key) {
    return _config_options[key];
}

/**
 * Taken from opentracker.c and adapted
 */
void set_ot_global(char **option, string const& val_string) {
    const char *value = val_string.c_str();
    while (isspace(*value)) {
        ++value;
    }
    free(*option);
    *option = strdup(value);
}


} // namespace config
} // namespace terasaur
