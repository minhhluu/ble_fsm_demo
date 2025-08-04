/*
 * Refactored from original main.c
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_strip);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "led_strip.h"

#define STRIP_NODE		DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define DELAY_TIME K_MSEC(CONFIG_SAMPLE_LED_UPDATE_DELAY)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

led_cmd_t led_cmd_data;

// Declare the static variable to store last command
static led_cmd_t last_led_cmd;

// global variables
static int global_brightness = 100;

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static const struct led_rgb colors[] = {
	RGB(255, 0, 0),   /* red */
    RGB(0, 255, 0),   /* green */  
    RGB(0, 0, 255),   /* blue */
};

static struct led_rgb pixels[STRIP_NUM_PIXELS];

int led_strip_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return -ENODEV;
	}

	LOG_INF("Found LED strip device %s", strip->name);
	return 0;
}

void led_strip_default(void)
{
	static size_t color = 0;
	int rc;

	for (size_t cursor = 0; cursor < ARRAY_SIZE(pixels); cursor++) {
		memset(&pixels, 0x00, sizeof(pixels));
		memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));

		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
		if (rc) {
			LOG_ERR("couldn't update strip: %d", rc);
		}

		k_sleep(DELAY_TIME);
	}

	color = (color + 1) % ARRAY_SIZE(colors);
}

// [mode][R][G][B][brightness][duration] = 6 bytes

static struct led_rgb scale_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
	struct led_rgb color;
	// Use global brightness instead of passed brightness
	color.r = (r * global_brightness) / 100;
	color.g = (g * global_brightness) / 100;
	color.b = (b * global_brightness) / 100;
	
	return color;
}

void led_strip_control(const led_cmd_t *cmd)
{	
	int rc;
	// Cache the command so brightness-only updates can use it
    last_led_cmd = *cmd;  // Save the entire command

	// LOG_HEXDUMP_INF(cmd, sizeof(*cmd), "Command Struct:");
	LOG_INF("Control mode: %d RGB: %02X %02X %02X Brightness: %d Duration: %d",
		cmd->mode, cmd->r, cmd->g, cmd->b, cmd->brightness, cmd->duration);

	global_brightness = cmd->brightness;  // Update global brightness (0-100 range)

	struct led_rgb color;

	switch (cmd->mode) {
	case 0:
		LOG_INF("Mode 0: Direct RGB control (always on)");
		color = scale_rgb(cmd->r, cmd->g, cmd->b, cmd->brightness);
		break;

	case 1:
		LOG_INF("Mode 1: Relax mode (warm amber)");
		color = scale_rgb(255, 160, 64, cmd->brightness);
		break;

	case 2:
		LOG_INF("Mode 2: Blue light night mode");
		color = scale_rgb(0, 0, 255, cmd->brightness);
		break;

	default:
		LOG_WRN("Unsupported mode: %d", cmd->mode);
		return -1;
	}

	// Apply color to all pixels
	for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
		pixels[i] = color;
	}

	// Update LED strip
	rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
	if (rc) {
		LOG_ERR("LED update failed: %d", rc);
		return rc;
	}

	return 0;
}

// Add a getter function to retrieve the last command
const led_cmd_t* get_last_led_cmd(void) {
    return &last_led_cmd;
}