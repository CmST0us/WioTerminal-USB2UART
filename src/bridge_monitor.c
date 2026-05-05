/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bridge_monitor.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bridge_monitor, LOG_LEVEL_INF);

static const struct device *cdc_dev;
static const struct device *uart_dev;

static struct uart_config_info current_cfg = {
	.baudrate = 115200,
	.data_bits = 8,
	.stop_bits = 1,
	.parity = 0,
	.connected = false,
};

void bridge_monitor_init(const struct device *cdc, const struct device *uart)
{
	cdc_dev = cdc;
	uart_dev = uart;
}

uint32_t bridge_monitor_get_tx_count(void)
{
	/* Byte counting not available without modifying uart_bridge driver */
	return 0;
}

uint32_t bridge_monitor_get_rx_count(void)
{
	/* Byte counting not available without modifying uart_bridge driver */
	return 0;
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
		current_cfg.data_bits = tmp.data_bits;
		current_cfg.stop_bits = tmp.stop_bits;
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

void bridge_monitor_reset_counts(void)
{
	/* No-op in v1 */
}
