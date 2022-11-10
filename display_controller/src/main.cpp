#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
#include "HardwareSerial.h"
#include "RingController.h"
#include "RFIDController.h"
#include "Intercom.h"
#include <math.h>

#define RING_LED_COUNT 12
#define NUM_RINGS 5
#define LED_PIN PB8
#define SERIAL_RESET_TIMEOUT 100

/*
Interrupt driven LED update.
*/

void updateRingController_Int();

/*
Interrupt driven RFID update.
*/

void updateRFIDController_Int(HardwareTimer *);

/*
Print any RFID reader errors to the monitor port.
*/

void printRFIDErrors();

/*
Process and send a new RFID event.
*/

void processRFIDEvent();

/*
Process a new serial message from the com center.
*/

void processSerialMessage();

/*
Conver the RFID tag byte array into a string that can be packed into 
a JSON object and sent to the com center.
*/

void makeUidString(char *s, uint8_t *u, uint8_t uS);

/*
Send the current tag setup. Will list the tags present at each location.
*/

void sendRFIDSetup();

/*
Add a lighting cue coming in over serial to the queue. 
*/

void addLightEffectFromSerial();

/*
Send serial on/off to block messages from com_center while LEDs are updating.
*/

void serialOn();
void serialOff();

TIM_TypeDef *Instance = TIM1;
HardwareTimer *ringTim;

HardwareSerial Serial2(USART2);
uint8_t numReaders = 5;
uint8_t RFIDPins[] = {PA7, PB0, PB1, PB10, PB11};
uint8_t RFIDPinMap[] = {0, 2, 4, 1, 3};
uint8_t locationAdjust = 0;

/*
The LED ring controller.
*/

RingController *ringController;

/*
The RFID controller.
*/

RFIDController *rfidController;

/* 
Toggle serial monitor on and off
*/

bool bSerialMonitorOn = false;

bool RFIDUpdate = false;
bool bReceivingData = false;
uint8_t bytesToReceive = 0;

char serialMsg[512];
char serialProcessBuffer[512];
uint16_t serialMsgIndex = 0;
bool newSerialMsg = false;

bool bResetSerial = false;
uint32_t timeOfLastSerialStart = 0;

uint32_t timeOfSetup = 0;

void updateRingController_Int()
{
    serialOff();
    ringController->update();
    if (ringController->shouldShow())
    {
        ringController->show();
    }
    serialOn();
}

void updateRFIDController_Int(HardwareTimer *)
{
    rfidController->update();
}

void reset()
{
    NVIC_SystemReset();
}

void printRFIDErrors()
{
    uint8_t errorCount;
    uint8_t errors[numReaders];
    rfidController->getReaderErrors(&errorCount, &errors[0]);
    Serial2.print("Reader Errors: ");
    for (uint8_t i = 0; i < errorCount; i++)
    {
        Serial2.print(errors[i]);
        Serial2.print(" ");
    }
    Serial2.println();
}

void makeUidString(char *s, uint8_t *u, uint8_t uS)
{
    uint8_t pos = 2;
    for (uint8_t i = 0; i < uS; i++)
    {
        if (i == 0)
        {
            sprintf(s, "%02X", u[i]);
        }
        else
        {
            sprintf(&s[pos], ":%02X", u[i]);
            pos += 3;
        }
    }
}

void sendRFIDSetup()
{
    char s[64];
    bool tagsPresent[numReaders];
    uint8_t uids[numReaders * 7];
    uint8_t uidLengths[numReaders];
    char msg[512];
    rfidController->getCurrentSetup(tagsPresent, uids, uidLengths);
    IntercomMessage im(MessageType::TAG_SETUP);
    for (uint8_t i = 0; i < numReaders; i++)
    {
        if (tagsPresent[i])
        {
            makeUidString(s, &uids[i * 7], uidLengths[i]);
            im.addSetupItem(i + locationAdjust, s);
        }
        else
        {
            im.addSetupItem(i + locationAdjust, "NO_TAG_PRESENT");
        }
    }
    im.toString(msg, 512);
    Serial1.println(msg);
    Serial1.flush();
    Serial2.println(msg);
    Serial2.flush();
}

void addLightEffectFromSerial()
{
    LightEffectData d;
    memcpy(&d, &serialProcessBuffer[2], 16);
    LightEffect le(&d);

    //Serial2.println("le");
    if (bSerialMonitorOn)
    {
        Serial2.print(le.getType());
        Serial2.print(", ");
        Serial2.print(le.getStartTime());
        Serial2.print(", ");
        Serial2.print(le.getAnimationTime());
        Serial2.print(", ");
        Serial2.print(le.getLocation());
        Serial2.print(", ");
        Serial2.print(le.getRedLevel());
        Serial2.print(", ");
        Serial2.print(le.getGreenLevel());
        Serial2.print(", ");
        Serial2.print(le.getBlueLevel());
        Serial2.print(", ");
        Serial2.println(le.getWhiteLevel());
    }

    ringController->addEvent(le.getStartTime(),
                             le.getType(),
                             le.getAnimationTime(),
                             le.getLocation(),
                             le.getRedLevel(),
                             le.getGreenLevel(),
                             le.getBlueLevel(),
                             le.getWhiteLevel());
}

void processRFIDEvent()
{
    uint8_t uidSize;
    uint8_t uid[7];
    char uidString[64];
    char msg[256];
    IntercomMessage im(MessageType::TAG_CHANGE);
    RFIDEvent *e = rfidController->getEvent();
    if (e->getEventType() == RFIDEventType::TAG_IN)
    {
        im.addStringParam("state", "down");
    }
    else
    {
        im.addStringParam("state", "up");
    }
    char loc[16];
    sprintf(loc, "%d", e->getLocation() + locationAdjust);
    im.addStringParam("location", loc);
    e->getUid(&uidSize, uid);
    makeUidString(uidString, &uid[0], uidSize);
    im.addStringParam("id", uidString);
    im.toString(msg, 256);
    Serial1.println(msg);
    Serial1.flush();
    if(bSerialMonitorOn) {
        Serial2.println("RFID");
        Serial2.println(msg);
        Serial2.flush();
    }
}

void processSerialMessage()
{
    uint8_t mId = serialProcessBuffer[0];
    if (bSerialMonitorOn) {
        Serial2.print("Message id - ");
        Serial2.println(mId, DEC);
    }
    char ack[128];
    sprintf(ack, "{\"event\":\"ACK\", \"params\":{\"id\":\"%d\"}}", mId);
    if (bSerialMonitorOn) Serial2.println(ack);
    Serial1.println(ack);
    delay(10);
    switch (serialProcessBuffer[1])
    {
    case INTERCOM_TAG_SETUP:
        if (bSerialMonitorOn) Serial2.println("TAG_SETUP");
        /*
        if ((serialProcessBuffer[1] > 5) && (serialProcessBuffer[1] != 0xAB))
        {
            break;
        }
        */
        if((serialProcessBuffer[2] != 0) && (serialProcessBuffer[2] != 5)) {
            break;
        }
        locationAdjust = serialProcessBuffer[2];
        if (bSerialMonitorOn) {
            Serial2.print("Location adjust = ");
            Serial2.println(locationAdjust, DEC);
        }
        sendRFIDSetup();
        break;
    case INTERCOM_LIGHT_EFFECT:
        if (bSerialMonitorOn) Serial2.println("LIGHT_EFFECT");
        addLightEffectFromSerial();
        break;
    case INTERCOM_RESET:
        reset();
        break;
    case INTERCOM_MONITOR:
        bSerialMonitorOn = serialProcessBuffer[2];
        break;
    default:
        break;
    }
}

void serialOn()
{
    char status[128];
    sprintf(status, "{\"event\":\"SERIAL_STATUS\", \"params\":{\"status\":\"on\"}}");
    if (bSerialMonitorOn) Serial2.println(status);
    Serial1.println(status);
}

void serialOff()
{
    char status[128];
    sprintf(status, "{\"event\":\"SERIAL_STATUS\", \"params\":{\"status\":\"off\"}}");
    if (bSerialMonitorOn) Serial2.println(status);
    Serial1.println(status);
}

void setup()
{

    pinMode(PC13, OUTPUT);
    pinMode(PA15, OUTPUT);
    digitalWrite(PC13, HIGH);

    Serial1.begin(57600);
    Serial2.begin(9600);
    ringController = new RingController(RING_LED_COUNT, NUM_RINGS, LED_PIN);
    rfidController = new RFIDController(numReaders, &RFIDPins[0], &RFIDPinMap[0]);

    Serial2.println("Starting RFID readers.");

    rfidController->init();
    if (!rfidController->getReaderStatus())
    {
        printRFIDErrors();
    }

    /*
    Setup timer for LED rings.
    Currently disable. Creates smoother animations but interrupts incoming 
    serial messages.
    */

    //ringTim = new HardwareTimer(Instance);
    //ringTim->setOverflow(30, HERTZ_FORMAT);
    //ringTim->attachInterrupt(updateRingController_Int);
    //ringTim->resume();

    Serial2.println("Init complete.");

    digitalWrite(PC13, LOW);
}

void loop()
{

    rfidController->update();

    if (rfidController->hasEvent())
    {
        processRFIDEvent();
        RFIDUpdate = false;
    }

    while (Serial1.available() > 0)
    {
        bResetSerial = false;
        uint8_t b = Serial1.read();
        if (!bReceivingData)
        {
            bReceivingData = true;
            serialMsgIndex = 0;
            bytesToReceive = b;
            timeOfLastSerialStart = millis();
        }
        else
        {
            serialMsg[serialMsgIndex++] = b;
            if (serialMsgIndex >= bytesToReceive)
            {
                bReceivingData = false;
                serialMsgIndex = 0;
                memcpy(serialProcessBuffer, serialMsg, bytesToReceive);
                newSerialMsg = true;
            }
        }
        bResetSerial = true;
    }

    if (newSerialMsg)
    {
        processSerialMessage();
        newSerialMsg = false;
    }

    if (((millis() - timeOfLastSerialStart) > SERIAL_RESET_TIMEOUT) && bReceivingData)
    {
        //Serial2.println("Resetting serial.");
        bReceivingData = false;
        serialMsgIndex = 0;
    }

    ringController->update();

    if (ringController->shouldShow())
    {
        serialOff();
        ringController->show();
        serialOn();
    }
}