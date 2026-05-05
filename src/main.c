/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart/uart_bridge.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <sample_usbd.h>

#include "bridge_monitor.h"
#include "display.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb2uart, LOG_LEVEL_INF);

const struct device *const cdc_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));

static struct usbd_context *sample_usbd;
static bool usb_connected;

#define DEVICE_DT_GET_COMMA(node_id) DEVICE_DT_GET(node_id),

const struct device *uart_bridges[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_uart_bridge, DEVICE_DT_GET_COMMA)
};

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			usbd_enable(ctx);
		}
		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			usbd_disable(ctx);
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_LINE_CODING ||
	    msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		for (uint8_t i = 0; i < ARRAY_SIZE(uart_bridges); i++) {
			uart_bridge_settings_update(msg->dev, uart_bridges[i]);
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		usb_connected = true;
	}
}

int main(void)
{
	int err;

	sample_usbd = sample_usbd_init_device(sample_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		err = usbd_enable(sample_usbd);
		if (err) {
			LOG_ERR("Failed to enable device support");
			return err;
		}
	}

	bridge_monitor_init(cdc_dev, NULL);

	display_init();

	LOG_INF("USB2UART bridge ready");

	while (1) {
		struct uart_config_info cfg;

		bridge_monitor_get_config(&cfg);
		cfg.connected = usb_connected;
		display_update_status(cfg.baudrate, cfg.data_bits,
				     cfg.stop_bits, cfg.parity, cfg.connected);
		display_refresh();
		k_sleep(K_MSEC(CONFIG_USB2UART_DISPLAY_REFRESH_MS));
	}

	return 0;
}
