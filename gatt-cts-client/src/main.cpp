/**
 * @file main.cpp
 * @author Ryotaro Onuki (kerikun11@gmail.com)
 * @brief ESP32 Current Time Service Client
 * @version 0.1
 * @date 2018-12-11
 *
 * @copyright Copyright (c) 2018 Ryotaro Onuki
 *
 */
#include <BLEClient.h>
#include <BLEDevice.h>
#include <FreeRTOS.h>
#include <nvs_flash.h>

#include <functional>
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

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
  MyAdvertisedDeviceCallbacks(
      BLEAdvertisedDevice *pAdvertisedDevice,
      std::function<bool(BLEAdvertisedDevice *pAdvertisedDevice)>
          isTargetDevice)
      : pAdvertisedDevice(pAdvertisedDevice), isTargetDevice(isTargetDevice) {}
  ~MyAdvertisedDeviceCallbacks() {}
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    logi << "MyAdvertisedDeviceCallbacks.onResult(): "
         << advertisedDevice.toString() << std::endl;
    if (isTargetDevice(&advertisedDevice)) {
      BLEDevice::getScan()->stop();
      logi << "device found" << std::endl;
      *pAdvertisedDevice = advertisedDevice;
    }
  }

private:
  BLEAdvertisedDevice *pAdvertisedDevice;
  std::function<bool(BLEAdvertisedDevice *pAdvertisedDevice)> isTargetDevice;
};

extern "C" void app_main() {
  /* NVS flash initialization */
  nvs_flash_init();

  /* BLE Initialization */
  BLEDevice::init("ESP32 CTS Client");

  /* BLE Advertising */
  BLEAdvertising *pBLEAdvertising = BLEDevice::getAdvertising();
  pBLEAdvertising->setMinInterval(250 / 0.625f);
  pBLEAdvertising->setMaxInterval(250 / 0.625f);
  pBLEAdvertising->start();

  /* BLE Scan */
  BLEScan *pScan = BLEDevice::getScan();
  /* set scan callback */
  BLEAdvertisedDevice advertisedDevice;
  MyAdvertisedDeviceCallbacks advertisedDeviceCallbacks(
      &advertisedDevice, [](BLEAdvertisedDevice *pAdvertisedDevice) {
        return pAdvertisedDevice->isAdvertisingService(GATT_CTS_UUID);
      });
  pScan->setAdvertisedDeviceCallbacks(&advertisedDeviceCallbacks);
  /* conduct scan */
  BLEScanResults scanResults =
      pScan->start(60); //< blocks until the target device is found
  scanResults.dump();
  pScan->setAdvertisedDeviceCallbacks(nullptr);

  /* BLE Client */
  BLEClient *pClient = BLEDevice::createClient();
  pClient->connect(&advertisedDevice);
  /* BLE Remote Service */
  BLERemoteService *pBLERemoteService = pClient->getService(GATT_CTS_UUID);
  std::cout << pBLERemoteService->toString() << std::endl;
  /* BLE Remote Characteristic */
  BLERemoteCharacteristic *pBLERemoteCharacteristic =
      pBLERemoteService->getCharacteristic(GATT_CTS_CTC_UUID);
  std::cout << pBLERemoteCharacteristic->toString() << std::endl;
  /* print info */
  logi << pClient->toString() << std::endl;

  /* Read Char */
  while (1) {
    FreeRTOS::sleep(2222);
    pBLERemoteCharacteristic->readUInt8();
    auto data = pBLERemoteCharacteristic->readRawData();
    int year = data[0] | (data[1] << 8);
    int month = data[2];
    int day = data[3];
    int hour = data[4];
    int minute = data[5];
    int second = data[6];
    // int weekday = data[7];
    int frac256 = data[8];
    std::stringstream stst;
    stst << std::setw(4) << std::setfill('0') << year;
    stst << "/" << std::setw(2) << std::setfill('0') << month;
    stst << "/" << std::setw(2) << std::setfill('0') << day;
    stst << "-" << std::setw(2) << std::setfill('0') << hour;
    stst << ":" << std::setw(2) << std::setfill('0') << minute;
    stst << "-" << std::setw(2) << std::setfill('0') << second;
    stst << "." << std::setw(3) << std::setfill('0') << frac256;
    logi << stst.str() << std::endl;
  }

  /* sleep forever */
  vTaskDelay(portMAX_DELAY);
}
