# Read user input
# Take input as a argument
if [ -z "$1" ]
  then
    echo "No argument supplied"
    exit 1
fi
len=$1

# Generate de Bruijn sequence
bruijnseq=""
for (( i=1; i<=$len; i++ )); do
    bruijnseq="$bruijnseq$(echo $bits | fold -w$i |sort -u|tr -d '\n')"
done

# Output the results as a binary bit string
# only display 1 or 0 no spaces
echo "${bruijnseq// /}"

# Loop to next random binary bits
while true; do
    # Convert hexadecimal to binary
    bits=$(echo $1 | xxd -p -c256 | tr -d '\n' | sed 's/../& /g')
    binary=""
    for i in $bits; do
        binary="${binary}$(echo "obase=2;ibase=16;$i" | bc)"
    done

    # Generate de Bruijn sequence
    bruijnseq=""
    for (( i=1; i<=$len; i++ )); do
        bruijnseq="$bruijnseq$(echo $binary | fold -w$i |sort -u|tr -d '\n')"
    done

    # Output the results as a binary bit string
    # only display 1 or 0 no spaces
    echo "${bruijnseq// /}"
done