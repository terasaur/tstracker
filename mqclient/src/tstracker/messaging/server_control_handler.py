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

# TODO: rename file to control_message_handler.py

import traceback
import datetime
from terasaur.messaging.rabbitmq_message_handler import ControlMessageHandler
from tstracker.torrent import Torrent, TorrentManager
from tstracker.seedbank import SeedbankManager

class TrackerControlMessageHandler(ControlMessageHandler):
    def _handle_action(self, action, data):
        try:
            if action == 'add_torrent':
                self._handle_add_torrent(data)
            elif action == 'remove_torrent':
                self._handle_remove_torrent(data)
            elif action == 'add_seedbank':
                self._handle_add_seedbank(data)
            elif action == 'remove_seedbank':
                self._handle_remove_seedbank(data)
            elif action == 'update_torrent':
                self._handle_update_torrent(data)
            else:
                self._log.warning('Control message received without valid action (%s)' % action)
        except Exception, e:
            if self._verbose:
                traceback.print_exc()
            self._log.error(str(e))

    def _handle_add_torrent(self, data):
        info_hash = data['info_hash']
        if self._verbose:
            self._log.info('Received add_torrent control message (%s)' % info_hash)
        TorrentManager.add(info_hash)
        self._log.info('Added torrent to tracker (%s)' % info_hash)

    def _handle_remove_torrent(self, data):
        info_hash = data['info_hash']
        if self._verbose:
            self._log.info('Received remove_torrent control message (%s)' % info_hash)
        TorrentManager.remove(info_hash)
        self._log.info('Removed torrent from tracker (%s)' % info_hash)

    def _handle_add_seedbank(self, data):
        info_hash = data['info_hash']
        ip_port = data['ip_port']
        if self._verbose:
            self._log.info('Received add_seedbank control message (%s, %s)' % (info_hash, ip_port))
        SeedbankManager.add(info_hash, ip_port)
        self._log.info('Added seed bank (%s) to torrent (%s)' % (ip_port, info_hash))

    def _handle_remove_seedbank(self, data):
        info_hash = data['info_hash']
        ip_port = data['ip_port']
        if self._verbose:
            self._log.info('Received remove_seedbank control message (%s, %s)' % (info_hash, ip_port))
        SeedbankManager.remove(info_hash, ip_port)
        self._log.info('Removed seed bank (%s) from torrent (%s)' % (ip_port, info_hash))

    def _handle_update_torrent(self, data):
        info_hash = data['info_hash']
        if self._verbose:
            self._log.info('Received update_torrent control message (%s)' % info_hash)
            print data
        t = Torrent.find(info_hash=info_hash)
        if not t:
            raise Exception('Invalid info hash trying to update torrent (%s)' % (info_hash))

        if data.has_key('published'):
            t.published = datetime.datetime.strptime(date['published'], '%Y-%m-%dT%H:%M:%S.%fZ')
        t.save()
