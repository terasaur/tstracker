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

from terasaur.mixin.timestamp import TimestampMixin
from tstracker import seedbank_db
from tstracker.torrent import Torrent

class SeedbankException(Exception): pass

class Seedbank(TimestampMixin):
    __slots__ = ('_id', 'id', 'ip_address', 'ip_port', 'created', 'updated')

    """
    pymongo.ObjectId _id (mongodb oid)
    int id (unique)
    string ip_address
    int ip_port
    datetime created
    datetime updated
    """

    def __init__(self, ip=None, port=0):
        self._id = None
        self.id = None
        self.ip_address = ip
        self.ip_port = port
        self.created = None
        self.updated = None

    @staticmethod
    def get(seedbank_id):
        return Seedbank.find(id=seedbank_id)

    @staticmethod
    def find(**kwargs):
        """
        Find seed bank by id
        """
        id = kwargs.get('id', None)
        if id:
            data = seedbank_db.get(id)
            result = Seedbank._data_to_seedbank(data)
        else:
            data_list = seedbank_db.find(**kwargs)
            if data_list.count() == 1:
                result = Seedbank._data_to_seedbank(data_list[0])
            elif data_list.count() > 1:
                result = Seedbank._gen_find_results(data_list)
            else:
                result = None
        return result

    @staticmethod
    def _gen_find_results(data_list):
        sb_list = []
        for data in data_list:
            sb = Seedbank._data_to_seedbank(data)
            sb_list.append(sb)
        return sb_list

    @staticmethod
    def _data_to_seedbank(data):
        if data:
            sb = Seedbank()
            sb.id = data['seedbank_id']
            sb.ip_address = data['ip_address']
            sb.ip_port = data['ip_port']
            sb.created = data['created']
            sb.updated = data['updated']
        else:
            sb = None
        return sb

    def save(self):
        self.validate()
        save_dict = self._get_save_dict()
        oid = seedbank_db.save(save_dict)
        self.created = save_dict['created']
        self.updated = save_dict['updated']
        if not self.id:
            self.id = save_dict['seedbank_id']
            self._id = oid

    def validate(self):
        if not self.ip_address:
            raise SeedbankException('Missing ip address')
        if not int(self.ip_port) > 0:
            raise SeedbankException('Missing or invalid port number')

    @staticmethod
    def _validate_info_hash(info_hash):
        if not info_hash:
            raise SeedbankException('Missing info hash')

        if not is_valid_info_hash(info_hash):
            raise SeedbankException('Invalid info hash')

    def _get_save_dict(self):
        created_date = self.created if self.created else self._get_now()
        self.updated = self._get_now()
        save_dict = {
            'seedbank_id': self.id,
            'created': created_date,
            'updated': self.updated,
            'ip_address': self.ip_address,
            'ip_port': int(self.ip_port)
            }
        return save_dict

    def delete(self):
        self._validate_info_hash(self.info_hash)
        seedbank_db.delete(self.info_hash)

    def __str__(self):
        return '%s:%i' % (self.ip_address, self.ip_port)

class SeedbankManager(object):
    @staticmethod
    def add(info_hash, ip_port):
        sb = SeedbankManager._get_seedbank(ip_port)
        torrent = Torrent.find(info_hash=info_hash)
        if not torrent:
            raise SeedbankException('Could not find torrent for info hash: %s' % info_hash)
        torrent.add_seedbank(sb)

    @staticmethod
    def _get_seedbank(ip_port):
        """
        Find seed bank by ip address and port.  Add record if it doesn't already
        exist.
        """
        (ip, port) = ip_port.split(':')
        sb = Seedbank.find(ip=ip, port=port)
        if not sb:
            sb = Seedbank(ip, int(port))
            sb.save()
        return sb

    @staticmethod
    def remove(info_hash, ip_port):
        sb = SeedbankManager._get_seedbank(ip_port)
        torrent = Torrent.find(info_hash=info_hash)
        if not torrent:
            raise SeedbankException('Could not find torrent for info hash: %s' % info_hash)
        torrent.remove_seedbank(sb)
