#!/usr/bin/bash
read
for t in {1..16}
do
    sum=0
    for n in {1..10}
    do
        ((sum+=$(./main_exe $t)))
    done
    sum=$((sum / 10))
    echo $sum
done

