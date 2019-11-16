#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

cd /opt/opalsrv/bin
ulimit -c unlimited
./opalsrv --ini-file /opt/opalsrv/etc/opalsrv.ini --console --daemon
