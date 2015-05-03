#!/bin/bash

NUM_PLAYERS=80
START_MONEY=40
NUM_FILTER_ROUNDS=30

set -u

mkdir -p good

while [ 1 ] ; do
	for i in $(seq 1 $NUM_FILTER_ROUNDS) ; do
		best=$(./texas --money $START_MONEY --random 0 --ann $NUM_PLAYERS --save ai_*.net | tail -n20  | grep -E "^Saved file ai_.*\.net." | grep -o -E "ai_.*\.net")
		for b in $best; do
			cp $b good
		done
	done
	num_prev=$(ls ai_*.net | wc -l)
	rm -f ai_*.net
	mv good/*.net .
	num=$(ls ai_*.net | wc -l)
	echo Filtered in $num/$num_prev brains.
	if [ $num -eq 0 ]; then
		exit 1
	fi
done
