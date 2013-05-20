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
from optparse import OptionParser
import signal

# Path adjustment for non-dev deployments
local_libdir = os.path.realpath(os.path.dirname(os.path.realpath(__file__)) + '/../lib')
if os.path.exists(local_libdir):
    sys.path.insert(0, local_libdir)

import tstracker.config.config_defaults as config_defaults
import terasaur.config.config_helper as config_helper
import terasaur.log.log_helper as log_helper
from tstracker.server.tstrackermq import TerasaurTrackerMQ
from terasaur.server.daemonizer import daemonize

_LOGGER_NAME = 'tstrackermq'
_SIG_HANDLER_LOGGER_NAME = 'tstrackermq'

def _parse_args():
    parser = _get_option_parser()
    (options, args) = parser.parse_args()
    return (options, args)

def _get_option_parser():
    usage = """%prog [options]"""

    parser = OptionParser(usage=usage,
                          version='%prog 2.0',
                          description='ibiblio terasaur Seed Bank')
    parser.add_option('--config', dest='config_file', type='string', metavar="PATH", help='Specify configuration file', default=None)
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                      default=False, help='Enable verbose output')
    parser.add_option('-D', '--debug', action='store_true', dest='debug',
                      default=False, help='Enable debug logging')
    parser.add_option('--fork', action='store_true', dest='fork',
                      default=False, help='Fork process and run in background')
    return parser

def main():
    (options, args) = _parse_args()
    if options is None:
        return

    # nasty hack
    # TODO: refactor log_helper and log_init modules
    log_helper._LOG_NAME = _LOGGER_NAME

    if options.fork:
        daemonize()

    config_defaults.init()
    if options.config_file:
        file_list = [options.config_file]
    else:
        file_list = None
    ch = config_helper.ConfigHelper(file_list=file_list)
    if options.debug:
        options.verbose = True

    server = TerasaurTrackerMQ(config_helper=ch,
                               fork=options.fork,
                               verbose=options.verbose,
                               debug=options.debug)

    def sigtermHandler(signum, frame, server=server):
        log = log_helper.get_logger(_SIG_HANDLER_LOGGER_NAME)
        log.info('SIGTERM caught, exiting')
        server.stop()

    def sighupHandler(signum, frame, server=server):
        log = log_helper.get_logger(_SIG_HANDLER_LOGGER_NAME)
        log.info('SIGHUP caught, no-op')

    signal.signal(signal.SIGHUP, sighupHandler)
    signal.signal(signal.SIGTERM, sigtermHandler)

    server.start()

if __name__ == '__main__':
    main()
