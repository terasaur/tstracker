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
from datetime import datetime
import pytz

"""
Functions for storing and retrieving torrent statistics data.  See
torrent_stats module for details about data model.
"""

STATS_CONTROL_COLLECTION = 'stats_control'
STATS_DATA_MINUTE = 'stats_minute'
STATS_DATA_HOUR = 'stats_hour'
STATS_DATA_DAY = 'stats_day'

def get_control_value(key):
    result = mongodb_db.get(STATS_CONTROL_COLLECTION, {'_id': key})
    if result:
        return result['v']
    else:
        return None

def set_control_value(key, value):
    query = {'_id': key}
    data = {"$set": {'v': value}}
    mongodb_db.update(STATS_CONTROL_COLLECTION, query, data)

def get_conn():
    return mongodb_db.get_db_conn()

def get_minute_stats(torrent):
    return _get_stats(STATS_DATA_MINUTE, torrent)

def get_hour_stats(torrent):
    return _get_stats(STATS_DATA_HOUR, torrent)

def get_day_stats(torrent):
    return _get_stats(STATS_DATA_DAY, torrent)

def _get_stats(timeframe, torrent):
    conn = get_conn()
    db = conn[mongodb_db.DB_PARAMS['db_name']]
    res = db[timeframe].find({'ih':torrent.info_hash}).sort('ih')
    return res

def initialize():
    conn = mongodb_db.get_db_conn()
    db = conn[mongodb_db.DB_PARAMS['db_name']]
    # info hash index
    db[STATS_DATA_MINUTE].ensure_index('info_hash')
    db[STATS_DATA_HOUR].ensure_index('info_hash')
    db[STATS_DATA_DAY].ensure_index('info_hash')
    # control keys
    _initialize_date(db, 'last_incremental')
    _initialize_date(db, 'last_capture_minute')
    _initialize_date(db, 'last_capture_hour')
    _initialize_date(db, 'last_capture_day')
    conn.end_request()

def _initialize_date(db, key):
    value = get_control_value(key)
    if value is None:
        zero_date = datetime(1970, 1, 1, 0, 0, 0, 0, pytz.utc)
        data = {'_id': key, 'v': zero_date}
        db[STATS_CONTROL_COLLECTION].save(data)
