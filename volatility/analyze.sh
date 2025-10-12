#!/bin/bash

#VOL_CMD='python3 path/to/vol.py'
OUT_DIR='vol-analysis'

if [[ -z "$VOL_CMD" ]]; then
  echo "please set the VOL_CMD environment variable.";
  echo "for example: VOL_CMD='python3 path/to/vol.py'";
  exit 1;
fi;

if [[ $# -ne 2 ]]; then
  echo "usage: ./analyze.sh <volatility-plugins> <raw-dump>";
  exit 2;
fi;

echo "writing results to $OUT_DIR"
mkdir -p $OUT_DIR

while read line; do
  echo "starting $line ...";
  $VOL_CMD -f $2 $line &> $OUT_DIR/$line.log &
done <$1

wait
echo "all tasks completed"
