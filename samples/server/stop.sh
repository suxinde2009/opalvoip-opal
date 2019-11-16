#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

cd /opt/opalsrv/bin
./opalsrv --kill
