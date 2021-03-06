/*
    Copyright (c) 2019 Niek Beukema, SODAQ.  All rights reserved.

    This file is part of the unofficial r410m library.

    This program is free software: you can redistribute it and/or modify 
    it under the terms of the GNU Affero General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version. 
    
    This program is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
    See the GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public 
    License along with this program. If not, see
    <https://www.gnu.org/licenses/>. 
*/

#include "Sodaq_nbIOT.h"
#include "Sodaq_wdt.h"

#if defined(ARDUINO_SODAQ_SARA)
/* SODAQ SARA */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define DEBUG_STREAM_BAUD 115200
#define powerPin SARA_ENABLE
#define enablePin SARA_TX_ENABLE
#elif defined(ARDUINO_SAMD_MKRNB1500)
#define DEBUG_STREAM Serial
#define DEBUG_STREAM_BAUD 9600
#define MODEM_STREAM SerialSARA
#define powerPin SARA_PWR_ON 
#define enablePin SARA_RESETN  
#else
#error "Please use one of the listed boards or add your board."
#endif



#define STARTUP_DELAY 5000

const char* apn = "nb.inetd.gdsp";    // Vodafone APN
const char* cdp = "172.16.14.22";
uint8_t cid = 1;                      // Unsupported for R4XX
const uint8_t band = 20;              // Narrowband band
const char* forceOperator = "20404";  // Vodafone operator ID
const int maxByteReceive = 100;       // Read maximum 100*2 bytes at UDP read

// const char* apn = "cdp.iot.t-mobile.nl";
// const char* cdp = "172.27.131.100";
// uint8_t cid = 1;
// const uint8_t band = 8;
// const char* forceOperator = "20416"; // optional - depends on SIM / network

nbIOT nbiot;

void sendOverSocket()
{
    int localPort = 16666;
    int socketID = nbiot.createSocket(localPort);

    if (socketID >= 7 || socketID < 0) {
        DEBUG_STREAM.println("Failed to create socket");
        return;
    } else {
        DEBUG_STREAM.println("Created socket!");
        DEBUG_STREAM.println();
        DEBUG_STREAM.println("Sending message through UDP");
        
        sendMessage(socketID);
        // wait for data
        waitForResponse();

        nbiot.closeSocket(socketID);
        DEBUG_STREAM.println();
    }
}

void setup()
{
    sodaq_wdt_safe_delay(STARTUP_DELAY);

    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    DEBUG_STREAM.println("Initializing library");

    MODEM_STREAM.begin(nbiot.getSaraR4Baudrate());
    nbiot.setDiag(DEBUG_STREAM);
    nbiot.init(MODEM_STREAM, powerPin, enablePin, SARA_R4XX_TOGGLE, cid);

    if (!nbiot.connect(apn, cdp, forceOperator, band)) {
        DEBUG_STREAM.println("Failed to connect to the modem!");
    } else {
        sendOverSocket();
    }
}

void loop()
{
    sodaq_wdt_safe_delay(60000);
    if (!nbiot.isConnected()) {
        if (!nbiot.connect(apn, cdp, forceOperator, band)) {
            DEBUG_STREAM.println("Failed to connect to the modem!");
        }
    }
    else {
        sendOverSocket();
    }
}

void sendMessage(int socketID) {
    //String is converted to hex in library.
    const char* strBuffer = "This is a test string";
    size_t size = strlen(strBuffer);


    int lengthSent = nbiot.socketSend(socketID, "195.34.89.241", 7, strBuffer); // ip "195.34.89.241", port 7 is the ublox echo service
    
    DEBUG_STREAM.print("String length vs sent: ");
    DEBUG_STREAM.print(size);
    DEBUG_STREAM.print(" vs ");
    DEBUG_STREAM.println(lengthSent);
}

void waitForResponse() {
      if (nbiot.waitForUDPResponse()) {
        DEBUG_STREAM.println("Received response!");

        while (nbiot.hasPendingUDPBytes()) {
            char data[200];
            // read two bytes at a time
            SaraN2UDPPacketMetadata p;
            int size = nbiot.socketReceiveHex(data, maxByteReceive, &p);

            if (size) {
                DEBUG_STREAM.println("UDP Packet received!");
                DEBUG_STREAM.println(data);
                // p is a pointer to memory that is owned by nbiot class
                DEBUG_STREAM.println(p.socketID);
                DEBUG_STREAM.println(p.ip);
                DEBUG_STREAM.println(p.port);
                DEBUG_STREAM.println(p.length);
                DEBUG_STREAM.println(p.remainingLength);
                DEBUG_STREAM.println("End of data");
            }
            else {
                DEBUG_STREAM.println("Receive failed!");
            }
        }
    }
    else {
        DEBUG_STREAM.println("Timed-out!");
    }
}
