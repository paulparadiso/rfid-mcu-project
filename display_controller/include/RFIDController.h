#include <Arduino.h>
#include <Adafruit_PN532.h>

#define READ_BLOCK_TIME 50
#define MAX_READERS 5
#define MAX_UID_SIZE 7

enum class RFIDEventType {
    TAG_IN,
    TAG_OUT
};

class RFIDEvent {

public:
    RFIDEvent();
    RFIDEvent(RFIDEventType type, uint8_t l, uint8_t s, uint8_t* id);
    uint8_t getLocation();
    void setLocation(uint8_t l);
    uint8_t getUidSize();
    void setUidSize(uint8_t s);
    uint8_t* getUid();
    void getUid(uint8_t* s, uint8_t* u);
    void setUid(uint8_t s, uint8_t* u);
    RFIDEventType getEventType();
    void setEventType(RFIDEventType type);

private:
    uint8_t uid[7];
    uint8_t uidSize;
    RFIDEventType eventType;
    uint8_t location;

};

class RFIDController {

public:
    RFIDController();
    RFIDController(uint8_t numReaders, uint8_t* pins, uint8_t* pinMap);
    void init();
    void update();
    bool getReaderStatus();
    void getCurrentSetup(bool* tP, uint8_t* u, uint8_t* s);
    void getReaderErrors(uint8_t* numErrors, uint8_t* errors);
    void createEvent(uint8_t loc, bool stat);
    bool hasEvent();
    RFIDEvent* getEvent();

private:
    Adafruit_PN532* nfcReaders[MAX_READERS];
    bool checkNFCReader();
    uint8_t numActiveReaders;
    uint8_t csPins[MAX_READERS];
    uint8_t csPinMap[MAX_READERS];
    uint8_t uids[MAX_READERS][MAX_UID_SIZE];
    uint8_t uidLengths[MAX_READERS];
    uint8_t currentReader;
    uint8_t nextReader;
    bool reading;
    bool readerStatus[MAX_READERS] = { false };
    bool tagsPresent[MAX_READERS] = { false };
    bool eventReady;
    RFIDEvent event;

};