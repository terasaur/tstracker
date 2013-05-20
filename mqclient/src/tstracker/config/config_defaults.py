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

import os
import terasaur.config.config_helper as config_helper
import terasaur.db.mongodb_db as mongodb_db

"""
Default list when looking for a configuration file.  The first match
is read by the ConfigHelper.
"""
CONFIG_FILENAME = 'tstracker.conf'
CONFIG_SEARCH_DEFAULT = [
    CONFIG_FILENAME,
    'conf/' + CONFIG_FILENAME,
    os.path.dirname(__file__) + '/../../../../conf/' + CONFIG_FILENAME,
    '/usr/local/tstracker/conf/' + CONFIG_FILENAME,
    '/etc/tstracker/' + CONFIG_FILENAME
    ]

"""
Default configuration values
"""
DEFAULT_VALUES = {
    config_helper.MAIN_SECTION: {
        'error_log': '/var/log/tstracker/tstrackermq.error.log',
        'log_level': 'info'
        },
    config_helper.MONGODB_SECTION: {
        'db_host': 'localhost',
        'db_port': '27017',
        'db_name': 'tstracker',
        'db_user': '',
        'db_pass': ''
        },
    config_helper.MQ_SECTION: {
        'host': 'localhost',
        'port': '5672',
        'user': 'terasaur',
        'pass': 'terasaur',
        'vhost': '/terasaur',
        'exchange': 'terasaur',
        'control_queue': 'tracker.control',
        'terasaur_queue': 'terasaur.web'
        }
    }

def init():
    global DEFAULT_VALUES
    global CONFIG_SEARCH_DEFAULT
    config_helper.set_defaults(DEFAULT_VALUES, CONFIG_SEARCH_DEFAULT)
    mongodb_db.reset_config_values(DEFAULT_VALUES)
