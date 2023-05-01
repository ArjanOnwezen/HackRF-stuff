# hackrf_ook
Great Scott Gadgets HackRF OOK transmit poc.

#THIS CODE IS RE-UPLOADED DUE TO IT BEING TAKEN DOWN. ALL CREDITS GO TO THE ARTHUR.

This is a tool to make [HackRF One](https://greatscottgadgets.com/hackrf) emit an ASK-OOK signal. The primary test setup is to emulate a 27 Mhz Cardin S46 TX2 garage door remote control.
`hackrf_ook` computes samples and uses libhackrf to transmit the signal with default parameters or values specified on the command line.

Current available options are :

- `-f hertz` : transmit frequency (default -f 27195000)
- `-c hertz` : carrier frequency (default -f 27000)
- `-s µs` : preamble duration in microseconds (default -s 6156)
- `-b µs` : overall bit duration in microseconds (default -b 2104)
- `-0 µs` : width of gap for bit 0 in microseconds (default -0 1368)
- `-1 µs` : width of gap for bit 1 in microseconds (default -1 692)
- `-p µs` : trailing duration after message in microseconds (default -p 30548)
- `-m binary_message` : send this bits  (default -m 010100100101011011110011)
- `-r number` : repeat message 'number' time (default repeat until Ctrl+C).
- `-g` : send pulse first then gap for each bit (default to gap first, like in the drawing below)
- `-n` : bitwise NOT all bit
- `-d` : do nothing just print informations without transmitting with HackRF (no TX)
- `-h` : show this help

```
                                                 /  /
 preamble | bit | bit | bit | bit | bit |       /  / bit | bit |     pause
          |  0  |  1  |  2  |  3  |  4  |      /  /   22 |  23 |
          |     |     |     |     |     |     /  /       |     |
__________| ____|   __| ____|   __| ____|    /  /    ____|   __|
|         | |   |   | | |   |   | | |   |   /  /     |   |   | |
|         |_|   |___| |_|   |___| |_|   |  /  /     _|   |___| |_________________
          |     |     |     |     |     | /  /           |     |
          |     |     |     |     |     |/  /            |     |
                                        /  /


Bits configuration :

Without -g option, gap first :

|       |_____________|
|       |             |
|       |             |
|       |             |
|       |             |
|       |             |
|_______|             |
|       |             |
|       |             |
<- gap ->                   (-0 or -1 option)
<----- bit width ----->     (-b option)


with -g option, pulse first :

|_____________|       |
|             |       |
|             |       |
|             |       |
|             |       |
|             |       |
|             |_______|
|             |       |
|             |       |
              <- gap ->     (-0 or -1 option)
<----- bit width ----->     (-b option)
```

## Installation

```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make package
$ sudo dpkg -i hackrf_ook-0.0.1-Linux.deb
```


## Example Usage :

Sending `1110101010111010101010101` message with 1700 µs bit width, where 1 equal 416 µs high and 0 equal 1284 µs high, and with pulse bit first then gap, at 433.92 Mhz, repeat forever, 10 ms pause between each message. This works with my cheap remote controlled outlet (brand = Trefilaction, model = RSL366T) :

```
./hackrf_ook -s 0 -b 1700 -0 1284 -1 416 -p 10000 -m 1110101010111010101010101 -f 433920000 -g
Allocating 420000 I samples (0+(13600*25)+80000)
Allocating 420000 Q samples (0+(13600*25)+80000)
Precalculating samples...
--_--_--_-__--_-__--_-__--_-__--_--_--_-__--_-__--_-__--_-__--_-__--_-__--__________
25 bits to transmit at 433920000 Hz with a carrier frequency of 27000 Hz
Preamble:0(0us)   pause:80000(10000us)   bit:13600(1700us)   0:10272(1284us)   1:3328(416us)
Setting up the HackRF...
Transmitting, stop with Ctrl-C
^CCaught signal 2
Exiting...
Freeing I samples
Freeing Q samples
```

Sending `010100100101011011110011` at 27.195 Mhz with bit width of 2104 µs, 0 = 1368 µs, 1 = 692 µs, with a pause of 30.548 ms at the end of each message, repeat 10 time. Message starts with a 6156 µs preamble (high). This is for my Cardin S466 TX2 garage door remote control :

```
./hackrf_ook -s 6156 -b 2104 -0 1368 -1 692 -p 30548 -m 010100100101011011110011 -f 27195000 -r 10
Allocating 697600 I samples (49248+(16832*24)+244384)
Allocating 697600 Q samples (49248+(16832*24)+244384)
Precalculating samples...
----__-_--__-_--__-__-_--__-__-_--__-_--__-_--_--__-_--_--_--_--__-__-_--_--_________
24 bits to transmit at 27195000 Hz with a carrier frequency of 27000 Hz
Preamble:49248(6156us)   pause:244384(30548us)   bit:16832(2104us)   0:10944(1368us)   1:5536(692us)
Repeating the message 10 time
Setting up the HackRF...
Transmitting, stop with Ctrl-C ...or wait until TX is done
Exiting...
Freeing I samples
Freeing Q samples
```

Message bits, pulses duration and timing can be read with an RTL-SDR device and [rtl_433](https://github.com/merbanan/rtl_433) :

```
% ./rtl_433 -f 27195000 -A
[...]
Using device 0: Generic RTL2832U OEM
Detached kernel driver
Found Rafael Micro R820T tuner
Exact sample rate is: 250000.000414 Hz
[R82XX] PLL not locked!
Sample rate set to 250000.
Bit detection level set to 8000.
Tuner gain set to Auto.
Reading samples in async mode...
Tuned to 27195000 Hz.
[...]
Detected OOK package
------------------------------
protocol       = Cardin S466
message        = 10101101 10101001 00001100 

                 123456789
dipswitch      = +-+-+-+-o

                 -->ON
right button   = 2 o-- (this is left button or two buttons on same channel)
                 1 o--
Analyzing pulses...
Analyzing pulses...
Total count:  325,  width: 275669               (1102.7 ms)
Pulse width distribution:
 [ 0] count:   13,  width:  1537 [1534;1551]    (6148 us)
 [ 1] count:  143,  width:   185 [182;189]      ( 740 us)
 [ 2] count:  169,  width:   354 [350;359]      (1416 us)
Gap width distribution:
 [ 0] count:  143,  width:   341 [336;346]      (1364 us)
 [ 1] count:  169,  width:   172 [168;177]      ( 688 us)
 [ 2] count:   12,  width:  7616 [7613;7621]    (30464 us)
Pulse period distribution:
 [ 0] count:   13,  width:  1879 [1876;1890]    (7516 us)
 [ 1] count:  104,  width:   357 [355;360]      (1428 us)
 [ 2] count:   91,  width:   695 [693;698]      (2780 us)
 [ 3] count:  104,  width:   526 [524;528]      (2104 us)
 [ 4] count:   12,  width:  7971 [7968;7974]    (31884 us)
Level estimates [high, low]:  15781,   1730
Frequency offsets [F1, F2]:     927,      0     (+3.5 kHz, +0.0 kHz)
Guessing modulation: Pulse Width Modulation with startbit/delimiter
Attempting demodulation... short_limit: 269, long_limit: 945, reset_limit: 7622, demod_arg: 2
pulse_demod_pwm_ternary(): Analyzer Device 
bitbuffer:: Number of rows: 14 
[00] {0} : 
[01] {24} 52 56 f3 : 01010010 01010110 11110011 
[02] {24} 52 56 f3 : 01010010 01010110 11110011 
[03] {24} 52 56 f3 : 01010010 01010110 11110011 
[04] {24} 52 56 f3 : 01010010 01010110 11110011 
[05] {24} 52 56 f3 : 01010010 01010110 11110011 
[06] {24} 52 56 f3 : 01010010 01010110 11110011 
[07] {24} 52 56 f3 : 01010010 01010110 11110011 
[08] {24} 52 56 f3 : 01010010 01010110 11110011 
[09] {24} 52 56 f3 : 01010010 01010110 11110011 
[10] {24} 52 56 f3 : 01010010 01010110 11110011 
[11] {24} 52 56 f3 : 01010010 01010110 11110011 
[12] {24} 52 56 f3 : 01010010 01010110 11110011 
[13] {24} 52 56 f3 : 01010010 01010110 11110011 
```
