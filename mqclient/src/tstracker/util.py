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

from tstracker.seedbank import Seedbank

def print_torrent(torrent):
    if torrent.seedbanks:
        sb_list = []
        for sb_id in torrent.seedbanks:
            sb = Seedbank.get(sb_id)
            sb_list.append('%s:%s' % (sb.ip_address, sb.ip_port))
        sb_list_str = ', '.join(sb_list)
    else:
        sb_list_str = 'None'

    print '%s' % (torrent.info_hash)
    print '        Seeds: %s' % (torrent.seeds)
    print '        Peers: %s' % (torrent.peers)
    print '    Completed: %s' % (torrent.completed)
    print '    Seedbanks: %s' % (sb_list_str)
    print '    Published: %s' % (torrent.published)
    print '        Added: %s' % (torrent.created)
    print '      Updated: %s' % (torrent.updated)
