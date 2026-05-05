/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum data_direction {
	DATA_DIR_TX,
	DATA_DIR_RX,
};

void display_init(void);
void display_update_status(uint32_t baudrate, uint8_t data_bits, uint8_t stop_bits,
			   uint8_t parity, bool connected);
void display_update_counts(uint32_t tx_count, uint32_t rx_count);
void display_append_data(enum data_direction dir, const uint8_t *data, size_t len);
void display_refresh(void);

#endif /* DISPLAY_H_ */
