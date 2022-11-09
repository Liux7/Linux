#!/bin/sh
killall server
make ptd
./bin/server 8088 ./Web
