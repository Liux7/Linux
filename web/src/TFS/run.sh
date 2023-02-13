#!/bin/bash
for ((i=1;i<=100;i++))
do
    ./test $i >> res.txt
done