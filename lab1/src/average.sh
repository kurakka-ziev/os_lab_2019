#!/bin/bash

# any args? > exit
if [ $# -eq 0 ]; then
  echo "No arguments provided"
  exit 1
fi

sum=0
count=0

for arg in "$@"; do
# count if arg is a number, otherwise exit
  if [[ $arg =~ ^-?[0-9]+\.?[0-9]*$ ]]; then
    ((sum += arg))
    ((count++))
  else
    echo "Error: Non-numeric argument '$arg'" >&2
    exit 1
  fi
done

average=$(echo "scale=2; $sum / $count" | bc -l)

echo "Count: $count"
echo "Average: $average"