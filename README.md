# midi-gpio
sending midi events on GPIO events
## Usage
`midi-connector <CLIENT:PORT>` or `midi-connector <NAME>`

`midi-connector` will output midi events directly to the specified client port or port name, as specified by `aconnect -l`

designed for use with [btmidi-server](https://github.com/oxesoft/bluez/blob/midi/tools/btmidi-server.c)
