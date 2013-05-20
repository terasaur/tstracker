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

import bson
from tstracker.torrent import Torrent
from terasaur.messaging.rabbitmq_publisher import SelfManagingRabbitMQPublisher
import terasaur.config.config_helper as config_helper
import tstracker.stats_db as stats_db
from terasaur.mixin.timestamp import TimestampMixin

class TorrentStatsUpdater(TimestampMixin):
    """
    Send peer, seed, and completed counts to the terasaur web application
    """
    def __init__(self, **kwargs):
        self._config = kwargs.get('config', None)
        self._options = kwargs.get('options', None) # OptionsParser options
        self._verbose = self._options.verbose

    def run(self):
        self._publisher = self._create_publisher()
        if self._options.stats_init:
            if self._verbose:
                print 'Initializing stats database'
            stats_db.initialize()
        elif self._options.info_hash:
            if self._verbose:
                print 'Updating torrent stats for ' + self._options.info_hash
            self._update_single_torrent(self._options.info_hash)
        else:
            if self._verbose:
                print 'Updating stats for all torrents'
            self._update_all_torrents()
        self._publisher.stop()

    def _update_single_torrent(self, info_hash):
        t = Torrent.find(info_hash=info_hash)
        self._send_stats_update(t)

    def _update_all_torrents(self):
        if not self._options.stats_full and not self._options.stats_incremental:
            raise Exception('Must specify either full or incremental stats update')
        if self._options.stats_full is True:
            self._update_full()
        else:
            self._update_incremental()

    def _update_full(self):
        now = self._get_now()
        torrents = Torrent.find()
        self._update_send_torrent_list(torrents, now)
        # note: we have updated the incremental timestamp to prevent large repeat updates

    def _update_incremental(self):
        now = self._get_now()
        last_update = stats_db.get_control_value('last_incremental')
        query = {'updated': {"$gt": last_update}}
        torrents = Torrent.find(query=query)
        self._update_send_torrent_list(torrents, now)

    def _update_send_torrent_list(self, torrents, now):
        if self._verbose:
            print 'Scheduling updates for %i torrents' % (len(torrents))
        count = 0
        for t in torrents:
            self._send_stats_update(t)
            count += 1
        stats_db.set_control_value('last_incremental', now)
        if self._verbose:
            print 'Final update count is %i torrents' % (count)

    def _send_stats_update(self, t):
        if self._verbose:
            print 'Sending stats update for %s (peers: %i, seeds: %i, completed: %i)' % (t.info_hash, t.peers, t.seeds, t.completed)

        data = {
            'action': 'torrent_stats',
            'stats_action': 'update',
            'info_hash': t.info_hash,
            'peers': t.peers,
            'seeds': t.seeds,
            'completed': t.completed,
            # _format_date from TimestampMixin
            'last_updated': self._format_date(t.updated)
            }
        self._send_message(data)

    def _create_publisher(self):
        routing_key = self._config.get(config_helper.MQ_SECTION, 'terasaur_queue')
        mq = SelfManagingRabbitMQPublisher(config=self._config,
                                           routing_key=routing_key,
                                           verbose=self._verbose)
        return mq

    def _format_date(self, d):
        return d.strftime('%Y-%m-%dT%H:%M:%SZ')

    def _send_message(self, data):
        message = bson.BSON().encode(data)
        self._publisher.publish(message)
