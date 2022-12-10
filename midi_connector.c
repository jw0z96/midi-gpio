#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <alsa/asoundlib.h>
#include <pigpio.h>

// ALSA Sequencer handle
static snd_seq_t *psSeq;

// ALSA Sequencer event
static snd_seq_event_t sEvent;

/*
	The wiring setup is pretty simple, all input pins connect to one leg of
	their own button, the other button legs all connect to a common ground

	Viewing the GPIO pins with the board's USB hub /ethernet at the bottom, the
	pins in use are those closest to the USB hub:

	https://www.etechnophiles.com/wp-content/uploads/2020/12/R-Pi-3-B-Pinout.jpg

				...	...
				|	|
	unused		19	16		BUTTON_3
	BUTTON_2	26	20		BUTTON_1
	GROUND		GND	21		BUTTON_0
*/

// Define an array of pins, BCM numbering scheme
#define NUM_BUTTON_PINS (4)
static const uint8_t aui8ButtonIDs[NUM_BUTTON_PINS] = {
	21,
	20,
	26,
	16,
};

static uint8_t GetPinIndex(uint8_t ui8ID)
{
	switch (ui8ID)
	{
		default: break;
		case 21: return 0;
		case 20: return 1;
		case 26: return 2;
		case 16: return 3;
	}
	return 0;
}

void Destroy(int signal)
{
	// from the example code - reset all pins on sigkill, sigterm
	// set all used pins to input, pull down
	printf("Resetting pin state\n");
	for (uint8_t i = 0; i < NUM_BUTTON_PINS; ++i)
	{
		gpioSetMode(aui8ButtonIDs[i], PI_INPUT);
		gpioSetPullUpDown(aui8ButtonIDs[i], PI_PUD_DOWN);
	}

	gpioTerminate();

	snd_seq_close(psSeq);

	exit(0);
}


void GPIOEvent(int pin, int level, uint32_t tick)
{
	const uint8_t ui8Index = GetPinIndex(pin);

	printf("GPIO %d became %d at %d \n", pin, level, tick);

	// Since we set pull up mode, level 0 is note on, level 1 is note off
	if (level == 0)
	{
		snd_seq_ev_set_noteon(
			&sEvent,
			0, // midi_channel
			36 + ui8Index, // note enum
			80 // velocity
		);
	}
	else
	{
		snd_seq_ev_set_noteoff(
			&sEvent,
			0, // midi_channel
			36 + ui8Index, // note enum
			80 // velocity
		);
	}

	snd_seq_event_output(psSeq, &sEvent);
	snd_seq_drain_output(psSeq);
}

void SetupGPIO(void)
{
	printf("Setup pin state\n");

	// Connect signal handlers to reset any changes we make here
	signal(SIGINT, Destroy);
	signal(SIGTERM, Destroy);
	signal(SIGHUP, Destroy);

	for (uint8_t i = 0; i < NUM_BUTTON_PINS; ++i)
	{
		gpioSetMode(aui8ButtonIDs[i], PI_INPUT);
		gpioSetPullUpDown(aui8ButtonIDs[i], PI_PUD_UP);
		gpioGlitchFilter(aui8ButtonIDs[i], 4000); // set 4ms debounce for gpioSetAlertFunc
		gpioSetAlertFunc(aui8ButtonIDs[i], GPIOEvent);
	}
}

int main(int argc, char *argv[])
{
	static int iError;
	// static snd_seq_t *psSeq;
	static snd_seq_addr_t sDestPort;

	if (argc < 2)
	{
		printf("Please specify output ALSA MIDI CLIENT:PORT or PORT name\n");
		return 1;
	}

	if (gpioInitialise() < 0)
	{
		printf("pigpio initialisation failed\n");
		return 1;
	}

	iError = snd_seq_open(&psSeq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	if (iError < 0)
	{
		printf("Cannot open sequencer %s", snd_strerror(iError));
		return 1; // TODO: graceful exit
	}

	/*
		this is just checking whether the requested port is a valid string
		it'll unpack it into sDestPort if possible, but this doesn't necessarily
		mean the port is available.
	*/
	iError = snd_seq_parse_address(psSeq, &sDestPort, argv[1]);
	if (iError < 0)
	{
		printf("\"%s\" is not a valid CLIENT:PORT, or PORT name\n", argv[1]);
		return 1;
	}

	// TODO: check the port can actualy output - possibly the only way to do
	// this is to create a source port, and then attempt to connect to the
	// destination.

	/*
		set our name (otherwise it's "Client-xxx"),
		not strictly necessary since we don't create a port
	*/
	iError = snd_seq_set_client_name(psSeq, "midi-connector");
	if (iError < 0)
	{
		printf("Cannot set client name %s", snd_strerror(iError));
		return 1; // TODO: graceful exit
	}

	// Setup GPIO pins and their callbacks
	SetupGPIO();

	// Initialise ALSA Event
	snd_seq_ev_clear(&sEvent);
	snd_seq_ev_set_direct(&sEvent);

	// Send directly to the destination port
	snd_seq_ev_set_dest(&sEvent, sDestPort.client, sDestPort.port);

	// The alert callbacks will do everything we need from now on
	// TODO: a version of this which just continually samples buttons & submits
	// midi events from the same loop
	pause();

	// Unreachable
	Destroy(0);

	return 0;
}