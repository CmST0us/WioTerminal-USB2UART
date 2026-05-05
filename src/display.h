/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

void display_init(void);
void display_update_status(uint32_t baudrate, uint8_t data_bits, uint8_t stop_bits,
			   uint8_t parity, bool connected);
void display_refresh(void);

#endif /* DISPLAY_H_ */
