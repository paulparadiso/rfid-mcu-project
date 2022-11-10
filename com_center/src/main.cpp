#include <Arduino.h>
#include "BLEUartManager.h"
#include "MessageManager.h"
#include "HardwareSerial.h"
#include "SoftwareSerial.h"
#include "Intercom.h"

/*
Name that will be assigned to the BLE device. Set this as necessary for each device.
*/

#define UART_NAME "MAC_BLE_001"

/*
Size of the serial buffers.
*/

#define SERIAL_BUFFER_SIZE 512

/*
Delay time between successive calls to run the attract sequence when activated.
*/

#define ATTRACT_DELAY 30000

/*
Amount of time to wait between serial retries and number of retries to  attempt before dropping 
message from cue.
*/

#define MSG_ACK_WAIT 100
#define MAX_MSG_RETRIES 25

/*
Number of ble message buffers for receiving incoming messages.
*/

#define NUM_BLE_BUFFERS 10

/*
The serial ports used for communicating with the display control units.
The pin assignments for HardwareSerial1 (DCSerial2) need to be changed in 
HardwareSerial.cpp to 25 (RX) and 26 (TX).
*/

HardwareSerial DCSerial1(2);
HardwareSerial DCSerial2(1);

/*
Enum for the message targets of serial messages. A target of NONE disables
a message in the cue.
*/

enum class MessageTarget
{
    ONE,
    TWO,
    ALL,
    NONE
};

/*
Struct used for serial messages.
*/

typedef struct _SerialMessage
{
    uint8_t id;
    MessageTarget target = MessageTarget::NONE;
    uint8_t ack;
    uint8_t msgLength = 0;
    uint32_t lastSend;
    uint8_t retries = 0;
    uint8_t msg[32];
} SerialMessage;

/*
Callback used by BLE device for new messages.
*/

void msgReceived(char *msg);

/*
Properly format and send an outgoing BLE message.
*/

void packageAndSend(char *b);

/*
Add a serial message to the cue. This will create a SerialMessage of a given length
to be sent to a given target. 
*/

void addMessage(MessageTarget target, uint8_t *msg, uint8_t msgLength, uint8_t pos);

/*
Check the serial cue for messages that need to be sent or resend message that haven't been 
ack'd.
*/

void checkSerialSendBuffers();

/*
Send a serial message to a given target.
*/

void sendSerialMessage(MessageTarget target, uint8_t *buf, uint8_t s);

/*
Add the GET_SETUP message to the cue for both targets.
*/

void sendGetSetup();

/*
Add the RESET message to the cue for both targets.
*/

void sendReset();

/*
Send the PONG message over BLE.
*/

void sendPong();

/*
Add the attract sequence to the cue.
*/

void sendAttract();

/*
Add a light effect command to the cue.
*/

void sendLightEffect(Message *m);

/*
Confirm that a BLE message was received. This is not currently implemented 
on both ends.
*/

void confirmMessage(Message *m);

/*
Process an incoming BLE message.
*/

void processMessage(Message *m);

/*
Process a message in one of the display control buffers.
*/

void processDCBuffer(int b);

/*
    Update task passed to freertos to switch cores.
*/

void updateTask(void *parameter);
TaskHandle_t updateTaskHandle;

/* 
Toggle monitor on and off
*/

bool bSerialMonitorOn = false;

/*
Buffer and flag for processing BLE messages.
*/

char msgBuffer[SERIAL_BUFFER_SIZE * 2 * NUM_BLE_BUFFERS];
bool bMsgReceived = false;
char bleBuffer[SERIAL_BUFFER_SIZE * 2];
uint8_t msgBufferWritePos = 0;
uint8_t msgBufferReadPos = 0;

/*
Buffers, flags, and indexes used to process incoming serial messages.
*/

char dcBuffer[SERIAL_BUFFER_SIZE];
char dcBuffer2[SERIAL_BUFFER_SIZE];
char tmp[SERIAL_BUFFER_SIZE];
uint16_t dcIndex = 0;
uint16_t dcMsgLength = 0;
uint16_t dcIndex2 = 0;
uint16_t dcMsgLength2 = 0;
bool bProcessDCBuffer = false;
uint8_t currentDCBuffer = 0;
bool bDC1On = true;
bool bDC2On = true;

/*
JSON Buffer and TagSetupMessage used process incoming tag setup messages.
*/

char jsonContainer[1024];
TagSetupMessage tsm;
uint8_t tsmCount = 0;
bool bSendJson = false;
bool bWaitingForSetup = false;

/*
PONG String to send in response to PING message.
*/

char *pong = "{\"event\":\"PONG\"}\n";

/*
Serial message ring buffer used to store cued serial messages.
*/

SerialMessage messageBuffer[32];
uint8_t currentMessageBuffer = 0;
uint8_t currentMessageId = 0;
uint8_t currentMessageReadBuffer = 0;

/*
Flag and timer for attract loop.
*/

bool bAttractOn = false;
uint32_t timeOfLastAttract = 0;

void addMessage(MessageTarget target, uint8_t *msg, uint8_t msgLength, uint8_t pos = 0)
{
    if (msgLength > 31)
    {
        return;
    }
    uint8_t buf = currentMessageBuffer;
    if (pos != 0) {
        buf = pos;  
    }
    messageBuffer[buf].ack = 0;
    messageBuffer[buf].id = currentMessageId;
    messageBuffer[buf].msgLength = msgLength + 1;
    messageBuffer[buf].lastSend = 0;
    messageBuffer[buf].msg[0] = currentMessageId;
    messageBuffer[buf].target = target;
    messageBuffer[buf].retries = 0;
    memcpy(&messageBuffer[buf].msg[1], msg, msgLength);
    currentMessageBuffer = (currentMessageBuffer + 1) % 16;
    currentMessageId = (currentMessageId + 1) % 255;
}

void checkSerialSendBuffers()
{
    for (int i = 0; i < 16; i++)
    {
        if (messageBuffer[i].target != MessageTarget::NONE)
        {
            if (((messageBuffer[i].target == MessageTarget::ONE) && !bDC1On) || ((messageBuffer[i].target == MessageTarget::TWO) && !bDC2On))
            {
                //currentMessageReadBuffer = (currentMessageReadBuffer + 1) % 16;
                return;
            }
            if (messageBuffer[i].ack == 0 && ((millis() - messageBuffer[i].lastSend) > MSG_ACK_WAIT))
            {
                sendSerialMessage(messageBuffer[i].target, &messageBuffer[i].msg[0], messageBuffer[i].msgLength);
                messageBuffer[i].retries++;
                if (messageBuffer[i].retries >= MAX_MSG_RETRIES)
                {
                    if(bSerialMonitorOn) {
                        Serial.print("Giving up on message - ");
                        Serial.println(messageBuffer[i].id, DEC);
                    }
                    messageBuffer[i].target = MessageTarget::NONE;
                    messageBuffer[i].ack = 1;
                }
                messageBuffer[i].lastSend = millis();
                //currentMessageReadBuffer = (currentMessageReadBuffer + 1) % 16;
                return;
            }
        }
        currentMessageReadBuffer = (currentMessageReadBuffer + 1) % 16;
    }
}

void msgReceived(char *msg)
{
    memcpy(&msgBuffer[SERIAL_BUFFER_SIZE * 2 * msgBufferWritePos], msg, 1024);
    bMsgReceived = true;
    msgBufferWritePos = (msgBufferWritePos + 1) % NUM_BLE_BUFFERS;
}

void packageAndSend(char *b)
{
    memset(bleBuffer, 0, SERIAL_BUFFER_SIZE * 2);
    sprintf(bleBuffer, "[%s]\n", b);
    if(bSerialMonitorOn) {
        Serial.print("Sending - ");
        Serial.println(bleBuffer);
    }
    BLEUartManager::getInstance()->send((uint8_t *)bleBuffer, strlen(bleBuffer));
}

void sendSerialMessage(MessageTarget target, uint8_t *buf, uint8_t s)
{
    if(bSerialMonitorOn) Serial.println("Sending Message.");
    if (target == MessageTarget::ONE || target == MessageTarget::ALL)
    {
        if(bSerialMonitorOn) Serial.println("Target 1.");
        DCSerial1.write(s);
        DCSerial1.write(buf, s);
        delay(1);
        //DCSerial1.flush();
    }
    if (target == MessageTarget::TWO || target == MessageTarget::ALL)
    {
        if(bSerialMonitorOn) Serial.println("Target 2.");
        DCSerial2.write(s);
        DCSerial2.write(buf, s);
        delay(1);
        //DCSerial2.flush();
    }
}

void sendGetSetup()
{
    uint8_t command[2] = {INTERCOM_TAG_SETUP, 0};
    addMessage(MessageTarget::ONE, command, 2);
    command[1] = 5;
    addMessage(MessageTarget::TWO, command, 2);
    bWaitingForSetup = true;
    tsmCount = 0;
}

void sendReset()
{
    uint8_t command[2] = {1, INTERCOM_RESET};
    sendSerialMessage(MessageTarget::ALL, &command[0], 2);
}

void sendPong()
{
    packageAndSend(pong);
}

/*
void sendAttract() {
    uint8_t r = random(255);
    uint8_t g = random(255);
    uint8_t b = random(255);
    uint32_t delayTime = 200;
    uint32_t fadeTime = 1000;
    LightEffect leOn;
    LightEffect leOff;
    leOn.setRedLevel(r);
    leOn.setBlueLevel(b);
    leOn.setGreenLevel(g);
    leOn.setAnimationTime(fadeTime);
    leOff.setAnimationTime(fadeTime);
    leOff.setWhiteLevel(255);
    uint8_t msg[17];
    msg[0] =INTERCOM_LIGHT_EFFECT;
    uint8_t opOrder[10] = {0, 5, 1, 6, 2, 7, 3, 8, 4, 9};
    leOn.setType(LIGHT_EFFECT_RING_FADE_ON);
    for(int j = 0; j < 10; j++) {
        //char msg[256]10
        uint8_t i = opOrder[j];
        if (i < 5) {
            leOn.setLocation(i);
            leOn.setStartTime(delayTime * i);
            leOn.makeByteArray(&msg[1]);
            sendSerialMessage(MessageTarget::ONE, msg, 17);
            delay(10);
        } else {
            leOn.setLocation(i - 5);
            leOn.setStartTime(delayTime * i);
            leOn.makeByteArray(&msg[1]);
            sendSerialMessage(MessageTarget::TWO, msg, 17);
            delay(10);
        }
    }
    for(int k = 0; k < 10; k++) {
        uint8_t i = opOrder[k];
        if(i < 5) {
            leOff.setLocation(i);
            leOff.setStartTime((delayTime * 10) + (delayTime * i));
            leOff.makeByteArray(&msg[1]);
            sendSerialMessage(MessageTarget::ONE, msg, 17);
            delay(10);
        } else {
            leOff.setLocation(i - 5);
            leOff.setStartTime((delayTime * 10) + (delayTime * i));
            leOff.makeByteArray(&msg[1]);
            sendSerialMessage(MessageTarget::TWO, msg, 17);
            delay(10);
        }   
    }
}
*/

void sendAttract()
{
    uint8_t r = random(127);
    uint8_t g = random(127);
    uint8_t b = random(127);
    uint32_t delayTime = 2000;
    uint32_t fadeTime = 1000;
    LightEffect leOn;
    LightEffect leOff;
    leOn.setType(LIGHT_EFFECT_RING_FADE_ON);
    leOn.setLocation(255);
    leOff.setLocation(255);
    leOn.setRedLevel(r);
    leOn.setBlueLevel(b);
    leOn.setGreenLevel(g);
    leOff.setStartTime(delayTime);
    leOff.setType(LIGHT_EFFECT_RING_FADE_ON);
    leOff.setWhiteLevel(127);
    leOn.setAnimationTime(fadeTime);
    leOff.setAnimationTime(fadeTime);
    uint8_t msg[17];
    leOn.makeByteArray(&msg[1]);
    msg[0] = INTERCOM_LIGHT_EFFECT;
    addMessage(MessageTarget::ONE, msg, 17);
    addMessage(MessageTarget::TWO, msg, 17);
    leOff.makeByteArray(&msg[1]);
    addMessage(MessageTarget::ONE, msg, 17);
    addMessage(MessageTarget::TWO, msg, 17);
}

void sendLightEffect(Message *m)
{
    MessageTarget target = MessageTarget::ALL;
    char v[64];
    int location = 255;
    if (m->getParam("location", v) == 0)
    {
        location = atoi(v);
        if (location != 255)
        {
            if (location < 5)
            {
                target = MessageTarget::ONE;
            }
            else
            {
                target = MessageTarget::TWO;
                location = location - 5;
            }
        }
    }
    uint8_t rawMsg[17];
    LightEffect le;
    le.setType(LIGHT_EFFECT_RING_FADE_ON);
    le.setLocation(location);
    if (m->getParam("startTime", v) == 0)
    {
        le.setStartTime(atoi(v));
    }
    if (m->getParam("animationTime", v) == 0)
    {
        le.setAnimationTime(atoi(v));
    }
    if (m->getParam("redLevel", v) == 0)
    {
        uint8_t rl = atoi(v);
        /*
        if(rl == 128) {
            rl = 127;
        }
        */
        le.setRedLevel(rl);
    }
    if (m->getParam("greenLevel", v) == 0)
    {
        uint8_t gl = atoi(v);
        if(gl == 128) {
            gl = 127;
        }
        le.setGreenLevel(gl);
    }
    if (m->getParam("blueLevel", v) == 0)
    {
        uint8_t bl = atoi(v);
        if(bl == 128) {
            bl = 127;
        }
        le.setBlueLevel(bl);
    }
    if (m->getParam("whiteLevel", v) == 0)
    {
        uint8_t wl = atoi(v);
        if(wl == 128) {
            wl = 127;
        }
        le.setWhiteLevel(wl);
    }
    rawMsg[0] = INTERCOM_LIGHT_EFFECT;
    le.makeByteArray(&rawMsg[1]);
    if (target == MessageTarget::ALL)
    {
        addMessage(MessageTarget::ONE, (uint8_t *)&rawMsg[0], 17);
        addMessage(MessageTarget::TWO, (uint8_t *)&rawMsg[0], 17);
    }
    else
    {
        addMessage(target, (uint8_t *)&rawMsg[0], 17);
    }
}

void confirmMessage(Message *m)
{
    char output[512];
    Message c;
    c.setType(MessageEventType::MESSAGE_RECEIVED);
    char key[] = "id";
    char val[64];
    m->getId(val);
    c.addParam(key, val);
    memset(output, 0, 512);
    MessageManager::stringFromMessage(c, output);
    BLEUartManager::getInstance()->send((uint8_t *)output, strlen(output));
}

void sendSerialMonitorState() {
    uint8_t rawMsg[2] = {INTERCOM_MONITOR, bSerialMonitorOn};
    addMessage(MessageTarget::ONE, (uint8_t*)&rawMsg[0], 2);
    addMessage(MessageTarget::TWO, (uint8_t*)&rawMsg[0], 2);
}

void setSerialMonitorState(Message *m) {
    char val[64];
    if(m->getParam("state", val) == 0) {
        if(strcmp(val, "on") == 0) {
            bSerialMonitorOn = true;
        } else {
            bSerialMonitorOn = false;
        }
    }
}

void processMessage(Message *m)
{
    bAttractOn = false;
    switch (m->getType())
    {
    case MessageEventType::GET_SETUP:
        sendGetSetup();
        break;
    case MessageEventType::PING:
        sendPong();
        break;
    case MessageEventType::ATTRACT:
        bAttractOn = true;
        timeOfLastAttract = millis();
        sendAttract();
        break;
    case MessageEventType::RESET:
        sendReset();
        break;
    case MessageEventType::MONITOR:
        setSerialMonitorState(m);
        break;
    default:
        sendLightEffect(m);
        break;
    }
}

void processDCBuffer(int b)
{
    char *p;
    if (b == 0)
    {
        p = dcBuffer;
    }
    else
    {
        p = dcBuffer2;
    }
    memset(tmp, 0, SERIAL_BUFFER_SIZE);
    memcpy(tmp, p, strlen(p));
    Message m = MessageManager::parseMessage(tmp);
    if (m.getType() == MessageEventType::TAG_SETUP)
    {
        if (!bWaitingForSetup) {
            return;
        }
        bAttractOn = false;
        //Serial.print("SETUP FROM ");
        //Serial.println(b);
        int add = 0;
        if (b == 1)
        {
            add = 5;
        }
        for (int i = 0; i < 5; i++)
        {
            char t[64];
            memset(t, 0, 64);
            m.getTag(i + add, t);
            tsm.addTag(i + add, t);
            tsmCount++;
        }

        if (tsmCount >= 10)
        {
            tsm.getJson(jsonContainer);
            bSendJson = true;
            bWaitingForSetup = false;
            tsmCount = 0;
        }
    }
    else if (m.getType() == MessageEventType::ACK)
    {
        char t[64];
        m.getParam("id", t);
        uint8_t id = atoi(t);
        if(bSerialMonitorOn) {
            Serial.print("Message ACK - ");
            Serial.println(id, DEC);
        }
        for (int i = 0; i < 16; i++)
        {
            if (messageBuffer[i].id == id)
            {
                messageBuffer[i].ack = 1;
                messageBuffer[i].target = MessageTarget::NONE;
            }
        }
    }
    else if (m.getType() == MessageEventType::SERIAL_STATUS)
    {
        char status[64];
        m.getParam("status", status);
        if (strcmp(status, "on") == 0)
        {
            if (b == 0)
            {
                bDC1On = true;
                //if(bSerialMonitorOn) Serial.println("Turning on dc1");
            }
            else
            {
                bDC2On = true;
                //if(bSerialMonitorOn) Serial.println("Turning on dc2");
            }
        }
        else
        {
            if (b == 0)
            {
                bDC1On = false;
                //if(bSerialMonitorOn) Serial.println("Turning off dc1");
            }
            else
            {
                bDC2On = false;
                //if(bSerialMonitorOn) Serial.println("Turning off dc2 ");
            }
        }
    }
    else if (m.getType() != MessageEventType::FAIL)
    {
        bAttractOn = false;
        packageAndSend(p);
        //Serial.println("Message sent.");
    }
    memset(p, 0, SERIAL_BUFFER_SIZE);   
}

void setup()
{
    Serial.begin(9600);
    //DCSerial0.begin(9600);
    DCSerial1.begin(57600);
    DCSerial2.begin(57600);
    BLEUartManager::getInstance()->init(UART_NAME, msgReceived);
    memset(dcBuffer, 0, SERIAL_BUFFER_SIZE);
    memset(dcBuffer2, 0, SERIAL_BUFFER_SIZE);

    /*
    Assign updates to run on core 0 using freertos API.
    */

    xTaskCreatePinnedToCore(
        updateTask,
        "Update",
        16384,
        NULL,
        1,
        &updateTaskHandle,
        0
    );

    delay(10000);
    sendReset();
}

void updateTask(void * parameter)
{
    for (;;)
    {
        if (msgBufferWritePos != msgBufferReadPos)
        {
            if(bSerialMonitorOn) {
                Serial.print("Received message - ");
                Serial.println(&msgBuffer[SERIAL_BUFFER_SIZE * 2 * msgBufferReadPos]);
            }
            //char msg[SERIAL_BUFFER_SIZE * 2];
            //memcpy(msg, msgBuffer, strlen(msgBuffer));
            Message m = MessageManager::parseMessage(&msgBuffer[SERIAL_BUFFER_SIZE * 2 * msgBufferReadPos]);
            if (m.isValid()) {
                //confirmMessage(&m);
                processMessage(&m);
                //delay(20);
            }
            //memset(msgBuffer, 0, SERIAL_BUFFER_SIZE * 2);
            msgBufferReadPos = (msgBufferReadPos + 1) % NUM_BLE_BUFFERS;
            bMsgReceived = false;
        }

        if (bSendJson)
        {
            packageAndSend(jsonContainer);
            bSendJson = false;
            memset(jsonContainer, 0, 1024);
        }

        checkSerialSendBuffers();

        while (DCSerial1.available())
        {
            char in = DCSerial1.read();
            //Serial.print(in);
            if (in != '\n')
            {
                dcBuffer[dcIndex++] = in;
            }
            else
            {
                dcBuffer[dcIndex] = '\n';
                dcMsgLength = dcIndex;
                dcIndex = 0;
                //bProcessDCBuffer = true;
                currentDCBuffer = 0;
                processDCBuffer(currentDCBuffer);
            }
        }

        while (DCSerial2.available())
        {
            char in = DCSerial2.read();
            //Serial.print(in);
            if (in != '\n')
            {
                dcBuffer2[dcIndex2++] = in;
            }
            else
            {
                dcBuffer2[dcIndex2] = '\n';
                dcMsgLength2 = dcIndex2;
                dcIndex2 = 0;
                //bProcessDCBuffer = true;
                currentDCBuffer = 1;
                processDCBuffer(currentDCBuffer);
            }
        }

        /*
        if (bProcessDCBuffer) {
            processDCBuffer(currentDCBuffer);
            bProcessDCBuffer = false;
        }
        */

        if (bAttractOn && ((millis() - timeOfLastAttract) > ATTRACT_DELAY))
        {
            sendAttract();
            timeOfLastAttract = millis();
        }

        vTaskDelay(10);
    }
}

void loop()
{
}
