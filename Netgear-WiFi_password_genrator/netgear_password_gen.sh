#!/bin/bash

# This script will generate a combination of adjective + noun + digits 

# Get length of files
adj_length=$(wc -l < adj.txt)
noun_length=$(wc -l < noun.txt)

# Generating adjective + noun + digit combinations using a loop
while [ $adj_length -gt 0 ] && [ $noun_length -gt 0 ]; do
  adjective=$(shuf -n 1 /home/pi/Documents/adj.txt)
  noun=$(shuf -n 1 /home/pi/Documents/noun.txt)
  digit=$(shuf -i 0-999 -n 1-3)
  echo "$adjective$noun$digit"
  adj_length=$((adj_length - 1))
  noun_length=$((noun_length - 1))
done
