#include <Arduino.h>
#include <ArduinoJson.h>

#define INTERCOM_TAG_SETUP 0xAB
#define INTERCOM_LIGHT_EFFECT 0x01
#define INTERCOM_TAG_STATE 0x02
#define INTERCOM_RESET 0x03
#define INTERCOM_ACK 0x04
#define INTERCOM_MONITOR 0x05

#define LIGHT_EFFECT_ALL_ON 0x00
#define LIGHT_EFFECT_ALL_OFF 0x01
#define LIGHT_EFFECT_PIXEL_ON 0x02
#define LIGHT_EFFECT_PIXEL_OFF 0x03
#define LIGHT_EFFECT_PIXEL_FADE_ON 0x04
#define LIGHT_EFFECT_PIXEL_FADE_OFF 0x05
#define LIGHT_EFFECT_RING_ON 0x06
#define LIGHT_EFFECT_RING_OFF 0x07
#define LIGHT_EFFECT_RING_FADE_ON 0x08
#define LIGHT_EFFECT_RING_FADE_OFF 0x09
#define LIGHT_EFFECT_RING_SWIRL_ON 0x0A
#define LIGHT_EFFECT_RING_SWIRL_OFF 0x0B


enum class MessageType {
    UNDEFINED,
    ERROR_STATUS,
    TAG_SETUP,
    TAG_CHANGE,
    LIGHT_EVENT,
    RESET
};

enum class LightEffectType {
    PIXEL_ON,
    PIXEL_OFF,
    PIXEL_FADE_ON,
    PIXEL_FADE_OFF,
    RING_ON,
    RING_OFF,
    RING_FADE_ON,
    RING_FADE_OFF
};

class IntercomMessage {

public:
    IntercomMessage();
    IntercomMessage(MessageType);
    IntercomMessage(char* m);
    bool fromString(char* m);
    bool toString(char* m, uint16_t sS);
    MessageType getType();
    void addStringParam(char* name, char* val);
    void addByteParam(char* name, uint8_t val);
    void getStringParam(char* name, char* val);
    void getByteParam(char* name, uint8_t* val);
    void addSetupItem(uint8_t l, char * u);

private:

    StaticJsonDocument<2048> json;
    JsonObject params;
    JsonArray tags;

};

typedef struct _LightEffectData {
    uint8_t command;
    uint8_t type;   
    uint16_t location;
    uint32_t startTime;
    uint32_t animationTime;
    uint8_t redLevel;
    uint8_t greenLevel;
    uint8_t blueLevel;
    uint8_t whiteLevel;
} LightEffectData;

class LightEffect {

public:
    LightEffect();
    LightEffect(LightEffectData* ld);
    LightEffect(uint8_t t, uint32_t st, uint32_t aT, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    void makeByteArray(uint8_t* a);
    uint8_t getType();
    void setType(uint8_t t);
    uint16_t getLocation();
    void setLocation(uint16_t l);
    uint32_t getStartTime();
    void setStartTime(uint32_t sT);
    uint32_t getAnimationTime();
    void setAnimationTime(uint32_t aT);
    uint8_t getRedLevel();
    void setRedLevel(uint8_t r);
    uint8_t getGreenLevel();
    void setGreenLevel(uint8_t g);
    uint8_t getBlueLevel();
    void setBlueLevel(uint8_t r);
    uint8_t getWhiteLevel();
    void setWhiteLevel(uint8_t g);

private:
    LightEffectData data;
};