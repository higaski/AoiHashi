/// BT SPP
///
/// \file   bt_spp.cpp
/// \author Vincent Hamp
/// \date   20/10/2018

#include <cstdint>
#include <cstring>
#include "bt.hpp"
#include "bt_gap.hpp"
#include "config.hpp"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "queue.hpp"
#include "uart.hpp"

QueueHandle_t bt_queue{nullptr};
static RingbufHandle_t bt_buf{nullptr};

/// Check if BT SPP should take the role of master or slave based on own and
/// remote BT device address.
///
/// \param  own           Own BT device address
/// \param  remote        Remote BT device address
/// \return own < remote  ESP_SPP_ROLE_SLAVE
/// \return own > remote  ESP_SPP_ROLE_MASTER
static esp_spp_role_t spp_master_or_slave(esp_bd_addr_t const& own,
                                          esp_bd_addr_t const& remote) {
  for (auto i{0u}; i < sizeof(esp_bd_addr_t); ++i)
    if (own[i] < remote[i])
      return ESP_SPP_ROLE_SLAVE;
    else if (own[i] > remote[i])
      break;

  return ESP_SPP_ROLE_MASTER;
}

/// Write to BT buffer
///
/// \param  param SPP callback parameters union
static void write_to_bt_buf(esp_spp_cb_param_t* param) {
  // Send data to ring buffer
  while (!xRingbufferSend(
      bt_buf, param->data_ind.data, param->data_ind.len, pdMS_TO_TICKS(10)))
    ;

  // Send ring buffer handle to queue
  while (!xQueueSend(bt_queue, &bt_buf, pdMS_TO_TICKS(10)))
    ;
}

/// BT SPP callback for ESP_SPP_ROLE_MASTER
///
/// \param  event SPP callback function events
/// \param  param SPP callback parameters union
static void bt_spp_master_cb(esp_spp_cb_event_t event,
                             esp_spp_cb_param_t* param) {
  switch (event) {
    // When SPP is inited, the event comes
    case ESP_SPP_INIT_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_INIT_EVT");
      esp_spp_start_discovery(remote_bda);
      break;

    // When SDP discovery complete, the event comes
    case ESP_SPP_DISCOVERY_COMP_EVT:
      ESP_LOGI(bt_spp_master_tag,
               "ESP_SPP_DISCOVERY_COMP_EVT status=%d scn_num=%d",
               param->disc_comp.status,
               param->disc_comp.scn_num);
      // Discovery successful -> connect
      if (param->disc_comp.status == ESP_SPP_SUCCESS)
        esp_spp_connect(ESP_SPP_SEC_AUTHENTICATE,
                        ESP_SPP_ROLE_MASTER,
                        param->disc_comp.scn[0],
                        remote_bda);
      // Else restart discovery
      else
        esp_spp_start_discovery(remote_bda);
      break;

    // When SPP Client connection open, the event comes
    case ESP_SPP_OPEN_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_OPEN_EVT");
      bt_task_start_up(param->open.handle);
      uart_task_start_up();
      break;

    // When SPP connection closed, the event comes
    case ESP_SPP_CLOSE_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_CLOSE_EVT");
      esp_restart();
      break;

    // When SPP server started, the event comes
    case ESP_SPP_START_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_START_EVT");
      break;

    // When SPP client initiated a connection, the event comes
    case ESP_SPP_CL_INIT_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_CL_INIT_EVT");
      break;

    // When SPP connection received data, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_DATA_IND_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_DATA_IND_EVT");
      write_to_bt_buf(param);
      break;

    // When SPP connection congestion status changed, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_CONG_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_CONG_EVT");
      break;

    // When SPP write operation completes, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_WRITE_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_WRITE_EVT");
      break;

    // When SPP Server connection open, the event comes
    case ESP_SPP_SRV_OPEN_EVT:
      ESP_LOGI(bt_spp_master_tag, "ESP_SPP_SRV_OPEN_EVT");
      break;
  }
}

/// BT SPP callback for ESP_SPP_ROLE_SLAVE
///
/// \param  event SPP callback function events
/// \param  param SPP callback parameters union
static void bt_spp_slave_cb(esp_spp_cb_event_t event,
                            esp_spp_cb_param_t* param) {
  switch (event) {
    // When SPP is inited, the event comes
    case ESP_SPP_INIT_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_INIT_EVT");
      esp_spp_start_srv(
          ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, bt_spp_server_name);
      break;

    // When SDP discovery complete, the event comes
    case ESP_SPP_DISCOVERY_COMP_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_DISCOVERY_COMP_EVT");
      break;

    // When SPP Client connection open, the event comes
    case ESP_SPP_OPEN_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_OPEN_EVT");
      break;

    // When SPP connection closed, the event comes
    case ESP_SPP_CLOSE_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_CLOSE_EVT");
      esp_restart();
      break;

    // When SPP server started, the event comes
    case ESP_SPP_START_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_START_EVT");
      break;

    // When SPP client initiated a connection, the event comes
    case ESP_SPP_CL_INIT_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_CL_INIT_EVT");
      break;

    // When SPP connection received data, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_DATA_IND_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_DATA_IND_EVT");
      write_to_bt_buf(param);
      break;

    // When SPP connection congestion status changed, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_CONG_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_CONG_EVT");
      break;

    // When SPP write operation completes, the event comes, only for
    // ESP_SPP_MODE_CB
    case ESP_SPP_WRITE_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_WRITE_EVT");
      break;

    // When SPP Server connection open, the event comes
    case ESP_SPP_SRV_OPEN_EVT:
      ESP_LOGI(bt_spp_slave_tag, "ESP_SPP_SRV_OPEN_EVT");
      bt_task_start_up(param->srv_open.handle);
      uart_task_start_up();
      break;
  }
}

/// Initialize BT SPP
void bt_spp_init() {
  ESP_LOGI(bt_spp_tag, "SPP init");
  bt_buf = xRingbufferCreate(bt_spp_buf_size, RINGBUF_TYPE_NOSPLIT);
  if (!bt_buf) {
    ESP_LOGE(bt_spp_tag, "%s can't create ring buffer for SPP", __func__);
    return;
  }

  bt_queue = xQueueCreate(8, sizeof(RingbufHandle_t));
  if (!bt_queue) {
    ESP_LOGE(bt_spp_tag, "%s can't create queue for SPP", __func__);
    return;
  }

  auto spp_role{spp_master_or_slave(own_bda, remote_bda)};
  ESP_LOGI(bt_spp_tag, "Own device spp role: %d", spp_role);

  esp_err_t ret{esp_spp_register_callback(
      spp_role == ESP_SPP_ROLE_MASTER ? bt_spp_master_cb : bt_spp_slave_cb)};
  if (ret != ESP_OK) {
    ESP_LOGE(bt_spp_tag,
             "%s spp register failed: %s\n",
             __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_spp_init(ESP_SPP_MODE_CB);
  if (ret != ESP_OK) {
    ESP_LOGE(
        bt_spp_tag, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
}