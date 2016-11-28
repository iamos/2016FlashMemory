#!/bin/bash
for y in digicam_old digicam_new linux symbian
do
	for x in 1 10 100
	do
	# echo $y.trace $x
	make clean
	make
	./msbs $y.trace $x > $y.$x
	done
done

