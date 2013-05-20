#!/usr/bin/env python2.6
#
# Copyright 2012 ibiblio
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import sys
import os
import traceback
from optparse import OptionParser, OptionGroup

# Path adjustment for non-dev deployments
local_libdir = os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + '/../lib')
if os.path.exists(local_libdir):
    sys.path.insert(0, local_libdir)

import tstracker.config.config_defaults as config_defaults
import terasaur.config.config_helper as config_helper
from tstracker.tstrackermq_cli import TerasaurTrackerMQCli
import terasaur.log.log_helper as log_helper

_LOGGER_NAME = 'tstrackermq'

def _get_option_parser():
    usage = """%prog [options] <command> <files>

Available commands:
    add             Add a torrent or add a seed bank to a torrent
    remove          Remove a torrent or remove a seed bank from a torrent4
    list            List torrents or list seed banks for a given torrent
    stats           Stats functions: push stats to terasaur, capture data"""

    #update          Update information for a torrent

    parser = OptionParser(usage=usage,
                          version='%prog 1.0',
                          description='Terasaur tracker MQ CLI')
    parser.add_option("-v", "--verbose", action="store_true", dest="verbose",
                      default=False, help="Enable verbose output")
    parser.add_option("--debug", action="store_true", dest="debug",
                      default=False, help="Enable debug output and stacktraces")
    parser.add_option('--infohash', dest='info_hash', type='string', metavar="SHA1", help='Specify info hash', default=None)
    parser.add_option('--seedbank', dest='seedbank', type='string', metavar="IP_PORT", help='Seed bank ip address and port (addr:port)', default=None)
    parser.add_option("--usemq", action="store_true", dest="usemq",
                      default=False, help="Use the MQ instead of direct-to-mongodb")

    stats_group = OptionGroup(parser, 'Stats options', '')
    stats_group.add_option("--full", action="store_true", dest="stats_full", default=False, help="Send updates for all torrents")
    stats_group.add_option("--incremental", action="store_true", dest="stats_incremental", default=False, help="Send updates for torrents changed since the last run")
    stats_group.add_option("--capture", action="store_true", dest="stats_capture", default=False, help="Take a stats snapshot")
    stats_group.add_option("--init", action="store_true", dest="stats_init", default=False, help="Initialize stats database")
    parser.add_option_group(stats_group)

    #update_group = OptionGroup(parser, 'Update options', '')
    #update_group.add_option('--published', dest='published', type='string', metavar="DATE", help='Published datetime (yyyy-mm-ddThh:mm:ss.mmmZ)', default=None)
    #parser.add_option_group(update_group)

    return parser

def _parse_args():
    parser = _get_option_parser()
    (options, args) = parser.parse_args()

    if len(args) == 0:
        parser.print_usage()
        options = None
        args = None

    return (parser, options, args)

def main():
    (parser, options, args) = _parse_args()
    if options is None:
        return

    # nasty hack
    # TODO: refactor log_helper and log_init modules
    log_helper._LOG_NAME = _LOGGER_NAME

    try:
        config_defaults.init()
        ch = config_helper.ConfigHelper()
        config = ch.get_config()
        cli = TerasaurTrackerMQCli(config)
        cli.execute(options, args)
    except Exception, e:
        print 'ERROR: ' + str(e)
        if options.debug is True:
            traceback.print_exc()

if __name__ == '__main__':
    main()
