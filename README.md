Used at the rover, GPSrelay will send GPS messages to AGIO and RTCM messages to simpleRTK2B. There is the option to inject message 1008 adapted from https://github.com/torriem/rtcm3add1008. To upload to the Uno either remove the simpleRTK2B or disable it with Ucenter.

Stack an ethernet shield on an Uno and simpleRTK2B on the the ethernet shield. Connect a jumper between Uno IOref and simpleRTK2B IOref if the ethernet shield doesn't have it.
Set AGIO Ntrip UDP port to 5432. 

There is a simpleRTK2B rover file adapted from "simpleRTK2B_FW113_Rover_AOrtner_10Hz-01".
