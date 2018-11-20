/// UART
///
/// \file   uart.cpp
/// \author Vincent Hamp
/// \date   20/10/2018

#include "driver/uart.h"
#include <array>
#include <cstdint>
#include <memory>
#include "config.hpp"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "queue.hpp"
#include "soc/uart_struct.h"

QueueHandle_t uart_queue{nullptr};
static RingbufHandle_t uart_buf{nullptr};
static uart_config_t uart_config{uart_config_default};
static DRAM_ATTR uart_dev_t* const UART[UART_NUM_MAX] = {
    &UART0, &UART1, &UART2};

/// Baud rate detection
///
/// \param  lowpulse  Minimum low-pulse width
/// \param  highpulse Minimum high-pulse width
/// \return Detected baud rate
static int baud_rate_detection(uint32_t lowpulse, uint32_t highpulse) {
  constexpr int supported_baud_rates[]{
      300,     600,     1200,    2400,    4800,    9600,    14400,
      19200,   38400,   57600,   115200,  128000,  153600,  230400,
      256000,  460800,  500000,  921600,  1000000, 1500000, 2000000,
      2500000, 3000000, 3500000, 4000000, 4500000, 5000000};

  uint32_t const pulse{(lowpulse + highpulse) / 2};
  uint32_t const baud_rate{80'000'000 / pulse};

  uint32_t i{};
  for (; i < sizeof(supported_baud_rates) / sizeof(int); ++i)
    if (baud_rate <= supported_baud_rates[i])
      break;

  if (!i)
    return supported_baud_rates[i];
  else {
    if (baud_rate - supported_baud_rates[i - 1] <
        supported_baud_rates[i] - baud_rate)
      return supported_baud_rates[i - 1];
    else
      return supported_baud_rates[i];
  }
}

/// UART receive task
///
/// \param  pvParameter Parameters passed to task
static void uart_rx_task([[maybe_unused]] void* pvParameter) {
  std::unique_ptr<uint8_t[]> rx{new (std::nothrow) uint8_t[uart_chunk_size]};

  for (;;) {
    esp_task_wdt_reset();

    // Read data from UART
    auto len{
        uart_read_bytes(uart_num, &rx[0], uart_chunk_size, pdMS_TO_TICKS(10))};
    if (len <= 0)
      continue;

    // Baud rate detection
    auto const baud_rate{baud_rate_detection(
        UART[uart_num]->lowpulse.min_cnt, UART[uart_num]->highpulse.min_cnt)};
    if (baud_rate != uart_config.baud_rate) {
      uart_config.baud_rate = baud_rate;
      uart_param_config(uart_num, &uart_config);
    }

    // Send data to ring buffer
    while (!xRingbufferSend(uart_buf, &rx[0], len, pdMS_TO_TICKS(10)))
      vTaskDelay(pdMS_TO_TICKS(10));

    // Send ring buffer handle to queue
    while (!xQueueSend(uart_queue, &uart_buf, pdMS_TO_TICKS(10)))
      vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/// UART transmit task
///
/// \param  pvParameter Parameters passed to task
static void uart_tx_task([[maybe_unused]] void* pvParameter) {
  for (;;) {
    esp_task_wdt_reset();

    // Receive ring buffer handle from queue
    RingbufHandle_t uart_buf{nullptr};
    if (!xQueueReceive(bt_queue, &uart_buf, portMAX_DELAY))
      continue;

    // Receive data from ring buffer
    uint8_t* data{nullptr};
    size_t len{};
    while (!(
        data = (uint8_t*)xRingbufferReceive(uart_buf, &len, pdMS_TO_TICKS(10))))
      vTaskDelay(pdMS_TO_TICKS(10));

    // Write data to UART
    while (len) {
      int written_len{uart_write_bytes(uart_num, (const char*)data, len)};
      if (written_len <= 0)
        continue;
      if ((len -= written_len))
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Return item from ring buffer
    vRingbufferReturnItem(uart_buf, (void*)data);
  }
}

/// Configure parameters of an UART driver, communication pins and install the
/// driver.
void uart_init() {
  uart_buf = xRingbufferCreate(uart_buf_size, RINGBUF_TYPE_NOSPLIT);
  if (!uart_buf) {
    ESP_LOGE(uart_tag, "%s can't create ring buffer for UART", __func__);
    return;
  }

  uart_queue = xQueueCreate(8, sizeof(RingbufHandle_t));
  if (!uart_queue) {
    ESP_LOGE(uart_tag, "%s can't create queue for UART", __func__);
    return;
  }

  uart_param_config(uart_num, &uart_config);
  uart_set_pin(uart_num, uart_tx_pin, uart_rx_pin, uart_rts_pin, uart_cts_pin);
  uart_driver_install(uart_num, uart_buf_size, uart_buf_size, 0, NULL, 0);

  // Enable baud rate detection
  UART[0]->auto_baud.en = 1;
}

/// Start UART receive and transmit tasks on application core
void uart_task_start_up() {
  xTaskCreatePinnedToCore(&uart_rx_task,
                          "uart_rx_task",
                          2048,
                          NULL,
                          task_priority_uart_rx,
                          NULL,
                          APP_CPU_NUM);
  xTaskCreatePinnedToCore(&uart_tx_task,
                          "uart_tx_task",
                          2048,
                          NULL,
                          task_priority_uart_tx,
                          NULL,
                          APP_CPU_NUM);
}