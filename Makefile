all:
	@gcc -o midi-connector midi_connector.c -lasound

clean:
	@rm midi-connector
