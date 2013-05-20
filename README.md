tstracker
=========

terasaur BitTorrent tracker

## Summary ##

The terasaur tracker is a customized version of [opentracker](http://erdgeist.org/arts/software/opentracker/).  It's wrapped in c++ and uses a mongodb database to save torrent information and statistics.

Integration with the terasaur web application (tsweb) is handled by a set of python tools (the mqclient) that communicate via AMQP.  The mq client and tracker run as separate processes.

## Building ##

You need boost build (bjam) to compile the tstracker code.  Use the build.sh script 

The following are valid:

    ./scripts/build.sh

    ./scripts/build.sh clean

    ./scripts/build.sh debug

## Running ##

The tracker and mq integration tools have only been confirmed to run on Linux, specifically CentOS 6 with Boost 1.48.  See the tstracker and tstrackermq.py executables for command line options.  A single configuration file is used by both.  See the provided sample, conf/tstracker.conf-dist.

