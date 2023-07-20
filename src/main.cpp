/// Main
///
/// \file   main.cpp
/// \author Vincent Hamp
/// \date   20/10/2018

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <bt.hpp>
#include <cstdint>
#include <cstring>
#include "config.hpp"
#include "uart.hpp"

/// Application called from ESP-IDF
extern "C" void app_main() {
  // Initialize NVS — it is used to store PHY calibration data
  esp_err_t ret{nvs_flash_init()};
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize BT and UART
  bt_init();
  uart_init();
}
