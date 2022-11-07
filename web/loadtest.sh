#!/bin/sh
./bin/http_load  -parallel 5 -fetches 50 -seconds 20 ./Web/urls
