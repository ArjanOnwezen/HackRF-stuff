#Code by Commander Crash of 29A-Society

#!/bin/bash

# Read user input
echo -n "Enter the length of bits: "
read len

# Generate de Bruijn sequence
bruijnseq=""
for (( i=1; i<=$len; i++ )); do
    bruijnseq="$bruijnseq$(echo $bits | fold -w$i |sort -u|tr -d '\n')"
done

echo "$bruijnseq"

# Loop to next random binary bits
while true; do
    bits=$(head -c 10 /dev/urandom | xxd -p)
    echo "$bits"

    # Generate de Bruijn sequence
    bruijnseq=""
    for (( i=1; i<=$len; i++ )); do
        bruijnseq="$bruijnseq$(echo $bits | fold -w$i |sort -u|tr -d '\n')"
    done

    echo "$bruijnseq"
done
