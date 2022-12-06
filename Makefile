all:
	@gcc -o midi-connector midi_connector.c -lasound -lpigpio

clean:
	@rm midi-connector
