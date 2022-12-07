#!/bin/bash

while true; 
do 
    CHARACTERS=$1
    DIGITS=$2
    RANDOM_STRING=`cat /dev/urandom | tr -cd $CHARACTERS | head -c $DIGITS`
    echo "$RANDOM_STRING"
done