#!/bin/bash
# size=192
# a=192
# for((i=0;i<=14;i++))
# do
#     ((a=a*2))
#     ./test $a
#     # echo $a
# done;
./test 131072 1
./test 131072 2
./test 131072 4
./test 131072 8
./test 131072 16
./test 131072 24




