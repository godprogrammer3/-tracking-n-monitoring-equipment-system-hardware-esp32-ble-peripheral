#include<Arduino.h>
#include <NimBLEDevice.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#include "driver/gpio.h"
#include "driver/can.h"

void canBusSendMessage(uint8_t data[8],uint8_t length){
  can_message_t message;
  
  message.identifier = 0;
  message.flags = CAN_MSG_FLAG_NONE;
  message.data_length_code = length; 
  for (int i = 0 ; i < length ; i++) {
     message.data[i] = data[i];
  }
  if (can_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("Message queued for transmission");
  } else {
    Serial.println("Failed to queue message for transmission");
  }
}

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
   void onRead(NimBLECharacteristic* pCharacteristic){
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onRead(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.print(pCharacteristic->getUUID().toString().c_str());
        Serial.print(": onWrite(), value: ");
        Serial.println(pCharacteristic->getValue().c_str());
         if(strcmp(pCharacteristic->getValue().c_str(),"open") == 0){
          uint8_t data[] = {1};
          canBusSendMessage(data,1);
        }else if(strcmp(pCharacteristic->getValue().c_str(),"close") == 0){
          uint8_t data[] = {0};
          canBusSendMessage(data,1);
        }
    };
    void onNotify(NimBLECharacteristic* pCharacteristic) {
        Serial.println("Sending notification to clients");
    };
};


void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  BLEDevice::init("toollo-locker");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID,
                                        NIMBLE_PROPERTY::READ |
                                        NIMBLE_PROPERTY::WRITE |
                                        NIMBLE_PROPERTY::NOTIFY
                                       );

  pCharacteristic->setValue("closed");
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  can_general_config_t g_config = {// สร้างต้วแปร g_config ใช้กำหนดค่าเกี่ยวกับบัส CAN
                                   .mode = CAN_MODE_NORMAL,
                                   .tx_io = GPIO_NUM_26, // กำหนดขา TX ต่อกับ 26
                                   .rx_io = GPIO_NUM_27, // กำหนดขา TX ต่อกับ 27
                                   .clkout_io = (gpio_num_t)CAN_IO_UNUSED,
                                   .bus_off_io = (gpio_num_t)CAN_IO_UNUSED,
                                   .tx_queue_len = 65,
                                   .rx_queue_len = 65,
                                   .alerts_enabled = CAN_ALERT_ALL,
                                   .clkout_divider = 0};
  can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
  if (can_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
  {
    Serial.println("Failed to install driver");
    return;
  }
  if (can_start() != ESP_OK)
  {
    Serial.println("Failed to start driver");
    return;
  }

  Serial.println("Initialed finished");
}

void loop() {
  delay(2000);
}