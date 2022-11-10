#include "MessageManager.h"

StaticJsonDocument<1024> doc;

MessageEventType Message::getType() {
    return type;
}

void Message::getType(char * s) {
    switch(type) {
        case MessageEventType::LIGHT_EFFECT:
            sprintf(s, "LIGHT_EFFECT");
            break;
        case MessageEventType::GET_SETUP:
            sprintf(s, "GET_SETUP");
            break;
        case MessageEventType::TAG_STATE:
            sprintf(s, "TAG_STATE");
            break;
        case MessageEventType::FAIL:
            sprintf(s, "FAIL");
            break;
        case MessageEventType::MESSAGE_RECEIVED:
            sprintf(s, "MESSAGE_RECEIVED");
            break;
        case MessageEventType::ACK:
            sprintf(s, "ACK");
            break;
        case MessageEventType::SERIAL_STATUS:
            sprintf(s, "SERIAL_STATUS");
            break;
        default:
            break;
    }
}   

void Message::setType(MessageEventType t) {
    type = t;
}

int Message::getId(char * i) {
    strcpy(i, id);
    return 0;
}

void Message::setId(char * i) {
    strcpy(id, i);
}

int Message::getParam(char * k, char * p) {
    for(int i = 0; i < paramCount; i++) {
        if(strcmp(k, params[i].key) == 0) {
            strcpy(p, params[i].value);
            return 0;
        }
    }
    return 1;
}

int Message::getParam(int c, char * k, char * p) {
    if (c < paramCount) {
        strcpy(k, params[c].key);
        strcpy(p, params[c].value);
        return 0;
    }
    return 1;
}

int Message::addParam(char * k, char * v) {
    if (paramCount < 16) {
        strcpy(params[paramCount].key, k);
        strcpy(params[paramCount].value, v);
        paramCount++;
        return 0;
    } else {
        return 1;
    }
}

int Message::setParam(char * k, char * p) {
    for(int i = 0; i < paramCount; i++) {
        if (strcmp(params[i].key, k) == 0) {
            strcpy(params[i].value, p);
            return 0;
        }
    }
    return 1;
}

int Message::addTag(int l, char* t){
    strcpy(tags[l], t);
    //Serial.print("IN MESSAGE ");
    //Serial.print(l);
    //Serial.println(tags[l]);
}

void Message::getTag(int l, char * t){
    strcpy(t, tags[l]);
}

int Message::getParamCount() {
    return paramCount;
}

void Message::makeValid() {
    valid = true;
}

bool Message::isValid() {
    return valid;
}

void Message::createTagContainer() {

}

TagSetupMessage::TagSetupMessage() {
    
    addTag(0, "45:AB:CD:32:45:93:23");
    addTag(1, "95:EF:AC:F2:4E:A3:67");
    addTag(2, "40:AB:4D:AC:45:66:87");
    addTag(3, "72:CB:C6:52:43:73:23");
    addTag(4, "15:CC:CD:F2:EF:97:63");
    addTag(5, "56:FE:ED:62:65:83:43");
    addTag(6, "46:AF:CA:CD:EF:89:67");
    addTag(7, "35:3B:4D:36:47:97:13");
    addTag(8, "4B:A4:CA:B2:C5:63:53");
    addTag(9, "A5:CB:C3:67:DC:EF:23");
    
    memset(tags, 0, MAX_TAGS * MAX_TAG_SIZE);
}

void TagSetupMessage::addTag(uint8_t l, char* t){
    strcpy(tags[l], t);
    //Serial.println(t);
    //Serial.println(tags[l]);
}

void TagSetupMessage::printTags() {
    /*
    for(int i = 0; i < 10; i++) {
        Serial.println(tags[i]);
    }
    */
}

void TagSetupMessage::printJson(){
    char tagSetup[1024];
    memset(tagSetup, 0, 1024); 
    sprintf(tagSetup, "{\"event\":\"TAG_SETUP\", \"params\":{\"tags\":[{\"location\": \"0\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"1\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"2\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"3\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"4\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"5\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"6\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"7\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"8\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"9\", \"id\":\"%s\"}]}}", tags[0], tags[1], tags[2], tags[3], tags[4], tags[5], tags[6], tags[7], tags[8], tags[9]);
    Serial.println(tagSetup);
}

void TagSetupMessage::getJson(char * j){
    char tagSetup[1024];
    //memset(tagSetup, 0, 1024);
    sprintf(j, "{\"event\":\"TAG_SETUP\", \"params\":{\"tags\":[{\"location\": \"0\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"1\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"2\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"3\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"4\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"5\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"6\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"7\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"8\", \"id\":\"%s\"},"
                                                                      "{\"location\": \"9\", \"id\":\"%s\"}]}}", tags[0], tags[1], tags[2], tags[3], tags[4], tags[5], tags[6], tags[7], tags[8], tags[9]);
    //Serial.println(tagSetup);
    //strcpy(j, tagSetup);
}

MessageManager::MessageManager() {

}

Message MessageManager::parseMessage(char * in) {

    Message m;
    m.setType(MessageEventType::FAIL);

    //Serial.println(in);
    
    DeserializationError error = deserializeJson(doc, in);  

    if (error) {  
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return m;
    }


    m.makeValid();

    //Serial.println("VALID");

    //m.setId((char*)doc["id"].as<char*>());

    const char * event = doc["event"];
    if (strcmp(event, "LIGHT_EFFECT") == 0) m.setType(MessageEventType::LIGHT_EFFECT);
    if (strcmp(event, "GET_SETUP") == 0) m.setType(MessageEventType::GET_SETUP);
    if (strcmp(event, "TAG_STATE") == 0) m.setType(MessageEventType::TAG_STATE);
    if (strcmp(event, "PING") == 0) m.setType(MessageEventType::PING);
    if (strcmp(event, "ATTRACT") == 0) m.setType(MessageEventType::ATTRACT);
    if (strcmp(event, "RESET") == 0) m.setType(MessageEventType::RESET);
    if (strcmp(event, "ACK") == 0) m.setType(MessageEventType::ACK);
    if (strcmp(event, "SERIAL_STATUS") == 0) m.setType(MessageEventType::SERIAL_STATUS);
    if (strcmp(event, "MONITOR") == 0) m.setType(MessageEventType::MONITOR);
    if (strcmp(event, "TAG_SETUP") == 0) {
        //Serial.println("TAG_SETUP");
        m.setType(MessageEventType::TAG_SETUP);
        JsonObject jo = doc["params"]; 
        for(JsonPair kv : jo) {
            //Serial.println((char *)kv.key().c_str());
            JsonArray ja = kv.value().as<JsonArray>();
            //Serial.println(ja.size());
            //Serial.println("Array");
            for(JsonVariant v : ja) {
                JsonObject jjo = v.as<JsonObject>();    
                int location;
                char id[64];
                for(JsonPair kv: jjo) {
                    /*
                    Serial.print((char *)kv.key().c_str());
                    Serial.print(" ==> ");
                    Serial.println((char*)kv.value().as<char*>());
                    */
                    if (strcmp((char *)kv.key().c_str(), "location") == 0) {
                        //k = "location";
                        //v = (char*)kv.value().as<char*>();
                        //strcpy(location, (char*)kv.value().as<char*>());
                        location = atoi((char*)kv.value().as<char*>());
                    }
                    if (strcmp((char *)kv.key().c_str(), "id") == 0) {  
                        //k = "location";
                        //v = (char*)kv.value().as<char*>();
                        //strcpy(id, "id");
                        strcpy(id, (char*)kv.value().as<char*>());
                    }
                }
                //Serial.print(location);
                //Serial.print(" ==> ");
                //Serial.println(id);
                //strcpy(tags[location], id);
                m.addTag(location, id);
            }
            //m.addParam((char *)kv.key().c_str(), (char*)kv.value().as<char*>());
        }   
        return m;
        //Serial.println((char*)doc["params"]);
        //return m; 
    } 

    JsonObject jo = doc["params"];

    for(JsonPair kv : jo) {
        //Serial.println((char*)kv.value().as<char*>());
        m.addParam((char *)kv.key().c_str(), (char*)kv.value().as<char*>());
    }

    return m;
}

int MessageManager::stringFromMessage(Message m, char * s) {
    char paramString[256];
    int paramLen = 0;
    for (int i = 0; i < m.getParamCount(); i++){
        char k[64];
        char v[64];
        char p[128];
        m.getParam(i, k, v);
        
        if(i > 0) {
            sprintf(p, ",\"%s\":\"%s\"", k, v);
        } else {
            sprintf(p, "\"%s\":\"%s\"", k, v);
        }
        strcpy(&paramString[paramLen], p);
        paramLen += strlen(p);
    }
    char t[64];
    m.getType(t);
    sprintf(s, "{\"event\":\"%s\",\"params\":{%s}}", t, paramString);
    return 0;
}
