#include "RFIDController.h"

RFIDEvent::RFIDEvent() {

}

RFIDEvent::RFIDEvent(RFIDEventType type, uint8_t l, uint8_t s, uint8_t* id){

}
    
uint8_t RFIDEvent::getLocation() {
    return location;
}

void RFIDEvent::setLocation(uint8_t l) {
    location = l;
}
    
uint8_t RFIDEvent::getUidSize() {
    return uidSize;
}   

void RFIDEvent::setUidSize(uint8_t s) {
    uidSize = s;
}

uint8_t* RFIDEvent::getUid() {
    return &uid[0];
}
    
void RFIDEvent::getUid(uint8_t* s, uint8_t* u) {
    *s = uidSize;
    memcpy(u, uid, uidSize);
}

void RFIDEvent::setUid(uint8_t s, uint8_t* u) {
    uidSize = s;
    memcpy(uid, u, uidSize);
}

RFIDEventType RFIDEvent::getEventType() {
    return eventType;
}   

void RFIDEvent::setEventType(RFIDEventType type) {
    eventType = type;
}

RFIDController::RFIDController() {

}

RFIDController::RFIDController(uint8_t nReaders, uint8_t* pins, uint8_t* pinMap) {
    numActiveReaders = nReaders;
    memcpy(csPins, pins, numActiveReaders);
    memcpy(csPinMap, pinMap, numActiveReaders);
    memset(&uids[0], 0, MAX_READERS * MAX_UID_SIZE);
}

void RFIDController::init() {
    for(uint8_t i = 0; i < numActiveReaders; i++) {
        nfcReaders[i] = new Adafruit_PN532(PA4, PA5, PA6, csPins[i]);
        delay(20);  
        nfcReaders[i]->begin();
        delay(20);
        uint32_t versiondata = nfcReaders[i]->getFirmwareVersion();
        Serial2.print(i);
        Serial2.print(" ");
        if (versiondata) {
            Serial2.println(versiondata);
            readerStatus[i] = true; 
            nfcReaders[i]->setPassiveActivationRetries(0xFF);
            nfcReaders[i]->SAMConfig();
            nfcReaders[i]->powerDown();
        } else {    
            Serial2.println("FAIL");
            readerStatus[i] = false;
        }
        delay(20);
    }
    currentReader = 0;
    reading = false;
}

bool RFIDController::getReaderStatus() {
    return true;
}

void RFIDController::getCurrentSetup(bool* tP, uint8_t* u, uint8_t* s) {
    memcpy(tP, tagsPresent, numActiveReaders);
    memcpy(u, uids, numActiveReaders * MAX_UID_SIZE);
    memcpy(s, uidLengths, numActiveReaders);
}

void RFIDController::getReaderErrors(uint8_t* numErrors, uint8_t* errors) {
    uint8_t errorCount = 0;
    for(uint8_t i = 0; i < numActiveReaders; i++) {
        if (!readerStatus[i]) {
            errors[errorCount++] = i;
        }
    }
    *numErrors = errorCount;    
}

void RFIDController::update() {
    if(!reading) {
        reading = true;
        if(checkNFCReader()){
            eventReady = true;
        }
        reading = false;
    }
    //delay(20);
}

void RFIDController::createEvent(uint8_t loc, bool stat) {
    event.setLocation(loc);
    if (stat) {
        event.setEventType(RFIDEventType::TAG_IN);
    } else {
        event.setEventType(RFIDEventType::TAG_OUT);
    }
    event.setUid(uidLengths[loc], &uids[loc][0]);
}

bool RFIDController::checkNFCReader() {
    bool changes = false;
    uint8_t r = csPinMap[currentReader];
    //if(!readerStatus[r]) return false;
    uint8_t uid[7];
    uint8_t uidLength;
    bool success = nfcReaders[r]->readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, READ_BLOCK_TIME);
    if (success != tagsPresent[r]) {
        tagsPresent[r] = success;
        event.setLocation(r);
        if (success) {
            memcpy(&uids[r][0], &uid[0], uidLength);
            uidLengths[r] = uidLength;
        } 
        createEvent(r, success);
        changes = true;
    } 
    nfcReaders[r]->powerDown();
    currentReader = (currentReader + 1) % numActiveReaders;
    return changes;
}

bool RFIDController::hasEvent() {
    return eventReady;
}

RFIDEvent* RFIDController::getEvent() {
    eventReady = false;
    return &event;
}