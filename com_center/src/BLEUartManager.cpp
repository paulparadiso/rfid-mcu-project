#include "BLEUartManager.h"

BLEUartManager* BLEUartManager::mInstance = NULL;

BLEUartManager::BLEUartManager() {

}

BLEUartManager* BLEUartManager::getInstance() {
    if (!mInstance) {
        mInstance = new BLEUartManager();
    }
    return mInstance;
}

void BLEUartManager::init(char* label, void (*f)(char * msg)) {
    BLEDevice::init(label);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setCallbacks(this);
    pService->start();
    pServer->getAdvertising()->addServiceUUID(BLEUUID(SERVICE_UUID));
    pServer->getAdvertising()->start();
    msgCallback = f;
}

void BLEUartManager::onConnect(BLEServer* pServer) {
    deviceConnected = true;
}

void BLEUartManager::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
}

void BLEUartManager::onWrite(BLECharacteristic* pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        for(int i = 0; i < rxValue.length(); i++) {
            if (rxValue[i] == bStop) {
                buffer[bIndex] = 0;
                (*msgCallback)((char*)buffer);
                bIndex = 0;
                memset(buffer, 0, 1024);
            } else {
                buffer[bIndex++] = rxValue[i];
            }
        }
    }
}   

int BLEUartManager::send(uint8_t * msg, uint16_t msgLen) {
    Serial.print("BLE send. ");
    Serial.println(msgLen);
    //Serial.println(msg);
    if(msgLen < BLE_CHUNK_SIZE) {
        pTxCharacteristic->setValue(&msg[0], msgLen);
        pTxCharacteristic->notify();
        return 0;
    }
    uint32_t currentPosition = 0;
    while((msgLen - currentPosition) < BLE_CHUNK_SIZE) {
        pTxCharacteristic->setValue(&msg[currentPosition], BLE_CHUNK_SIZE);
        pTxCharacteristic->notify();
        currentPosition += BLE_CHUNK_SIZE;
        Serial.println(currentPosition);
        //delay(10);
    }
    pTxCharacteristic->setValue(&msg[currentPosition], msgLen - currentPosition);
    pTxCharacteristic->notify();
    return 0;
}
