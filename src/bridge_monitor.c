/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bridge_monitor.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bridge_monitor, LOG_LEVEL_INF);

static const struct device *cdc_dev;

static struct uart_config_info current_cfg = {
	.baudrate = 115200,
	.data_bits = 8,
	.stop_bits = 1,
	.parity = 0,
	.connected = false,
};

void bridge_monitor_init(const struct device *cdc, const struct device *uart)
{
	ARG_UNUSED(uart);
	cdc_dev = cdc;
}

void bridge_monitor_get_config(struct uart_config_info *cfg)
{
	struct uart_config tmp;
	int ret;

	if (cdc_dev == NULL) {
		*cfg = current_cfg;
		return;
	}

	ret = uart_config_get(cdc_dev, &tmp);
	if (ret == 0) {
		current_cfg.baudrate = tmp.baudrate;
		switch (tmp.data_bits) {
		case UART_CFG_DATA_BITS_5:
			current_cfg.data_bits = 5;
			break;
		case UART_CFG_DATA_BITS_6:
			current_cfg.data_bits = 6;
			break;
		case UART_CFG_DATA_BITS_7:
			current_cfg.data_bits = 7;
			break;
		case UART_CFG_DATA_BITS_8:
			current_cfg.data_bits = 8;
			break;
		case UART_CFG_DATA_BITS_9:
			current_cfg.data_bits = 9;
			break;
		default:
			current_cfg.data_bits = 8;
			break;
		}
		switch (tmp.stop_bits) {
		case UART_CFG_STOP_BITS_0_5:
			current_cfg.stop_bits = 0;
			break;
		case UART_CFG_STOP_BITS_1:
			current_cfg.stop_bits = 1;
			break;
		case UART_CFG_STOP_BITS_1_5:
			current_cfg.stop_bits = 15;
			break;
		case UART_CFG_STOP_BITS_2:
			current_cfg.stop_bits = 2;
			break;
		default:
			current_cfg.stop_bits = 1;
			break;
		}
		switch (tmp.parity) {
		case UART_CFG_PARITY_NONE:
			current_cfg.parity = 0;
			break;
		case UART_CFG_PARITY_ODD:
			current_cfg.parity = 1;
			break;
		case UART_CFG_PARITY_EVEN:
			current_cfg.parity = 2;
			break;
		default:
			current_cfg.parity = 0;
			break;
		}
	}

	*cfg = current_cfg;
}
