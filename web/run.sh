#!/bin/sh
killall server
make sm
./bin/server 8088 ./Web
