#! /bin/bash
for i in `seq 0 1 $(($1-1))`;
do
	./mpsi $1 $2 $3 $i $4 $5&

	# ./oringt1 $1 $2 $3 $i $4 $5&

	echo "Running $i..." &
done
