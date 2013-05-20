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

from terasaur.db import torrent_db
from terasaur.mixin.timestamp import TimestampMixin
from terasaur.torrent.util import is_valid_info_hash

class TorrentException(Exception): pass

class Torrent(TimestampMixin):
    __slots__ = ('_id', 'info_hash', 'created', 'updated', 'published', 'peers', 'seeds', 'completed', 'seedbanks')

    """
    pymongo.ObjectId _id (mongodb oid)
    string info_hash (unique)
    datetime created
    datetime updated
    datetime published
    int peers
    int seeds
    int completed
    list seedbanks -- list of seedbank id integers
    """

    def __init__(self):
        self._id = None
        self.info_hash = None
        self.created = None
        self.updated = None
        self.published = None
        self.peers = 0
        self.seeds = 0
        self.completed = 0
        self.seedbanks = None

    @staticmethod
    def find(**kwargs):
        """
        Find single:
            - Query by info hash and return Torrent object
        Find multiple:
            - Return list of Torrent objects
        """
        info_hash = kwargs.get('info_hash', None)
        if info_hash is not None:
            return Torrent._find_single(info_hash)
        else:
            return Torrent._find_multiple(kwargs.get('query', None))

    @staticmethod
    def _find_single(info_hash, torrent_root=None):
        Torrent._validate_info_hash(info_hash)
        data = torrent_db.get(info_hash)
        return Torrent._data_to_torrent(data)

    @staticmethod
    def _find_multiple(query=None):
        data_list = torrent_db.find(query)
        return Torrent._gen_find_results(data_list)

    @staticmethod
    def _gen_find_results(data_list):
        torrent_list = []
        for data in data_list:
            t = Torrent._data_to_torrent(data)
            torrent_list.append(t)
        return torrent_list

    @staticmethod
    def _data_to_torrent(data):
        if data:
            t = Torrent()
            t._id = data['_id']
            t.info_hash = data['info_hash']
            t.created = data['created']
            t.updated = data['updated']
            t.published = data['published']
            t.peers = data['peers']
            t.seeds = data['seeds']
            t.completed = data['completed']
            t.seedbanks = data['seedbanks']
        else:
            t = None
        return t

    def save(self, override_updated=False):
        self.validate()
        save_dict = self._get_save_dict(override_updated)
        torrent_db.save(save_dict)
        if not self.created:
            self.created = save_dict['created']
        self.updated = save_dict['updated']

    def validate(self):
        self._validate_info_hash(self.info_hash)

    @staticmethod
    def _validate_info_hash(info_hash):
        if not info_hash:
            raise TorrentException('Missing info hash')

        if not is_valid_info_hash(info_hash):
            raise TorrentException('Invalid info hash')

    def _get_save_dict(self, override_updated=False):
        created_date = self.created if self.created else self._get_now()
        if not override_updated or self.updated is None:
            self.updated = self._get_now()

        published = self.published if self.published else self._get_now()
        save_dict = {
            'info_hash': self.info_hash,
            'created': created_date,
            'updated': self.updated,
            'published': published,
            'peers': long(self.peers),
            'seeds': long(self.seeds),
            'completed': long(self.completed)
            }
        if self._id:
            save_dict['_id'] = self._id
        else:
            # don't save seedbank list here.  use add/remove seedbank methods.
            save_dict['seedbanks'] = self.seedbanks

        return save_dict

    def delete(self):
        self._validate_info_hash(self.info_hash)
        torrent_db.delete(self.info_hash)

    def __str__(self):
        return '%s: Peers: %s, Seeds: %s, Completed: %s, Updated: %s' % (self.info_hash, self.peers, self.seeds, self.completed, self.updated)

    def add_seedbank(self, seedbank, override_update=False):
        """
        Add seedbank to torrent and save changes.  Does not raise an exception
        if the torrent doesn't have the given seedbank.
        """
        if not seedbank:
            raise TorrentException('Cannot add null seedbank to torrent')

        if self._has_seedbank(seedbank):
            raise TorrentException('Torrent already has seedbank (' + str(seedbank) + ')')

        if self.seedbanks is None:
            self.seedbanks = []

        if not seedbank.id in self.seedbanks:
            self.seedbanks.append(seedbank.id)

        self._save_seedbanks(override_update)

    def remove_seedbank(self, seedbank, override_updated=False):
        """
        Remove seedbank from torrent and save changes.  Does not raise an exception
        if the torrent doesn't have the given seedbank.
        """
        if not seedbank:
            raise TorrentException('Cannot remove null seedbank from torrent')

        if seedbank.id in self.seedbanks:
            self.seedbanks.remove(seedbank.id)
        self._save_seedbanks(override_updated)

    def _has_seedbank(self, seedbank):
        if self.seedbanks is None:
            return False
        if seedbank.id in self.seedbanks:
            return True
        else:
            return False

    def _save_seedbanks(self, override_updated=False):
        query = { "_id": self._id }
        data = { "$set": { 'seedbanks': self.seedbanks } }
        if not override_updated or self.updated is None:
            self.updated = self._get_now()
            data["$set"]['updated'] = self.updated
        torrent_db.update(query, data)

class TorrentManager(object):
    @staticmethod
    def add(info_hash):
        t = Torrent.find(info_hash=info_hash)
        if t:
            raise TorrentException('Torrent already exists in tracker (' + info_hash + ')')
        t = Torrent()
        t.info_hash = info_hash
        t.save()

    @staticmethod
    def remove(info_hash):
        t = Torrent.find(info_hash=info_hash)
        if not t:
            raise TorrentException('Could not find torrent in tracker (' + info_hash + ')')
        t.delete()

