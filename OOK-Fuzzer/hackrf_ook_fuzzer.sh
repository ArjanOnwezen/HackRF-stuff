#!/bin/bash
FERQ=$1
REPEAT=$2
DELAY=$3
LEN=$4

# calculates the total number of combinations based on the length specified in the argument LEN. This is used to control the while loop, so that it runs the correct number of times.
NUM_COMBINATIONS=$((3**$LEN)) 
COUNTER=0

while [ $COUNTER -lt $NUM_COMBINATIONS ]
do
   CODE=""
   for ((i=0; i<$LEN; i++))
   do
      rand=$(( RANDOM % 2 ))
      CODE="$CODE$rand"
   done

# Add randomness $(shuf -i 1000-9600 -n 1) ie. hackrf_ook -b $(shuf -i 1000-9600 -n 1)   
# First burst:
#  hackrf_ook -r 1 -s 3528 -b 650 -0 492 -1 248 -m 100001 -f $FERQ -p 3528 -g -n
   sleep $DELAY
# Second Burst
   hackrf_ook -r $REPEAT -s 0 -p 3528 -b 650 -0 492 -1 248 -m $CODE -f $FERQ -g -n

   COUNTER=$((COUNTER+1))
   if [ $COUNTER -eq $NUM_COMBINATIONS ]
   then
   	break
   fi
done
