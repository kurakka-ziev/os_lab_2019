#!/bin/bash
touch numbers.txt
cp /dev/null numbers.txt
for((i = 1; i <= $1; i++))
do
 #echo $RANDOM >> numbers.txt
 echo $(od -A n -t d -N 1 < /dev/random) >> numbers.txt
done