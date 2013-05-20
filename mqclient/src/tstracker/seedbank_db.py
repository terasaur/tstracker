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

import terasaur.db.mongodb_db as mongodb_db
import pymongo

"""
Customized interface for Seedbank CRUD functions
"""

SEEDBANK_COLLECTION = 'seedbank'

def get(id):
    item = mongodb_db.get(SEEDBANK_COLLECTION, {'seedbank_id': id})
    return item

def save(seedbank_data):
    if not seedbank_data['seedbank_id']:
        new_id = _get_next_seedbank_id()
        if not new_id:
            raise Exception('Invalid seedbank id')
        seedbank_data['seedbank_id'] = new_id
    oid = mongodb_db.save(SEEDBANK_COLLECTION, seedbank_data)
    return oid

def _get_next_seedbank_id():
    conn = mongodb_db.get_db_conn()
    db = conn[mongodb_db.DB_PARAMS['db_name']]
    result = db.counters.find_and_modify(query={'_id': 'seedbank_id'}, update={"$inc": {'c': 1}}, new=True)
    conn.end_request()
    new_id = int(result['c'])
    return new_id

def delete(info_hash):
    mongodb_db.delete(SEEDBANK_COLLECTION, {'seedbank_id': id})

def find(**kwargs):
    ip = kwargs.get('ip', None)
    port = kwargs.get('port', None)
    if ip and port:
        result = mongodb_db.find(SEEDBANK_COLLECTION, {'ip_address':ip, 'ip_port':int(port)})
        if result.count() > 1:
            raise Exception('Found multiple seedbank entries for %s:%s' % (ip, port))
    else:
        result = mongodb_db.find(SEEDBANK_COLLECTION, None)
    return result

def initialize():
    conn = mongodb_db.get_db_conn()
    db = conn[mongodb_db.DB_PARAMS['db_name']]
    # indexes
    db[SEEDBANK_COLLECTION].ensure_index('seedbank_id', unique=True)
    db[SEEDBANK_COLLECTION].ensure_index([('ip_address', pymongo.ASCENDING), ('ip_port', pymongo.ASCENDING)], unique=True)
    # verify seedbank id sequence
    result = db.counters.find_one({'_id':'seedbank_id'})
    if not result:
        db.counters.insert({'_id':'seedbank_id', 'c':0})
    conn.end_request()
