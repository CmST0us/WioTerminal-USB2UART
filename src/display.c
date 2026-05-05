/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#define SCREEN_W 320
#define SCREEN_H 240

/* Layout constants */
#define STATUS_BAR_H  20
#define COUNTER_BAR_H 18
#define DATA_AREA_H   100

/* Y positions */
#define STATUS_BAR_Y   0
#define COUNTER_BAR_Y  (STATUS_BAR_H)
#define TX_AREA_Y      (STATUS_BAR_H + COUNTER_BAR_H)
#define RX_AREA_Y      (TX_AREA_Y + DATA_AREA_H)

/* Max lines per direction (reduced to save RAM) */
#define MAX_LINES 128
#define LINE_BUF_LEN 48

/* Parity string lookup */
static const char *parity_str(uint8_t p)
{
	switch (p) {
	case 1: return "Odd";
	case 2: return "Even";
	default: return "None";
	}
}

/* LVGL objects */
static lv_obj_t *status_label;
static lv_obj_t *counter_label;
static lv_obj_t *tx_header_label;
static lv_obj_t *tx_data_label;
static lv_obj_t *rx_header_label;
static lv_obj_t *rx_data_label;

/* Text buffers for scrolling data areas */
static char tx_lines[MAX_LINES][LINE_BUF_LEN];
static char rx_lines[MAX_LINES][LINE_BUF_LEN];
static uint16_t tx_line_count;
static uint16_t rx_line_count;

/* Internal: append a formatted line to a data area */
static void append_line(char lines[][LINE_BUF_LEN], uint16_t *count,
			lv_obj_t *data_label, const char *text)
{
	if (*count >= MAX_LINES) {
		/* Scroll: shift everything up by one line */
		memmove(lines[0], lines[1], (MAX_LINES - 1) * LINE_BUF_LEN);
		*count = MAX_LINES - 1;
	}
	strncpy(lines[*count], text, LINE_BUF_LEN - 1);
	lines[*count][LINE_BUF_LEN - 1] = '\0';
	(*count)++;

	/* Rebuild label text from all lines */
	lv_label_set_text(data_label, "");
	for (uint16_t i = 0; i < *count; i++) {
		lv_label_ins_text(data_label, LV_LABEL_POS_LAST, lines[i]);
		if (i < *count - 1) {
			lv_label_ins_text(data_label, LV_LABEL_POS_LAST, "\n");
		}
	}
}

void display_init(void)
{
	const struct device *display_dev;
	lv_obj_t *scr;
	lv_obj_t *obj;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return;
	}

	display_blanking_off(display_dev);

	scr = lv_screen_active();

	/* Set screen background to black */
	lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

	/* Status bar (y=0, h=20) */
	obj = lv_obj_create(scr);
	lv_obj_set_size(obj, SCREEN_W, STATUS_BAR_H);
	lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_Y);
	lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_left(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_right(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

	status_label = lv_label_create(obj);
	lv_label_set_text(status_label, "Baud:--- D:- S:- P:---");
	lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(status_label, lv_color_white(), LV_PART_MAIN);
	lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 0, 0);

	/* Counter bar (y=20, h=18) */
	obj = lv_obj_create(scr);
	lv_obj_set_size(obj, SCREEN_W, COUNTER_BAR_H);
	lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, COUNTER_BAR_Y);
	lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_left(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_right(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

	counter_label = lv_label_create(obj);
	lv_label_set_text(counter_label, "TX: 0  RX: 0");
	lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(counter_label, lv_color_white(), LV_PART_MAIN);
	lv_obj_align(counter_label, LV_ALIGN_LEFT_MID, 0, 0);

	/* TX data area (y=38, h=100) */
	obj = lv_obj_create(scr);
	lv_obj_set_size(obj, SCREEN_W, DATA_AREA_H);
	lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, TX_AREA_Y);
	lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_left(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_right(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

	tx_header_label = lv_label_create(obj);
	lv_label_set_text(tx_header_label, "TX >>");
	lv_obj_set_style_text_font(tx_header_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(tx_header_label, lv_color_make(0x00, 0xFF, 0x00),
				     LV_PART_MAIN);
	lv_obj_align(tx_header_label, LV_ALIGN_TOP_LEFT, 0, 0);

	tx_data_label = lv_label_create(obj);
	lv_label_set_text(tx_data_label, "");
	lv_obj_set_style_text_font(tx_data_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(tx_data_label, lv_color_white(), LV_PART_MAIN);
	lv_obj_align(tx_data_label, LV_ALIGN_TOP_LEFT, 0, 14);
	lv_obj_set_width(tx_data_label, SCREEN_W - 4);

	/* RX data area (y=138, h=100) */
	obj = lv_obj_create(scr);
	lv_obj_set_size(obj, SCREEN_W, DATA_AREA_H);
	lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, RX_AREA_Y);
	lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_top(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_left(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_pad_right(obj, 1, LV_PART_MAIN);
	lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

	rx_header_label = lv_label_create(obj);
	lv_label_set_text(rx_header_label, "RX >>");
	lv_obj_set_style_text_font(rx_header_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(rx_header_label, lv_color_make(0x00, 0x80, 0xFF),
				     LV_PART_MAIN);
	lv_obj_align(rx_header_label, LV_ALIGN_TOP_LEFT, 0, 0);

	rx_data_label = lv_label_create(obj);
	lv_label_set_text(rx_data_label, "");
	lv_obj_set_style_text_font(rx_data_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(rx_data_label, lv_color_white(), LV_PART_MAIN);
	lv_obj_align(rx_data_label, LV_ALIGN_TOP_LEFT, 0, 14);
	lv_obj_set_width(rx_data_label, SCREEN_W - 4);
}

void display_update_status(uint32_t baudrate, uint8_t data_bits, uint8_t stop_bits,
			   uint8_t parity, bool connected)
{
	char buf[48];

	snprintf(buf, sizeof(buf), "Baud:%u D:%u S:%u P:%s %s",
		 baudrate, data_bits, stop_bits, parity_str(parity),
		 connected ? "\xE2\x97\x8F" : "\xE2\x97\x8B");
	lv_label_set_text(status_label, buf);

	if (connected) {
		lv_obj_set_style_text_color(status_label,
					    lv_color_make(0x00, 0xFF, 0x00),
					    LV_PART_MAIN);
	} else {
		lv_obj_set_style_text_color(status_label,
					    lv_color_white(),
					    LV_PART_MAIN);
	}
}

void display_update_counts(uint32_t tx_count, uint32_t rx_count)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "TX: %u  RX: %u", tx_count, rx_count);
	lv_label_set_text(counter_label, buf);
}

void display_append_data(enum data_direction dir, const uint8_t *data, size_t len)
{
	char line[LINE_BUF_LEN];
	char hex_part[3 * 8 + 1]; /* "XX " * 8 + NUL */
	char ascii_part[8 + 1];
	size_t offset = 0;

	while (offset < len) {
		size_t chunk = len - offset;
		uint16_t hex_pos = 0;
		uint16_t ascii_pos = 0;

		if (chunk > 8) {
			chunk = 8;
		}

		for (size_t i = 0; i < chunk; i++) {
			uint8_t byte = data[offset + i];

			snprintf(hex_part + hex_pos, sizeof(hex_part) - hex_pos,
				 "%02X ", byte);
			hex_pos += 3;

			ascii_part[ascii_pos++] = (byte >= 0x20 && byte <= 0x7E)
						      ? (char)byte : '.';
		}
		ascii_part[ascii_pos] = '\0';

		/* Pad hex part if less than 8 bytes */
		while (hex_pos < 3 * 8) {
			hex_part[hex_pos++] = ' ';
		}
		hex_part[hex_pos] = '\0';

		snprintf(line, sizeof(line), "%s %s", hex_part, ascii_part);

		if (dir == DATA_DIR_TX) {
			append_line(tx_lines, &tx_line_count, tx_data_label, line);
		} else {
			append_line(rx_lines, &rx_line_count, rx_data_label, line);
		}

		offset += chunk;
	}
}

void display_refresh(void)
{
	lv_timer_handler();
}
