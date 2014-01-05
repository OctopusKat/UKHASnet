UKHASnet

Simple wireless network aimed for use with low power licence exempt
wireless modules. Ideally to be used for a local ground network for
example for remote temperature sensing however will also be designed
to allow for high altitude communications such as via meterological
balloons.

The first prototypes are based upon the RFM22 868MHZ radio modules
and Arduino (AVR based) microprocessors.

Each node periodically transmits a data string as well as listening
and repeating any data that it receives. To keep track of repeating
a repeat value is defined in the data string and each node will
edit the string and reduce this by one. Once it reaches 0 the
repeater will not transmit the string again. To keep track of the 
path of the repeats each node is assigned a ID character which it
appends to the end of the data string. Otherwise the data within the
string can be what ever you want however to make parsing easier 
common data sets will be assigned a start character.

Prototype 1 format (4/1/14)

example: B,001,3>52.0,-0.0[B]
	ID,count,repeat_value>lat,lon[node_ID1]

Prototype 2 format (5/1/14)

example: 3>52.0,-0.0[B]
	repeat_value>lat,lon[node_ID1]
The removal of ID at the beginning of the string is that it is
instead at the end to allow for the path to be constructed. Count 
is data that doesn't necessarily need to be defined and can be 
put in the data section.

Suggested start characters for data within the string

[ : start of the node path
] : end of the node path this needs to be at the end of the packet.
> : location, suggested format is >latitude,longitude,altitude
T : temperature data, multiple values could be seperated by ,
V : voltage data, for monitor battery levels
: : personal message

The various data fields can be stringed together in one message for 
example

3>52.0,-0.0T22.0V3.7:Hello World[B]

As the message is repeated it will gain further IDs appended to the end.

2>52.0,-0.0T22.0V3.7:Hello World[B,D] - note the repeat value is less
1>52.0,-0.0T22.0V3.7:Hello World[B,D,E]
0>52.0,-0.0T22.0V3.7:Hello World[B,D,E,X] - this final string won't be 
repeated.
