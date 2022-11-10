#include "Arduino.h"
#include "ArduinoJson.h"

#define MAX_TAGS 10
#define MAX_TAG_SIZE 64

enum class MessageEventType {
    LIGHT_EFFECT,
    GET_SETUP,
    MESSAGE_RECEIVED,
    TAG_STATE,
    TAG_SETUP,
    PING,
    ATTRACT,
    RESET,
    ACK,
    SERIAL_STATUS,
    MONITOR,
    FAIL
};

typedef struct _MessageParam {
    char key[64];
    char value[64];
} MessageParam;

class Message {

public:
    Message(){};
    MessageEventType getType();  
    void getType(char* s);
    void setType(MessageEventType t);    
    int getId(char * i);
    void setId(char * i);
    int getParam(char * k, char * p);
    int getParam(int c, char * k, char * p);
    int addParam(char * k, char * v);
    int setParam(char * k, char * p);
    int addTag(int l, char* t);
    void getTag(int l, char* t);
    int getParamCount();
    void makeValid();
    bool isValid();
private:
    MessageEventType type;
    MessageParam params[16];
    void createTagContainer();
    char id[64];
    int paramCount = 0;
    bool valid = false;
    char tags[MAX_TAGS][MAX_TAG_SIZE];
    uint16_t locations[5];
};

class TagSetupMessage {

public:
    TagSetupMessage();
    void addTag(uint8_t l, char* t);
    void printTags();
    void printJson();
    void getJson(char * j);
private:
    char tags[MAX_TAGS][MAX_TAG_SIZE];
};

class MessageManager {

public:
    MessageManager();
    static Message parseMessage(char * in);
    static int stringFromMessage(Message m, char * s);

};