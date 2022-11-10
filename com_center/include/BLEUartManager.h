#ifndef BLE_UART_MANAGER_H
#define BLE_UART_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define BLE_CHUNK_SIZE 20

class BLEUartManager : public BLEServerCallbacks, public BLECharacteristicCallbacks {

public:

    BLEUartManager();
    static BLEUartManager* getInstance();
    void init(char * label, void (*f)(char * msg));
    void onWrite(BLECharacteristic* pCharacteristic);
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
    int send(uint8_t * msg, uint16_t msgLen);

private:

    static BLEUartManager* mInstance;
    void (*msgCallback) (char * msg);
    BLEServer* pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;
    uint8_t txValue = 0;    
    char buffer[1024];
    uint16_t bIndex = 0;
    char bStop = '\n';

};

#endif