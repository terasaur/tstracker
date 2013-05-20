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

from terasaur.log.log_init import LogInitMixin
from terasaur.config.config_helper import MQ_SECTION
from terasaur.db import mongodb_db, torrent_db
from tstracker import seedbank_db
from terasaur.messaging.rabbitmq_consumer import RabbitMQConsumer
from terasaur.messaging.rabbitmq_publisher import SelfManagingRabbitMQPublisher
from terasaur.messaging.rabbitmq_connector import CONTENT_TYPE_BINARY
from tstracker.messaging.server_control_handler import TrackerControlMessageHandler

class TerasaurTrackerMQException(Exception): pass

class TerasaurTrackerMQ(LogInitMixin):
    def __init__(self, **kwargs):
        self.__init_from_kwargs(**kwargs)
        self._tick_interval = 0.5
        self._log = None
        self._mq_in = None
        self._mq_out = None
        self._run = True

    def __init_from_kwargs(self, **kwargs):
        self._config_helper = kwargs.get('config_helper', None)
        self._verbose = bool(kwargs.get('verbose', False))
        self._debug = bool(kwargs.get('debug', False))
        self._forked = bool(kwargs.get('fork', False))

        if not self._config_helper:
            raise TerasaurTrackerMQException('Missing config helper')

    def start(self):
        config = self._get_config()
        LogInitMixin.__init__(self, config=config)
        self._init_tstracker_db(config)
        self._start_mq_out(config)
        self._start_mq_in(config)

        try:
            while self._run:
                time.sleep(self._tick_interval)
        except KeyboardInterrupt:
            pass

        self._stop_mq_connectors()
        self._log.info('Exiting...')

    def stop(self):
        self._run = False

    def _get_config(self):
        return self._config_helper.get_config()

    def _init_tstracker_db(self, config):
        mongodb_db.set_connection_params_from_config(config)
        torrent_db.initialize()
        seedbank_db.initialize()

    """
    Message queue functions
    """
    def _start_mq_in(self, config):
        """
        Initiate inbound consumer connection to rabbitmq
        """
        queue_name = config.get(MQ_SECTION, 'control_queue')
        handler = TrackerControlMessageHandler(server=self, verbose=self._verbose)
        self._mq_in = RabbitMQConsumer(config=config,
                                       handler=handler,
                                       queue_name=queue_name,
                                       verbose=self._verbose)
        self._mq_in.start()

    def _start_mq_out(self, config):
        """
        Initiate outbound publisher connection to rabbitmq
        """
        routing_key = config.get(MQ_SECTION, 'terasaur_queue')
        self._mq_out = SelfManagingRabbitMQPublisher(config=config,
                                                     routing_key=routing_key,
                                                     verbose=self._verbose,
                                                     content_type=CONTENT_TYPE_BINARY)

    def _stop_mq_connectors(self):
        """ Stop rabbitmq connections """
        self._stop_mq_in()
        self._stop_mq_out()

    def _stop_mq_in(self):
        self._mq_in.stop()
        self._mq_in.join()
        self._mq_in = None

    def _stop_mq_out(self):
        self._mq_out.stop()
        self._mq_out = None
