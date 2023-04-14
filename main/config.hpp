/// Config
//
/// \file   config.hpp
/// \author Vincent Hamp
/// \date   08/10/2018

#pragma once

#include <driver/gpio.h>
#include <driver/uart.h>

/// BT device name
inline constexpr auto bt_dev_name{"ESP32_BT_UART_BRIDGE"};

/// BT SPP server name
inline constexpr auto bt_spp_server_name{"ESP32_BT_UART_SPP_SERVER"};

/// Tags for logging
inline constexpr auto bt_tag{"BT"};
inline constexpr auto bt_gap_tag{"BT_GAP"};
inline constexpr auto bt_spp_tag{"BT_SPP"};
inline constexpr auto bt_spp_master_tag{"BT_SPP_MASTER"};
inline constexpr auto bt_spp_slave_tag{"BT_SPP_SLAVE"};
inline constexpr auto uart_tag{"UART"};

/// Min and max inquiry duration (for BT discovery a random duration between min
/// and max is picked)
inline constexpr auto inquiry_duration_min{1};
inline constexpr auto inquiry_duration_max{5};

/// SPP chunk size
inline constexpr auto bt_spp_chunk_size{1024};

/// SPP ring buffer length
inline constexpr auto bt_spp_buf_len{4};

/// SPP ring buffer size
inline constexpr auto bt_spp_buf_size{bt_spp_chunk_size * bt_spp_buf_len};

/// UART chunk size
inline constexpr auto uart_chunk_size{1024};

/// UART ring buffer length
inline constexpr auto uart_buf_len{4};

/// UART ring buffer size
inline constexpr auto uart_buf_size{uart_chunk_size * uart_buf_len};

/// BT transmit task priority
inline constexpr UBaseType_t task_priority_bt_tx{4};

/// UART receive task priority
inline constexpr UBaseType_t task_priority_uart_rx{6};

/// UART transmit task priority
inline constexpr UBaseType_t task_priority_uart_tx{5};

/// UART peripheral number
inline constexpr auto uart_num{UART_NUM_0};

/// UART transmit pin number
inline constexpr auto uart_tx_pin{GPIO_NUM_1};

/// UART receive pin number
inline constexpr auto uart_rx_pin{GPIO_NUM_3};

/// UART request-to-send pin number
inline constexpr auto uart_rts_pin{UART_PIN_NO_CHANGE};

/// UART clear-to-send pin number
inline constexpr auto uart_cts_pin{UART_PIN_NO_CHANGE};

/// UART configuration parameters
inline constexpr uart_config_t uart_config_default{
  .baud_rate = 921600,
  .data_bits = UART_DATA_8_BITS,
  .parity = UART_PARITY_DISABLE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  .rx_flow_ctrl_thresh = 0,
  .source_clk = UART_SCLK_DEFAULT};

/// Enable/disable UART baud rate detection
inline constexpr auto uart_auto_baud_rate{false};