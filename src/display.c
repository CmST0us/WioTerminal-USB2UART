/*
 * SPDX-License-Identifier: Apache-2.0
 * Nothing Design — monochrome, typographic, subtractive
 */

#include "display.h"
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#define SCREEN_W 320
#define SCREEN_H 240

/* Nothing Design color palette */
#define COL_BG         lv_color_black()
#define COL_TEXT       lv_color_white()
#define COL_SECONDARY  lv_color_make(0x99, 0x99, 0x99)
#define COL_DISABLED   lv_color_make(0x55, 0x55, 0x55)
#define COL_ACCENT     lv_color_make(0xD7, 0x19, 0x21) /* Nothing red */
#define COL_CONNECTED  lv_color_make(0x00, 0xE6, 0x76) /* green dot */

/* Wire colors per user spec */
#define COL_WIRE_RX    lv_color_make(0xFF, 0xFF, 0xFF) /* white = RX */
#define COL_WIRE_TX    lv_color_make(0xFF, 0xD6, 0x00) /* yellow = TX */
#define COL_WIRE_VCC   lv_color_make(0xFF, 0x2D, 0x2D) /* red = VCC */
#define COL_WIRE_GND   lv_color_make(0x66, 0x66, 0x66) /* dark gray = GND */

/* Y layout anchors */
#define HERO_Y         32
#define CONFIG_Y       100
#define GROVE_Y        130

/* LVGL objects */
static lv_obj_t *baud_label;     /* hero: baudrate number */
static lv_obj_t *baud_unit;      /* "baud" label */
static lv_obj_t *conn_dot;       /* connection indicator */
static lv_obj_t *config_label;   /* 8N1 config line */
static lv_obj_t *grove_title;    /* "GROVE" label */
static lv_obj_t *wire_labels[4]; /* RX, TX, VCC, GND */
static lv_obj_t *wire_dots[4];   /* colored dots */
static lv_obj_t *usb_label;      /* "USB" at bottom center */

static const char *stop_bits_str(uint8_t sb)
{
	switch (sb) {
	case 0:  return "0.5";
	case 15: return "1.5";
	case 2:  return "2";
	default: return "1";
	}
}

static const char *parity_str(uint8_t p)
{
	switch (p) {
	case 1:  return "O";
	case 2:  return "E";
	default: return "N";
	}
}

void display_init(void)
{
	const struct device *display_dev;
	lv_obj_t *scr;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		return;
	}

	display_blanking_off(display_dev);
	scr = lv_screen_active();
	lv_obj_set_style_bg_color(scr, COL_BG, LV_PART_MAIN);

	/* ── Primary: Baudrate hero number ── */
	baud_label = lv_label_create(scr);
	lv_label_set_text(baud_label, "---");
	lv_obj_set_style_text_font(baud_label, &lv_font_montserrat_38, LV_PART_MAIN);
	lv_obj_set_style_text_color(baud_label, COL_TEXT, LV_PART_MAIN);
	lv_obj_align(baud_label, LV_ALIGN_TOP_LEFT, 20, HERO_Y);

	/* "baud" unit label — tight to the number */
	baud_unit = lv_label_create(scr);
	lv_label_set_text(baud_unit, "baud");
	lv_obj_set_style_text_font(baud_unit, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(baud_unit, COL_SECONDARY, LV_PART_MAIN);
	lv_obj_align_to(baud_unit, baud_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

	/* Connection dot — top right, accent when connected */
	conn_dot = lv_obj_create(scr);
	lv_obj_set_size(conn_dot, 10, 10);
	lv_obj_align(conn_dot, LV_ALIGN_TOP_RIGHT, -20, HERO_Y + 6);
	lv_obj_set_style_bg_color(conn_dot, COL_DISABLED, LV_PART_MAIN);
	lv_obj_set_style_border_width(conn_dot, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(conn_dot, 5, LV_PART_MAIN);

	/* ── Secondary: Config line (8N1) ── */
	config_label = lv_label_create(scr);
	lv_label_set_text(config_label, "8 N 1");
	lv_obj_set_style_text_font(config_label, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_obj_set_style_text_color(config_label, COL_SECONDARY, LV_PART_MAIN);
	lv_obj_align(config_label, LV_ALIGN_TOP_LEFT, 20, CONFIG_Y);

	/* ── Tertiary: Grove interface ── */
	grove_title = lv_label_create(scr);
	lv_label_set_text(grove_title, "GROVE  LEFT");
	lv_obj_set_style_text_font(grove_title, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(grove_title, COL_DISABLED, LV_PART_MAIN);
	lv_obj_set_style_text_letter_space(grove_title, 2, LV_PART_MAIN);
	lv_obj_align(grove_title, LV_ALIGN_TOP_LEFT, 20, GROVE_Y);

	/* Wire color legend — stacked lines with colored dot + label */
	static const char *wire_names[] = { "RX", "TX", "VCC", "GND" };
	static const char *color_names[] = { "WHITE", "YELLOW", "RED", "BLACK" };
	lv_color_t wc[4];

	wc[0] = COL_WIRE_RX;   /* white */
	wc[1] = COL_WIRE_TX;   /* yellow */
	wc[2] = COL_WIRE_VCC;  /* red */
	wc[3] = COL_WIRE_GND;  /* dark gray */

	for (int i = 0; i < 4; i++) {
		int y_off = GROVE_Y + 20 + i * 16;
		int x = 24;

		/* colored dot */
		wire_dots[i] = lv_obj_create(scr);
		lv_obj_set_size(wire_dots[i], 6, 6);
		lv_obj_align(wire_dots[i], LV_ALIGN_TOP_LEFT, x, y_off + 2);
		lv_obj_set_style_bg_color(wire_dots[i], wc[i], LV_PART_MAIN);
		lv_obj_set_style_border_width(wire_dots[i], 0, LV_PART_MAIN);
		lv_obj_set_style_radius(wire_dots[i], 3, LV_PART_MAIN);

		/* label: "WHITE = RX" */
		char buf[24];

		snprintf(buf, sizeof(buf), "%s = %s", color_names[i], wire_names[i]);

		wire_labels[i] = lv_label_create(scr);
		lv_label_set_text(wire_labels[i], buf);
		lv_obj_set_style_text_font(wire_labels[i], &lv_font_montserrat_10, LV_PART_MAIN);
		lv_obj_set_style_text_color(wire_labels[i], COL_SECONDARY, LV_PART_MAIN);
		lv_obj_align(wire_labels[i], LV_ALIGN_TOP_LEFT, x + 12, y_off);
	}

	/* ── USB interface — bottom center ── */
	usb_label = lv_label_create(scr);
	lv_label_set_text(usb_label, "USB");
	lv_obj_set_style_text_font(usb_label, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_set_style_text_color(usb_label, COL_DISABLED, LV_PART_MAIN);
	lv_obj_set_style_text_letter_space(usb_label, 3, LV_PART_MAIN);
	lv_obj_align(usb_label, LV_ALIGN_BOTTOM_MID, 0, -6);

	/* Horizontal rule above USB */
	lv_obj_t *rule = lv_obj_create(scr);
	lv_obj_set_size(rule, SCREEN_W - 40, 1);
	lv_obj_align(rule, LV_ALIGN_BOTTOM_MID, 0, -22);
	lv_obj_set_style_bg_color(rule, COL_DISABLED, LV_PART_MAIN);
	lv_obj_set_style_border_width(rule, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(rule, 0, LV_PART_MAIN);
}

void display_update_status(uint32_t baudrate, uint8_t data_bits, uint8_t stop_bits,
			   uint8_t parity, bool connected)
{
	char buf[16];

	/* Hero: baudrate number */
	snprintf(buf, sizeof(buf), "%u", baudrate);
	lv_label_set_text(baud_label, buf);

	/* Config line: "8 N 1" */
	snprintf(buf, sizeof(buf), "%u %s %s", data_bits, parity_str(parity), stop_bits_str(stop_bits));
	lv_label_set_text(config_label, buf);

	/* Connection dot */
	lv_obj_set_style_bg_color(conn_dot, connected ? COL_CONNECTED : COL_DISABLED,
				  LV_PART_MAIN);
}

void display_refresh(void)
{
	lv_timer_handler();
}
