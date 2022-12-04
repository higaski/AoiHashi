/// BT GAP
///
/// \file   bt_gap.cpp
/// \author Vincent Hamp
/// \date   20/10/2018

#include <esp_bt.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_spp_api.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <algorithm>
#include <bt_spp.hpp>
#include <cstdint>
#include <cstring>
#include <limits>
#include "config.hpp"

/// Own BT device address
esp_bd_addr_t own_bda{};

/// Remote BT device address
esp_bd_addr_t remote_bda{};

// random only works when RF subsystem is enabled

/// Get random value between min and max
///
/// \param  min Minimum value
/// \param  Max Maximum value
/// \return Random value between min and max
static uint32_t random_interval(uint32_t min, uint32_t max) {
  return min + (static_cast<float>(max - min) * esp_random()) /
                 std::numeric_limits<uint32_t>::max();
}

/// Check if BT device address is valid
///
/// \param  bda   BT device address
/// \return true  BT device address valid
/// \return false BT device address invalid
static bool is_valid_bda(esp_bd_addr_t const& bda) {
  for (auto i{0u}; i < ESP_BD_ADDR_LEN; ++i)
    if (bda[i] != 0) return true;
  return false;
}

/// Convert BT device address to string
///
/// \param  bda   BT device address
/// \param  str   Buffer to write into
/// \param  size  Maximum size of string
/// \return Pointer to buffer which contains string
static char* bda2str(esp_bd_addr_t bda, char* str, size_t size) {
  if (bda == NULL || str == NULL || size < 18) return NULL;

  uint8_t* p{bda};
  sprintf(
    str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);

  return str;
}

/// Get BT device name from extended inquiry response
///
/// \param  eir         Pointer to extended inquiry response
/// \param  bdname      Pointer to buffer for BT device name
/// \param  bdname_len  Pointer to length of BT device name
/// \return true        Success
/// \return false       Failure
static bool
get_name_from_eir(uint8_t* eir, uint8_t* bdname, uint8_t* bdname_len) {
  uint8_t rmt_bdname_len{0};

  if (!eir) return false;

  uint8_t* rmt_bdname{esp_bt_gap_resolve_eir_data(
    eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len)};
  if (!rmt_bdname)
    rmt_bdname = esp_bt_gap_resolve_eir_data(
      eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);

  if (rmt_bdname) {
    if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
      rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;

    if (bdname) {
      memcpy(bdname, rmt_bdname, rmt_bdname_len);
      bdname[rmt_bdname_len] = '\0';
    }
    if (bdname_len) *bdname_len = rmt_bdname_len;

    return true;
  }

  return false;
}

/// Check if discovery event found remote ESP32
///
/// \param  param A2DP state callback parameters
/// \return true  Discovery event found remote ESP32
/// \return false Discovery event found something else
static bool is_remote_esp_device(esp_bt_gap_cb_param_t* param) {
  // TODO Not sure if this doesn't blow up the stack? We have 3584 bytes which
  // is a lot... but still?
  uint8_t bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
  uint8_t bdname_len{0};
  char bda_str[18];
  esp_bt_gap_dev_prop_t* p;  // Bluetooth Device Property Descriptor

  ESP_LOGI(
    bt_gap_tag, "Device found: %s", bda2str(param->disc_res.bda, bda_str, 18));

  for (int i{0u}; i < param->disc_res.num_prop; i++) {
    p = param->disc_res.prop + i;
    switch (p->type) {
      // Class of Device, value type is uint32_t
      case ESP_BT_GAP_DEV_PROP_COD: {
        ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DEV_PROP_COD");
        uint32_t cod{*(uint32_t*)(p->val)};
        ESP_LOGI(bt_gap_tag, "--Class of Device: 0x%x", cod);
        break;
      }

      // Received Signal strength Indication, value type is int8_t, ranging from
      // -128 to 127
      case ESP_BT_GAP_DEV_PROP_RSSI: {
        ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DEV_PROP_RSSI");
        int32_t rssi{*(int8_t*)(p->val)};
        ESP_LOGI(bt_gap_tag, "--RSSI: %d", rssi);
        break;
      }

      // Bluetooth device name, value type is int8_t []
      case ESP_BT_GAP_DEV_PROP_BDNAME: {
        ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DEV_PROP_BDNAME");
        bdname_len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN)
                       ? ESP_BT_GAP_MAX_BDNAME_LEN
                       : (uint8_t)p->len;
        memcpy(bdname, (uint8_t*)(p->val), bdname_len);
        bdname[bdname_len] = '\0';
        ESP_LOGI(bt_gap_tag, "Device name: %s", bdname);
        break;
      }

      // Extended Inquiry Response
      case ESP_BT_GAP_DEV_PROP_EIR: {
        uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
        ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DEV_PROP_EIR");
        memcpy(eir, (uint8_t*)(p->val), p->len);
        get_name_from_eir(eir, bdname, &bdname_len);
        if (bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
          bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        bdname[bdname_len] = '\0';
        ESP_LOGI(bt_gap_tag, "Device name: %s", bdname);
        break;
      }
    }
  }

  return bdname_len && !strcmp((char*)bdname, bt_dev_name);
}

/// BT GAP callback
///
/// \param  event BT GAP callback events
/// \param  param A2DP state callback parameters
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event,
                          esp_bt_gap_cb_param_t* param) {
  switch (event) {
    // device discovery result event
    case ESP_BT_GAP_DISC_RES_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DISC_RES_EVT");
      // Found remote esp
      if (is_remote_esp_device(param)) {
        memcpy(remote_bda, param->disc_res.bda, sizeof(esp_bd_addr_t));
        esp_bt_gap_cancel_discovery();
        bt_spp_init();
      }
      break;

    // discovery state changed event
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
      // Restart discovery if we haven't found remote esp
      if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED &&
          !is_valid_bda(remote_bda))
        esp_bt_gap_start_discovery(
          ESP_BT_INQ_MODE_GENERAL_INQUIRY,
          random_interval(inquiry_duration_min, inquiry_duration_max),
          0);
      break;

    // get remote services event
    case ESP_BT_GAP_RMT_SRVCS_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_RMT_SRVCS_EVT");
      break;

    // get remote service record event
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_RMT_SRVC_REC_EVT");
      break;

    // AUTH complete event
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_AUTH_CMPL_EVT");
      if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
        ESP_LOGI(bt_gap_tag,
                 "authentication success: %s",
                 param->auth_cmpl.device_name);
        esp_log_buffer_hex(bt_gap_tag, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
      } else {
        ESP_LOGE(bt_gap_tag,
                 "authentication failed, status:%d",
                 param->auth_cmpl.stat);
      }
      break;
    }

    // Legacy Pairing Pin code request
    case ESP_BT_GAP_PIN_REQ_EVT: {
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_PIN_REQ_EVT");
      ESP_LOGI(bt_gap_tag,
               "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d",
               param->pin_req.min_16_digit);
      if (param->pin_req.min_16_digit) {
        ESP_LOGI(bt_gap_tag, "Input pin code: 0000 0000 0000 0000");
        esp_bt_pin_code_t pin_code{0};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
      } else {
        ESP_LOGI(bt_gap_tag, "Input pin code: 1234");
        esp_bt_pin_code_t pin_code;
        pin_code[0] = '1';
        pin_code[1] = '2';
        pin_code[2] = '3';
        pin_code[3] = '4';
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
      }
      break;
    }

    // Simple Pairing User Confirmation request
    case ESP_BT_GAP_CFM_REQ_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_CFM_REQ_EVT");
      esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
      break;

    // Simple Pairing Passkey Notification
    case ESP_BT_GAP_KEY_NOTIF_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_KEY_NOTIF_EVT");
      break;

    // Simple Pairing Passkey request
    case ESP_BT_GAP_KEY_REQ_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_KEY_REQ_EVT");
      break;

    // read rssi event
    case ESP_BT_GAP_READ_RSSI_DELTA_EVT:
      ESP_LOGI(bt_gap_tag, "ESP_BT_GAP_READ_RSSI_DELTA_EVT");
      break;

    //
    case ESP_BT_GAP_EVT_MAX: break;

    default: break;
  }
}

/// Initialize BT GAP
void bt_gap_init() {
  // Set default parameters for Secure Simple Pairing
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

  // Set default parameters for Legacy Pairing
  // Use variable pin, input pin code when pairing
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
  esp_bt_pin_code_t pin_code;
  esp_bt_gap_set_pin(pin_type, 0, pin_code);

  esp_bt_dev_set_device_name(bt_dev_name);

  // Get own BT device address
  uint8_t const* adr{esp_bt_dev_get_address()};
  if (!adr) {
    ESP_LOGE(bt_gap_tag, "%s can't retrieve own address\n", __func__);
    return;
  }
  memcpy(own_bda, adr, sizeof(esp_bd_addr_t));
  char bda_str[18];
  ESP_LOGI(bt_gap_tag, "Own address: %s", bda2str(own_bda, bda_str, 18));

  // Set discoverable and connectable mode, wait to be connected
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

  // Register GAP callback function
  esp_bt_gap_register_callback(bt_app_gap_cb);

  // Start to discover nearby Bluetooth devices
  esp_bt_gap_start_discovery(
    ESP_BT_INQ_MODE_GENERAL_INQUIRY,
    random_interval(inquiry_duration_min, inquiry_duration_max),
    0);
}