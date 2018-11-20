/// BT
///
/// \file   bt.cpp
/// \author Vincent Hamp
/// \date   20/10/2018

#include <cstdint>
#include <cstring>
#include "bt_gap.hpp"
#include "config.hpp"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "queue.hpp"

// Need to include rfc_int.h for declaration of rfc_cb for dirty workaround
extern "C" {
#include "rfc_int.h"
}

/// BT transmit task
///
/// \param  pvHandle  BT connection handle
static void bt_tx_task(void* pvHandle) {
  uint32_t const handle{(uint32_t const)pvHandle};

  for (;;) {
    esp_task_wdt_reset();

    // Receive ring buffer handle from queue
    RingbufHandle_t uart_buf{nullptr};
    if (!xQueueReceive(uart_queue, &uart_buf, portMAX_DELAY))
      continue;

    // Receive data from ring buffer
    uint8_t* data{nullptr};
    size_t len{};
    while (!(
        data = (uint8_t*)xRingbufferReceive(uart_buf, &len, pdMS_TO_TICKS(10))))
      vTaskDelay(pdMS_TO_TICKS(10));

    // Dirty workaround to keep SPP from starving from ugly flow control bug
    for (auto i{0}; i < MAX_RFC_PORTS; ++i)
      rfc_cb.port.port[i].credit_tx = 10;

    // Write data to SPP
    while (esp_spp_write(handle, len, data) != ESP_OK)
      vTaskDelay(pdMS_TO_TICKS(10));

    // Return item from ring buffer
    vRingbufferReturnItem(uart_buf, (void*)data);
  }
}

/// Start BT transmit task on application core
void bt_task_start_up(uint32_t handle) {
  xTaskCreatePinnedToCore(&bt_tx_task,
                          "bt_tx_task",
                          2048,
                          (void*)handle,
                          task_priority_bt_tx,
                          NULL,
                          APP_CPU_NUM);
}

/// Initialize BT
void bt_init() {
  // Release memory from BLE mode (which we don't need)
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  esp_err_t ret{esp_bt_controller_init(&bt_cfg)};
  if (ret != ESP_OK) {
    ESP_LOGE(bt_gap_tag,
             "%s initialize controller failed: %s\n",
             __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
  if (ret != ESP_OK) {
    ESP_LOGE(bt_gap_tag,
             "%s enable controller failed: %s\n",
             __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bluedroid_init();
  if (ret != ESP_OK) {
    ESP_LOGE(bt_gap_tag,
             "%s initialize bluedroid failed: %s\n",
             __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bluedroid_enable();
  if (ret != ESP_OK) {
    ESP_LOGE(bt_gap_tag,
             "%s enable bluedroid failed: %s\n",
             __func__,
             esp_err_to_name(ret));
    return;
  }

  bt_gap_init();
}