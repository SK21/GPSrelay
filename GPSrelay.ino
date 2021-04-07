// use with Uno/ethernet shield/SimpleRTK2B
// ethernet shield needs to go on Uno, then SimpleRTK2B on ethernet shield
// need to run a jumper from Uno IOref to SimpleRTK2B IOref if ethernet shield doesn't have it

#include <Ethernet.h>
#include <EthernetUdp.h>

#define Use1008 1   // 0 false, 1 true 
byte mac[] = { 0xDE, 0xAD, 0xEE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 121);

unsigned int localPort = 5432;      // local port to listen on, set in AGIO to send to
// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
EthernetUDP Udp;

const byte MaxBytes = 1000;
char GPSdata[MaxBytes];
char NewByte;
byte Count = 0;
bool GPSheaderFound = false;
char CheckSum[2];

// Blank RTCM3 type 1008 message
const char packet1008[12] = { 0xd3,0x00,0x06,0x3f,0x00,0x00,0x00,0x00,0x00,0x99,0x25,0xca };

byte RTCMbyte = 0;
int RTCMlength = 0;
unsigned int RTCMtype = 0;
int RTCMcount = 0;
bool RTCMheaderFound;
bool RTCMlengthFound;
bool RTCMtypeFound;

void setup()
{
    Serial.begin(38400);
    delay(5000);
    Serial.println("GPSrelay   06-Apr-2021");

    Ethernet.begin(mac, ip);

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("Ethernet shield was not found.");
        while (true)
        {
            // stop
        }
    }

    Serial.println("Ethernet shield found.");

    // start UDP
    Udp.begin(localPort);

    Serial.println("Finished Setup.");
}

void loop()
{
    RelayCorrectionData();
    RelayGPSData();
}

void RelayCorrectionData()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        for (int i = 0; i < packetSize; i++)
        {
            RTCMbyte = Udp.read();
            Serial.write(RTCMbyte);
#if(Use1008)
            Inject1008();
#endif
        }
    }
}

void RelayGPSData()
{
    // GPS data from receiver over serial to AGIO over UDP
    while (Serial.available())
    {
        NewByte = Serial.read();

        if (GPSheaderFound)
        {
            if (NewByte == '$')
            {
                if (CheckSumMatch())
                {
                    // send current message
                    Udp.beginPacket(Udp.remoteIP(), 9999);
                    Udp.write(GPSdata);
                    Udp.endPacket();
                }

                // next message
                GPSdata[0] = NewByte;
                Count = 1;
            }
            else
            {
                // build current message
                GPSdata[Count] = NewByte;
                Count++;
            }
        }
        else
        {
            // find new message
            GPSheaderFound = (NewByte == '$');
            if (GPSheaderFound)
            {
                GPSdata[0] = NewByte;
                Count = 1;
            }
        }
    }
}

bool CheckSumMatch()
{
    bool Result = false;
    if (Count > 4)
    {
        if (GPSdata[0] == '$')
        {
            byte CS = 0;
            for (int i = 1; i < Count; i++)
            {
                if (GPSdata[i] == '*')
                {
                    if (Count - i > 2)
                    {
                        CheckSum[0] = toHex(CS / 16);
                        CheckSum[1] = toHex(CS % 16);
                        Result = (CheckSum[0] == GPSdata[i + 1]) && (CheckSum[1] = GPSdata[i + 2]);
                        break;
                    }
                    else
                    {
                        // wrong length message
                        break;
                    }
                }
                else
                {
                    CS ^= GPSdata[i];   // xor
                }
            }
        }
    }
    return Result;
}

static char toHex(uint8_t nibble)
{
    if (nibble >= 10)
    {
        return nibble + 'A' - 10;
    }
    else
    {
        return nibble + '0';
    }
}

//adapted from https://github.com/torriem/rtcm3add1008
void Inject1008()
{
    if (RTCMheaderFound)
    {
        if (RTCMlengthFound)
        {
            if (RTCMtypeFound)
            {
                RTCMcount++;
                if (RTCMcount == RTCMlength + 3)    // message length + 3 CRC bytes
                {	
                    //inject a 1008 message
                    Serial.write(packet1008, 12);
                    RTCMheaderFound = false;    // start again
                }
            }
            else
            {
                RTCMtype = (RTCMtype << 8) + RTCMbyte;
                RTCMcount++;
                if (RTCMcount == 2)
                {
                    RTCMtypeFound = true;
                    RTCMtype = RTCMtype >> 4;   //isolate type to the most significant 12 bits
                    if ((RTCMtype != 1005) && (RTCMtype != 1006)) RTCMheaderFound = false;  // wrong type, start over
                }
            }
        }
        else
        {
            RTCMlength = (RTCMlength << 8) + RTCMbyte;
            RTCMcount++;
            if (RTCMcount == 2)
            {
                RTCMlengthFound = true;
                RTCMlength = RTCMlength & 0x03FF;   //isolate only the least significant 10 bits
                RTCMcount = 0;
            }
        }
    }
    else
    {
        if (RTCMbyte == 0xD3)
        {
            RTCMheaderFound = true;
            RTCMcount = 0;
            RTCMlength = 0;
            RTCMlengthFound = false;
            RTCMtype = 0;
            RTCMtypeFound = false;
        }
    }
}
