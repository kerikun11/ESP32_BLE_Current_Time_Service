/**
 * @file main.cpp
 * @author Ryotaro Onuki (kerikun11@gmail.com)
 * @brief ESP32 Current Time Service Server
 * @version 0.1
 * @date 2018-12-11
 *
 * @copyright Copyright (c) 2018 Ryotaro Onuki
 *
 */
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <FreeRTOS.h>
#include <nvs_flash.h>

#include <iomanip>
#include <iostream>
#include <sstream>

// コンパイルエラーを防ぐため， Arduino.h で定義されているマクロをundef
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <chrono>

#define logi                                                                   \
  (std::cout << "[I] " << std::setw(8) << std::setfill(' ')                    \
             << xTaskGetTickCount() * portTICK_PERIOD_MS << " ["               \
             << std::setw(16) << std::setfill(' ') << __FILE__ << ":"          \
             << std::setw(4) << std::setfill(' ') << __LINE__ << "] ")

static const BLEUUID GATT_CTS_UUID((uint16_t)0x1805);
static const BLEUUID GATT_CTS_CTC_UUID((uint16_t)0x2A2B);

extern "C" void app_main() {
  /* NVS flash initialization */
  nvs_flash_init();

  /* BLE Initialization */
  BLEDevice::init("ESP32 CTS Server");

  /* BLE Server */
  BLEServer *pServer = BLEDevice::createServer();
  class MyBLEServerCallbacks : public BLEServerCallbacks {
  public:
    MyBLEServerCallbacks() {}
    virtual void onConnect(BLEServer *pServer) override {
      BLEDevice::getAdvertising()->stop();
    }
    virtual void onDisconnect(BLEServer *pServer) override {
      BLEDevice::getAdvertising()->start();
    }
  };
  pServer->setCallbacks(new MyBLEServerCallbacks());
  /* Current Time Service */
  BLEService *pService = pServer->createService(GATT_CTS_UUID);
  /* Current Time Characteristic */
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      GATT_CTS_CTC_UUID, BLECharacteristic::PROPERTY_READ |
                             BLECharacteristic::PROPERTY_WRITE |
                             BLECharacteristic::PROPERTY_NOTIFY);
  uint8_t data[10] = {0};
  pCharacteristic->setValue(data, 10);
  class MyBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
  public:
    MyBLECharacteristicCallbacks() {}
    virtual void onRead(BLECharacteristic *pCharacteristic) override {
      std::time_t t = std::time(NULL);
      std::tm *now = std::localtime(&t);
      uint8_t data[10] = {0};
      data[0] = (1900 + now->tm_year) & 0xff;
      data[1] = ((1900 + now->tm_year) >> 8) & 0xff;
      data[2] = now->tm_mon + 1;
      data[3] = now->tm_mday;
      data[4] = now->tm_hour;
      data[5] = now->tm_min;
      data[6] = now->tm_sec;
      data[7] = now->tm_wday + 1;
      data[8] = (std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count() %
                 1000) *
                256 / 1000;
      logi << static_cast<int>(data[8]) << std::endl;
      pCharacteristic->setValue(data, sizeof(data));
    }
    virtual void onWrite(BLECharacteristic *pCharacteristic) override {
      logi << pCharacteristic->toString() << std::endl;
    }
  };
  pCharacteristic->setCallbacks(new MyBLECharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  /* BLE Advertising */
  BLEAdvertising *pBLEAdvertising = BLEDevice::getAdvertising();
  pBLEAdvertising->addServiceUUID(GATT_CTS_UUID);
  pBLEAdvertising->setMinInterval(250 / 0.625f);
  pBLEAdvertising->setMaxInterval(250 / 0.625f);
  pBLEAdvertising->start();

  /* Read Char */
  while (1) {
    FreeRTOS::sleep(2222);
  }

  /* sleep forever */
  vTaskDelay(portMAX_DELAY);
}
