#include <stdio.h>
#include <alsa/asoundlib.h>

int main(int argc, char *argv[])
{
	static int iError;
	static snd_seq_t *psSeq;
	static snd_seq_addr_t sDestPort;

	if (argc < 2)
	{
		printf("Please specify output ALSA MIDI CLIENT:PORT or PORT name\n");
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

	// try play some notes for testing
	{
		snd_seq_event_t sEvent;

		// Initialise event
		snd_seq_ev_clear(&sEvent);
		snd_seq_ev_set_direct(&sEvent);

		// Send directly to the destination port
		snd_seq_ev_set_dest(&sEvent, sDestPort.client, sDestPort.port);

		for (int i = 0; i < 10; i++)
		{
			snd_seq_ev_set_noteon(
				&sEvent,
				0, // midi_channel
				36 + i, // note enum
				80 // velocity
			);

			snd_seq_event_output(psSeq, &sEvent);
			// snd_seq_drain_output(psSeq);
			printf("on %i\n", (36 + i));

			// TODO: is draining the output slow? feels like we could queue up events
			// you can sent multiple events via snd_seq_event_output before draining

			snd_seq_ev_set_noteon(
				&sEvent,
				0, // midi_channel
				72 + i, // note enum
				80 // velocity
			);

			snd_seq_event_output(psSeq, &sEvent);
			snd_seq_drain_output(psSeq);
			printf("on %i\n", (72 + i));


			sleep(1);

			snd_seq_ev_set_noteoff(
				&sEvent,
				0, // midi_channel
				36 + i, // note enum
				80 // velocity
			);

			snd_seq_event_output(psSeq, &sEvent);
			snd_seq_drain_output(psSeq);
			printf("off %i\n", (36 + i));

			sleep(1);
		}
	}

	printf("Done\n");
	snd_seq_close(psSeq);

	return 0;
}