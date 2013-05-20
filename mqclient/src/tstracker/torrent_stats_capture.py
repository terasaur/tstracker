#
# Copyright 2013 ibiblio
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

from tstracker.torrent import Torrent
import tstracker.stats_db as stats_db
import terasaur.db.mongodb_db as mongodb_db
from terasaur.mixin.timestamp import TimestampMixin

# TODO: remove post-testing
from datetime import datetime
import pytz

class DataPoint(object):
    """
    Stats data point.  Common format for minute, hour, and day data.
    """
    def __init__(self, **kwargs):
        """
        Constructor takes either:
         - torrent & now: Torrent object and datetime object
         - row: a stats dict object, normally from a mongodb query
        """
        if kwargs.has_key('torrent'):
            t = kwargs.get('torrent')
            self.ih = t.info_hash
            self.t = kwargs.get('now')
            self.p = t.peers
            self.s = t.seeds
            self.c = t.completed
        elif kwargs.has_key('row'):
            row = kwargs.get('row')
            if row.has_key('ih'):
                self.ih = row['ih']
            else:
                self.ih = row['_id'] # aggregate query results
            if row.has_key('t'):
                self.t = row['t']
            else:
                self.t = None
            self.p = row['p']
            self.s = row['s']
            self.c = row['c']

    def __str__(self):
        return self.ih + ' (' + str(self.t) + '): ' + str(self.p) + ', ' + str(self.s) + ', ' + str(self.c)

    def save(self, db, coll):
        data = {
            'ih': self.ih,
            't': self.t,
            'p': self.p,
            's': self.s,
            'c': self.c
            }
        db[coll].insert(data, w=1)

class TorrentStatsCapture(TimestampMixin):
    """
    stats_data collection in mongodb:
     - ih (info_hash): string
     - t (time): datetime
     - p (long): peers
     - s (long): seeds
     - c (long): completed

    series details
     - minute: save 8 hours of data, 480 data points
     - hour: save 1 week of data, 168 data points
     - day: one data point per day, no limit (yet)
    """
    def __init__(self, **kwargs):
        self._config = kwargs.get('config', None)
        self._options = kwargs.get('options', None) # OptionsParser options
        self._verbose = self._options.verbose if self._options else False
        self._now = self._get_now()

        # TODO: remove after testing
        #self._now = datetime(2013, 03, 14, 0, 0, 34, tzinfo=pytz.utc)

        if self._verbose:
            print 'Current datetime: ' + str(self._now)

    def run(self):
        self._init_conn()

        # always capture minute stats; assume we run every minute.
        self._capture_minute(self._now)

        # check for roll up hourly stats
        if self._should_capture_hour(self._now):
            self._capture_hour(self._now)

        # check for roll up daily stats
        if self._should_capture_day(self._now):
            self._capture_day(self._now)

        self._close_conn()

    def _init_conn(self):
        self._conn = stats_db.get_conn()
        self._db = self._conn[mongodb_db.DB_PARAMS['db_name']]

    def _close_conn(self):
        self._conn.end_request()

    def _capture_minute(self, now):
        last = stats_db.get_control_value('last_capture_minute')
        torrents = self._get_updated_torrents(last)

        if self._verbose:
            print 'Saving current counts to minute stats'
            print 'Last minute stats capture: ' + str(last)

        for torrent in torrents:
            # skip torrents with zero counts
            if torrent.peers == 0 and torrent.seeds == 0 and torrent.completed == 0:
                continue
            # save as minute data point
            self._save_data_point(stats_db.STATS_DATA_MINUTE, torrent, now)

        # delete rows that are more than 480 minutes old (8 hours)
        eight_hours_ago = self._in_the_past(now, hours=8)
        if self._verbose:
            print 'Clearing stats_minute past %s' % (str(eight_hours_ago))
        result = self._db[stats_db.STATS_DATA_MINUTE].remove({"t": {"$lt": eight_hours_ago}}, w=1)
        self._print_records_removed(result)

        stats_db.set_control_value('last_capture_minute', now)

    def _get_updated_torrents(self, last):
        """
        Get torrents that have changed since the last stats capture
        """
        torrents = Torrent.find(query={'updated': {"$gt": last}})
        return torrents

    def _should_capture_hour(self, dt):
        last = stats_db.get_control_value('last_capture_hour')
        an_hour_ago = self._in_the_past(dt, hours=1)
        return (last <= an_hour_ago and dt.minute == 0)

    def _capture_hour(self, now):
        """
        Capture hourly average for torrents that have changed in the last hour.
        """
        if self._verbose:
            print 'Saving hourly stats averages'

        # get average minute data for all torrents
        # limit to date > 1 hour ago
        an_hour_ago = self._in_the_past(now, hours=1)
        query = self._get_aggregate_query(an_hour_ago)
        rows = self._db[stats_db.STATS_DATA_MINUTE].aggregate(query)

        # loop by torrent, save averages to hourly data point
        for row in rows['result']:
            self._save_data_point_with_averages(row, now, 60, stats_db.STATS_DATA_HOUR)

        # delete rows that are more than 168 hours old (1 week)
        a_week_ago = self._in_the_past(now, days=7)
        if self._verbose:
            print 'Clearing stats_hour past %s' % (str(a_week_ago))
        result = self._db[stats_db.STATS_DATA_HOUR].remove({"t": {"$lt": a_week_ago}})
        self._print_records_removed(result)

        # update last capture datetime
        stats_db.set_control_value('last_capture_hour', now)

    def _get_aggregate_query(self, timestamp):
        query = [
            { "$match": { "t": { "$gte": timestamp } } },
            { "$group":
                { "_id": "$ih",
                  "p": {"$sum": "$p"},
                  "s": {"$sum": "$s"},
                  "c": {"$avg": "$c"}
                }
            }
        ]
        return query

    def _should_capture_day(self, dt):
        last = stats_db.get_control_value('last_capture_day')
        a_day_ago = self._in_the_past(dt, days=1)
        return (last <= a_day_ago and dt.hour == 0)

    def _capture_day(self, now):
        """
        Capture daily average for torrents that have changed in the last day.  Note
        that daily stats do not expire.
        """
        if self._verbose:
            print 'Saving daily stats averages'

        # get oldest day's worth of hourly data points
        a_day_ago = self._in_the_past(now, days=1)
        query = self._get_aggregate_query(a_day_ago)
        rows = self._db[stats_db.STATS_DATA_HOUR].aggregate(query)

        # loop through per torrent, save day data point
        for row in rows['result']:
            self._save_data_point_with_averages(row, now, 24, stats_db.STATS_DATA_DAY)

        stats_db.set_control_value('last_capture_day', now)

    def _save_data_point(self, coll, torrent, now):
        p = DataPoint(torrent=torrent, now=now)

        if self._verbose:
            print 'Saving data point to %s: %s' % (coll, str(p))
        p.save(self._db, coll)

    def _save_data_point_with_averages(self, row, now, total, timeframe):
        """
        Calculate averages based on the given total number of data points
        for the sample timeframe, then save the data point.
        """
        p = DataPoint(row=row)
        p.t = now
        p.p = int(round(p.p/total))
        p.s = int(round(p.s/total))
        p.c = long(round(p.c))
        if self._verbose:
            print 'Saving data point to %s: %s' % (timeframe, str(p))
        p.save(self._db, timeframe)

    def _print_records_removed(self, result):
        if self._verbose:
            if result is None:
                count = 0
            else:
                count = result['n']
            print '%i records removed' % (count)
