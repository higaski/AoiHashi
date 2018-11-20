/// BT GAP
///
/// \file   bt_gap.hpp
/// \author Vincent Hamp
/// \date   08/10/2018

#pragma once

#include "esp_bt_device.h"

void bt_gap_init();

/// Own BT device address
extern esp_bd_addr_t own_bda;

/// Remote BT device address
extern esp_bd_addr_t remote_bda;