/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BRIDGE_MONITOR_H_
#define BRIDGE_MONITOR_H_

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

struct uart_config_info {
	uint32_t baudrate;
	uint8_t data_bits;
	uint8_t stop_bits;
	uint8_t parity;
	bool connected;
};

void bridge_monitor_init(const struct device *cdc_dev, const struct device *uart_dev);
void bridge_monitor_get_config(struct uart_config_info *cfg);

#endif /* BRIDGE_MONITOR_H_ */
