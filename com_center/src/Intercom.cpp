#include "Intercom.h"

IntercomMessage::IntercomMessage() {

}

IntercomMessage::IntercomMessage(MessageType mT) {
    switch(mT) {
        case MessageType::ERROR_STATUS:
            json["event"] = "ERROR_STATUS";
            break;
        case MessageType::TAG_SETUP:
            json["event"] = "TAG_SETUP";
            break;
        case MessageType::TAG_CHANGE:
            json["event"] = "TAG_STATE";
            break;
        case MessageType::LIGHT_EVENT:
            json["event"] = "LIGHT_EVENT";
            break;
        default:
            break;
    }
    params = json.createNestedObject("params");
    if (mT == MessageType::TAG_SETUP) {
        tags = params.createNestedArray("tags");
    }
}
    
IntercomMessage::IntercomMessage(char* m) {
    fromString(m);
}

bool IntercomMessage::fromString(char* m) {
    //Serial2.print("Creating IntercomMessaege from: ");
    //Serial2.println(m);
    DeserializationError error = deserializeJson(json, m);
    if( error ) {
        //Serial2.println("Deserialization fail.");
        return false;
    }
    return true;
}   

bool IntercomMessage::toString(char* m, uint16_t sS) {
    serializeJson(json, m, sS);
    return true;
}

MessageType IntercomMessage::getType() {
    const char* e = json["event"];
    if (strcmp(e, "ERROR_STATUS") == 0) {
        return MessageType::ERROR_STATUS;
    }
    if(strcmp(e, "TAG_SETUP") == 0) {
        return MessageType::TAG_SETUP;
    }
    if(strcmp(e, "TAG_CHANGE") == 0) {
        return MessageType::TAG_CHANGE;
    }
    if(strcmp(e, "LIGHT_EFFECT") == 0) {
        return MessageType::LIGHT_EVENT;
    }
    return MessageType::UNDEFINED;
}

void IntercomMessage::addStringParam(char* name, char* val) {
    params[name] = val;
}

void IntercomMessage::addByteParam(char* name, uint8_t val) {
    params[name] = val;
}

void IntercomMessage::getStringParam(char* name, char* val) {
    strcpy(val, json["params"][name]);
}

void IntercomMessage::getByteParam(char* name, uint8_t* val) {   
    *val = json["params"][name];
}

void IntercomMessage::addSetupItem(uint8_t l, char * u) {
    char loc[16];
    sprintf(loc, "%d", l);
    JsonObject o = tags.createNestedObject();
    o["location"] = loc;
    o["id"] = u;
}

LightEffect::LightEffect() {
    setType(LIGHT_EFFECT_RING_ON);
    setStartTime(0);
    setAnimationTime(0);
    setRedLevel(0);
    setGreenLevel(0);
    setBlueLevel(0);
    setWhiteLevel(0);
}
    
LightEffect::LightEffect(LightEffectData* ld) {
    setType(ld->type);
    setStartTime(ld->startTime);
    setAnimationTime(ld->animationTime);
    setRedLevel(ld->redLevel);
    setGreenLevel(ld->greenLevel);
    setBlueLevel(ld->blueLevel);
    setWhiteLevel(ld->whiteLevel);
}

LightEffect::LightEffect(uint8_t t, uint32_t sT, uint32_t aT, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    data.type = t;
    data.startTime = sT;
    data.animationTime = aT;
    data.redLevel = r;
    data.greenLevel = g;
    data.blueLevel = b;
    data.whiteLevel = w;
}

void LightEffect::makeByteArray(uint8_t* a) {
    memcpy(a, &data, sizeof(LightEffectData));
}

uint8_t LightEffect::getType() {
    return data.type;
}

void LightEffect::setType(uint8_t t) {
    data.type = t;
}

uint16_t LightEffect::getLocation() {
    return data.location;
}

void LightEffect::setLocation(uint16_t l) {
    data.location = l;
}

uint32_t LightEffect::getStartTime() {
    return data.startTime;
}

void LightEffect::setStartTime(uint32_t sT) {
    data.startTime = sT;
}

uint32_t LightEffect::getAnimationTime() {
    return data.animationTime;
}
    
void LightEffect::setAnimationTime(uint32_t aT) {
    data.animationTime = aT;
}

uint8_t LightEffect::getRedLevel() {
    return data.redLevel;
}

void LightEffect::setRedLevel(uint8_t r) {
    data.redLevel = r;
}

uint8_t LightEffect::getGreenLevel() {
    return data.greenLevel;
}

void LightEffect::setGreenLevel(uint8_t g) {
    data.greenLevel = g;
}

uint8_t LightEffect::getBlueLevel() {
    return data.blueLevel;
}

void LightEffect::setBlueLevel(uint8_t b) {
    data.blueLevel = b;
}

uint8_t LightEffect::getWhiteLevel() {
    return data.whiteLevel;
}

void LightEffect::setWhiteLevel(uint8_t w) {
    data.whiteLevel = w;
}