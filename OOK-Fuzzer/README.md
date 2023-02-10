Hackrf_ook Fuzzer:
Will generate random bit string based on the length you want it will brake after exhausting all combinations.
This script assumes you have hackrf and hackrf_ook installed system wide in your PATH=
How to use:
./hackrf_ook_2_sig.sh FREQ REPEAT DELAY LENGTH
./hackrf_ook_2_sig.sh 315055000 6 0.00900 26

hackrf_ook_fuzzer_Fixed-RND-FERQ:
Same as script for Hackrf_ook Fuzzer however will shift through a set of predefined frequencies after each transmission.
How to use:
./hackrf_ook_fuzzer_Fixed-RND-FERQ.sh REPEAT DELAY LENGTH
./hackrf_ook_fuzzer_Fixed-RND-FERQ.sh 5 0.00900 25
