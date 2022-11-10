#!/bin/sh
killall server
make ptdpl
./bin/server 8088 ./Web
