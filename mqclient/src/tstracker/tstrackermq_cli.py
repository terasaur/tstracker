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

import time
import bson
import sys

import terasaur.log.log_helper as log_helper
from terasaur.mixin.timestamp import TimestampMixin
from terasaur.torrent.util import is_valid_info_hash
import terasaur.config.config_helper as config_helper
from terasaur.messaging.rabbitmq_publisher import SelfManagingRabbitMQPublisher
from tstracker.torrent import Torrent, TorrentManager
from tstracker.seedbank import SeedbankManager
from tstracker.util import print_torrent
from tstracker.torrent_stats_updater import TorrentStatsUpdater
from tstracker.torrent_stats_capture import TorrentStatsCapture

class TrackerUpdater(TimestampMixin):
    def __init__(self, config, options):
        self._config = config
        self._options = options

    def _assert_info_hash(self, options):
        if not options.info_hash or not is_valid_info_hash(options.info_hash):
            raise Exception('Missing or invalid info hash')

class TrackerMQUpdater(TrackerUpdater):
    def add(self):
        self._send_message('add_torrent')

    def remove(self):
        self._send_message('remove_torrent')

    def list(self):
        print 'Cannot list torrents via the MQ'

    def stats(self):
        if self._options.stats_capture:
            tsu = TorrentStatsCapture(config=self._config, options=self._options)
        else:
            tsu = TorrentStatsUpdater(config=self._config, options=self._options)
        tsu.run()

    def _send_message(self, action):
        self._assert_info_hash(self._options)
        data = {'action':action, 'info_hash':self._options.info_hash}
        publisher = self._create_publisher(self._options)
        message = bson.BSON().encode(data)
        publisher.publish(message)
        publisher.stop()

    def _create_publisher(self, options):
        routing_key = self._config.get(config_helper.MQ_SECTION, 'control_queue')
        mq = SelfManagingRabbitMQPublisher(config=self._config,
                                           routing_key=routing_key,
                                           verbose=options.verbose)
        return mq

class TrackerDbUpdater(TrackerUpdater):
    def add(self):
        self._assert_info_hash(self._options)
        if self._options.seedbank:
            SeedbankManager.add(self._options.info_hash, self._options.seedbank)
        else:
            TorrentManager.add(self._options.info_hash)

    def remove(self):
        self._assert_info_hash(self._options)
        if self._options.seedbank:
            SeedbankManager.remove(self._options.info_hash, self._options.seedbank)
        else:
            TorrentManager.remove(self._options.info_hash)

    def list(self):
        if self._options.info_hash:
            self._list_single(self._options.info_hash)
        else:
            self._list_multiple()

    def stats(self):
        print 'Stats operations always use the MQ'

    def _list_single(self, info_hash):
        t = Torrent.find(info_hash=self._options.info_hash)
        if not t:
            print 'Torrent not found'
        else:
            print_torrent(t)

    def _list_multiple(self):
        t_list = Torrent.find()
        if not t_list or len(t_list) == 0:
            print 'No torrents found'
        else:
            for torrent in t_list:
                print torrent

    def _find(self, info_hash):
        data = torrent_db.get(info_hash)
        return data

class TerasaurTrackerMQCli(object):
    def __init__(self, config):
        if not config:
            raise Exception('Missing ConfigParser')
        self._config = config
        self._init_log()

    def _init_log(self):
        log_type = log_helper.LOG_TYPE_STREAM
        log_level = 'info'
        log_target = sys.stdout
        log_helper.initialize(log_type, log_level, log_target)

    def execute(self, options, args):
        command_str = args.pop(0)
        self._assert_valid_command(command_str)

        # stats always uses the MQ
        if command_str == 'stats':
            options.usemq = True

        if options.usemq:
            updater = TrackerMQUpdater(self._config, options)
        else:
            updater = TrackerDbUpdater(self._config, options)

        if command_str == 'add':
            updater.add()
        elif command_str == 'remove':
            updater.remove()
        elif command_str == 'list':
            updater.list()
        elif command_str == 'stats':
            updater.stats()

    def _assert_valid_command(self, command):
        if command not in ['add', 'remove', 'list', 'stats']:
            raise Exception('Invalid command: ' + command)
