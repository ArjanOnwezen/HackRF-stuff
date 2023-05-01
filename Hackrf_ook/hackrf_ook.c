/*
 * Copyright (c) 2016, Denis Bodor
 *
 * Simple and ugly try to send ASK-OOK with hackrf
 * Derived from W.B.Hill ( M1BKF) <bugs@wbh.org> code hackrf_beep.c
 * (https://github.com/rufty/hackrf_beep)
 *
 * Tested remotes :
 *
 * Generic Remote (cf rtl_433)
 * Bouton 2 chan 1 on :
 * ./hackrf_ook -s 0 -b 1700 -0 1284 -1 416 -p 10000 -m 1110101010111010101010101 -f 433920000 -g
 * Bouton 2 chan 1 off :
 * ./hackrf_ook -s 0 -b 1700 -0 1284 -1 416 -p 10000 -m 1110101010111010101010111 -f 433920000 -g
 * Tested with rtl_433 and real device
 *
 * Cardin S46 (S466) TX2 garage door remote :
 * ./hackrf_ook
 * or
 * ./hackrf_ook -s 6156 -b 2104 -0 1368 -1 692 -p 30548 -m 010100100101011011110011 -f 27195000
 * Tested with rtl_433 (not real garage door... yet)
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <glib.h>
#include <libhackrf/hackrf.h>

#define OOK_FREQ		27195000ULL	// transmit frequency
#define OOK_START		49248		// nbr samples for preamble
#define OOK_BIT			16832		// nbr samples for a bit
#define OOK_0			10944		// start position for a 0 (relative to OOK_BIT)
#define OOK_1			5536		// start position for a 1 (relative to OOK_BIT)
#define OOK_PAUSE		244384		// trailer
#define OOK_NBR_BITS	24			// nbr of bits in the message
#define OOK_MSG_SIZE	OOK_START+(OOK_BIT*OOK_NBR_BITS)+OOK_PAUSE
#define OOK_NEGATE		0
#define OOK_DEFAULT_MSG "010100100101011011110011"
#define OOK_CARRIER		27000		// carrier frequency
#define OOK_PG			0			// send pulse first then gap for each bit
#define OOK_DO			1			// transmit with HackRF
#define OOK_COUNTER		-1			// repeat message or not

// Transmit frequency
uint64_t freq = OOK_FREQ;
// Sample rate
const uint32_t samplerate = 8000000;
// Transmitter IF gain
const unsigned int gain = 47;

int8_t *txbufferI;
int8_t *txbufferQ;
int bufferOffset;

int ook_start = OOK_START;
int ook_bit = OOK_BIT;
int ook_0 = OOK_0;
int ook_1 = OOK_1;
int ook_pause = OOK_PAUSE;
int ook_nbr_bits = OOK_NBR_BITS;
int ook_msg_size = OOK_MSG_SIZE;
int ook_negate = OOK_NEGATE;
int ook_carrier = OOK_CARRIER;
int ook_pg = OOK_PG;
int ook_do = OOK_DO;
int ook_counter = OOK_COUNTER;

const double tau = 2.0 * M_PI;

// bits to send
char *bits;

static hackrf_device* device = NULL;

GMainLoop *loop;

// Giving data to the HackRF
int tx_callback(hackrf_transfer* transfer)
{
	// How much to do?
	size_t count = transfer->valid_length;
	
	int i = 0;
	while (i < count) {
		// Not really sure what i'm doing here...
		(transfer->buffer)[i++] = txbufferI[bufferOffset];  // I
		(transfer->buffer)[i++] = txbufferQ[bufferOffset];  // Q
		bufferOffset++;
		if (ook_counter >=0) {
			if (bufferOffset == ook_msg_size) {
				ook_counter--;
				if (ook_counter < 1) {
					g_main_loop_quit(loop);
				}
			}
		}
		bufferOffset %= ook_msg_size; // loop on the buffer
	}
	return 0 ;
}

// Deal with interruptions.
void sigint_callback_handler(int signum)
{
	fprintf(stderr, "Caught signal %d\n", signum);
	g_main_loop_quit(loop);
}

void printhelp(char *binname)
{
	printf("HackRF One ASK-OOK Emitter v0.0.1\n");
	printf("Copyright (c) 2016 - Denis Bodor\n\n");
	printf("Usage : %s [OPTIONS]\n", binname);
	printf(" -f hertz             transmit frequency (default -f %llu)\n", OOK_FREQ);
	printf(" -c hertz             carrier frequency (default -f %d)\n", OOK_CARRIER);
	printf(" -s us                preamble duration in microseconds (default -s %d)\n", OOK_START/8);
	printf(" -b us                overall bit duration in microseconds (default -b %d)\n", OOK_BIT/8);
	printf(" -0 us                width of gap for bit 0 in microseconds (default -0 %d)\n", OOK_0/8);
	printf(" -1 us                width of gap for bit 1 in microseconds (default -1 %d)\n", OOK_1/8);
	printf(" -p us                trailing duration after message in microseconds (default -p %d)\n", OOK_PAUSE/8);
	printf(" -m binary_message    send this bits  (default -m %s)\n", OOK_DEFAULT_MSG);
	printf(" -r number            repeat message 'number' time (default repeat until Ctrl+C)\n");
	printf(" -n                   bitwise NOT all bit\n");
	printf(" -g                   send pulse first then gap for each bit (default to gap first)\n");
	printf(" -d                   do nothing just print informations (no TX)\n");
	printf(" -h                   show this help\n");
}

int main (int argc, char** argv)
{
	int result;
	int retopt;
	int opt = 0;
	char *endptr;
	double carrierAngle;

	while ((retopt = getopt(argc, argv, "f:c:s:b:0:1:p:m:r:gndh")) != -1) {
		switch (retopt) {
			case 'f':
				freq = (uint64_t)strtoll(optarg, &endptr, 10);
				if (endptr == optarg || freq == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'c':
				ook_carrier = (int)strtol(optarg, &endptr, 10);
				if (endptr == optarg || ook_carrier == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'h':
				printhelp(argv[0]);
				return(EXIT_SUCCESS);
				opt++;
				break;
			case 's':
				ook_start = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_start < 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'b':
				ook_bit = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_bit == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case '0':
				ook_0 = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_0 == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case '1':
				ook_1 = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_1 == 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'p':
				ook_pause = (int)strtol(optarg, &endptr, 10) * 8;
				if (endptr == optarg || ook_pause < 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'm':
				ook_nbr_bits = strlen(optarg);
				bits = strdup(optarg);
				opt++;
				break;
			case 'r':
				ook_counter = (int)strtol(optarg, &endptr, 10);
				if (endptr == optarg || ook_counter <= 0) {
					printf("You must specify a valid number\n");
					return(EXIT_FAILURE);
				}
				opt++;
				break;
			case 'g':
				ook_pg = 1;
				opt++;
				break;
			case 'n':
				ook_negate = 1;
				opt++;
				break;
			case 'd':
				ook_do = 0;
				opt++;
				break;
			default:
				printhelp(argv[0]);
				return(EXIT_FAILURE);
		}
	}

	if (bits == NULL) 
		bits = strdup(OOK_DEFAULT_MSG);

	ook_msg_size = ook_start+(ook_bit*ook_nbr_bits)+ook_pause;
	printf("Allocating %d I samples (%d+(%d*%d)+%d)\n", ook_msg_size, ook_start, ook_bit, ook_nbr_bits, ook_pause);
	printf("Allocating %d Q samples (%d+(%d*%d)+%d)\n", ook_msg_size, ook_start, ook_bit, ook_nbr_bits, ook_pause);
	txbufferI = malloc(ook_msg_size*sizeof(int8_t));
	txbufferQ = malloc(ook_msg_size*sizeof(int8_t));
	if (txbufferI == NULL || txbufferQ == NULL) {
		printf("Error allocating memory!\n");
		return(EXIT_FAILURE);
	}
	memset(txbufferI, 0, ook_msg_size*sizeof(int8_t));
	memset(txbufferQ, 0, ook_msg_size*sizeof(int8_t));

	fprintf(stderr, "Precalculating samples...\n");

	/*
	 * Precalc waveforms
	 * We need to amplitude modulate a signal on top of the base frequency
	 * So we use sin() and cos() and a full wave to make samples
	 * A full wave is a complete angle rotation (2*Pi or Tau, in radian)
	 * This take samprate/carrier_freq samples...
	 * So when we pulse, we pulse carrier_freq modulate on top of base frequency...
	 */
	// preamble
	int c = 0;
	int s = 0;
	int pos;
	// How many samples to have a full wave of the carrier freq ?
	int full = samplerate/ook_carrier;
	if (s < ook_start)
		printf("----");
	while (s < ook_start) {
		carrierAngle = c * tau / full;
		txbufferI[s] = (int8_t)(127.0 * sin(carrierAngle));
		txbufferQ[s] = (int8_t)(127.0 * cos(carrierAngle));
		s++;
		c++;
		c %= full; // loop on the carrier full wave
	}

	// compute samples for each bit
	for (int i = 0; i < ook_nbr_bits; i++) {
		if (bits[i] == '0') {
			if (ook_negate > 0) {
				s = ook_start+(ook_bit*i)+ook_1;
				if (ook_pg) printf("--_"); else printf("_--");
			} else {
				s = ook_start+(ook_bit*i)+ook_0;
				if (ook_pg) printf("-__"); else printf("__-");
			}
			//s = ook_negate > 0 ? ook_start+(ook_bit*i)+ook_1 : ook_start+(ook_bit*i)+ook_0;
		} else {
			if (ook_negate > 0) {
				s = ook_start+(ook_bit*i)+ook_0;
				if (ook_pg) printf("-__"); else printf("__-");
			} else {
				s = ook_start+(ook_bit*i)+ook_1;
				if (ook_pg) printf("--_"); else printf("_--");
			}
			//s = ook_negate > 0 ? ook_start+(ook_bit*i)+ook_0 : ook_start+(ook_bit*i)+ook_1;
		}
		// fill samples
		if (ook_pg == 1) {
			// pulse first, then gap
			// set s to the start of the bit then fill with samples to end of the pulse.
			pos = ook_start+(ook_bit*i);
			while (pos < s) {
				carrierAngle = c * tau / full;
				txbufferI[pos] = (int8_t)(127.0 * sin(carrierAngle));
				txbufferQ[pos] = (int8_t)(127.0 * cos(carrierAngle));
				pos++;
				c++;
				c %= full;
			}
		} else {
			// gap first, then pulse
			// set s to the start position of pulse in the bit,
			// then fill with samples to the end of a bit len.
			while (s < ook_start+(ook_bit*(i+1))) {
				carrierAngle = c * tau / full;
				txbufferI[s] = (int8_t)(127.0 * sin(carrierAngle));
				txbufferQ[s] = (int8_t)(127.0 * cos(carrierAngle));
				s++;
				c++;
				c %= full;
			}
		}
	}

	if (ook_pause)
		printf("_________");
	printf("\n");

	printf("%d bits to transmit at %llu Hz with a carrier frequency of %d Hz\n", ook_nbr_bits, freq, ook_carrier);
	printf("Preamble:%d(%dus)   pause:%d(%dus)   bit:%d(%dus)   0:%d(%dus)   1:%d(%dus)\n",
			ook_start, ook_start/8,
			ook_pause, ook_pause/8,
			ook_bit, ook_bit/8,
			ook_0, ook_0/8,
			ook_1, ook_1/8);

	if (ook_counter > 0)
		printf("Repeating the message %d time\n", ook_counter);

	if (!ook_do) {
		printf("Asked to not TX. Abort here.\n");
		printf("Freeing I samples\n");
		free(txbufferI);
		printf("Freeing Q samples\n");
		free(txbufferQ);
		return EXIT_SUCCESS;
	}

	loop = g_main_loop_new(NULL, FALSE);

	// Catch signals that we want to handle gracefully.
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);

	/* Setup the HackRF for transmitting at full power, 8M samples/s, ~27MHz */
	fprintf(stderr, "Setting up the HackRF...\n");

	// Initialize the HackRF support
	result = hackrf_init();
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Open the HackRF device
	result = hackrf_open(&device);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the sample rate
	result = hackrf_set_sample_rate_manual(device, samplerate, 1);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the filter bandwith to default
	result = hackrf_set_baseband_filter_bandwidth(device, hackrf_compute_baseband_filter_bw_round_down_lt(samplerate));
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the gain
	result = hackrf_set_txvga_gain(device, gain);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_txvga_gain() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Set the transmit frequency
	result = hackrf_set_freq(device, freq);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Turn on the amp
	result = hackrf_set_amp_enable(device, (uint8_t)1) ;
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_set_amp_enable() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	/* Transmitting */
	printf("Transmitting, stop with Ctrl-C");
	if (ook_counter > 0)
		printf(" ...or wait until TX is done");
	printf("\n");

	result = hackrf_start_tx(device, tx_callback, NULL);
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_start_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	// Spin until done or killed.
	// while ((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) { sleep (1); }
	g_main_loop_run(loop);

	/* Clean up and shut down */
	printf("Exiting...\n");
	g_main_loop_unref(loop);

	// Shut down the HackRF.
	if (device != NULL) {
		result = hackrf_stop_tx(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_stop_tx() failed: %s (%d)\n", hackrf_error_name(result), result);
		}
		result = hackrf_close(device);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_close() failed: %s (%d)\n", hackrf_error_name(result), result);
		}
		hackrf_exit();
	}

	printf("Freeing I samples\n");
	free(txbufferI);
	printf("Freeing Q samples\n");
	free(txbufferQ);

	return EXIT_SUCCESS;
}
