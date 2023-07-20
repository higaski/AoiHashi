/// Queue
///
/// \file   queue.hpp
/// \author Vincent Hamp
/// \date   20/10/2018

#pragma once

#include <freertos/queue.h>
#include <freertos/ringbuf.h>

/// BT queue
extern QueueHandle_t bt_queue;

/// UART queue
extern QueueHandle_t uart_queue;