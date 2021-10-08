/// Config
//
/// \file   config.hpp
/// \author Vincent Hamp
/// \date   08/10/2018

#pragma once

#include <driver/gpio.h>
#include <driver/uart.h>

/// BT device name
constexpr auto bt_dev_name{"ESP32_BT_UART_BRIDGE"};

/// BT SPP server name
constexpr auto bt_spp_server_name{"ESP32_BT_UART_SPP_SERVER"};

/// Tags for logging
constexpr auto bt_tag{"BT"};
constexpr auto bt_gap_tag{"BT_GAP"};
constexpr auto bt_spp_tag{"BT_SPP"};
constexpr auto bt_spp_master_tag{"BT_SPP_MASTER"};
constexpr auto bt_spp_slave_tag{"BT_SPP_SLAVE"};
constexpr auto uart_tag{"UART"};

/// Min and max inquiry duration (for BT discovery a random duration between min
/// and max is picked)
constexpr auto inquiry_duration_min{1};
constexpr auto inquiry_duration_max{5};

/// SPP chunk size
constexpr auto bt_spp_chunk_size{1024};

/// SPP ring buffer length
constexpr auto bt_spp_buf_len{4};

/// SPP ring buffer size
constexpr auto bt_spp_buf_size{bt_spp_chunk_size * bt_spp_buf_len};

/// UART chunk size
constexpr auto uart_chunk_size{1024};

/// UART ring buffer length
constexpr auto uart_buf_len{4};

/// UART ring buffer size
constexpr auto uart_buf_size{uart_chunk_size * uart_buf_len};

/// BT transmit task priority
constexpr UBaseType_t task_priority_bt_tx{4};

/// UART receive task priority
constexpr UBaseType_t task_priority_uart_rx{6};

/// UART transmit task priority
constexpr UBaseType_t task_priority_uart_tx{5};

/// UART peripheral number
constexpr auto uart_num{UART_NUM_0};

/// UART transmit pin number
constexpr auto uart_tx_pin{GPIO_NUM_1};

/// UART receive pin number
constexpr auto uart_rx_pin{GPIO_NUM_3};

/// UART request-to-send pin number
constexpr auto uart_rts_pin{UART_PIN_NO_CHANGE};

/// UART clear-to-send pin number
constexpr auto uart_cts_pin{UART_PIN_NO_CHANGE};

/// UART configuration parameters
constexpr uart_config_t uart_config_default{.baud_rate = 921600,
                                            .data_bits = UART_DATA_8_BITS,
                                            .parity = UART_PARITY_DISABLE,
                                            .stop_bits = UART_STOP_BITS_1,
                                            .flow_ctrl =
                                              UART_HW_FLOWCTRL_DISABLE,
                                            .rx_flow_ctrl_thresh = 0,
                                            .use_ref_tick = false};