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

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static const struct led_rgb colors[] = {
	RGB(CONFIG_SAMPLE_LED_BRIGHTNESS, 0x00, 0x00), /* red */
	RGB(0x00, CONFIG_SAMPLE_LED_BRIGHTNESS, 0x00), /* green */
	RGB(0x00, 0x00, CONFIG_SAMPLE_LED_BRIGHTNESS), /* blue */
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

void led_strip_animate(void)
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

// control
void led_strip_control(const led_cmd_t *cmd)
{
	int rc;

	LOG_INF("Control mode: %d RGB: %02X %02X %02X Brightness: %d Duration: %d",
		cmd->mode, cmd->r, cmd->g, cmd->b, cmd->brightness, cmd->duration);

	// Convert brightness (0–100%) to 0–255 scale
	uint8_t r = (cmd->r * cmd->brightness) / 100;
	uint8_t g = (cmd->g * cmd->brightness) / 100;
	uint8_t b = (cmd->b * cmd->brightness) / 100;

	switch (cmd->mode) {
	case 0: {
		// Mode 0: Static RGB color
		LOG_INF("test case 0 ok");
		struct led_rgb pixel = { .r = r, .g = g, .b = b };

		for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
			pixels[i] = pixel;
		}

		rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
		if (rc) {
			LOG_ERR("LED update failed: %d", rc);
		}
		break;
	}

	case 1:
		// Mode 1: “Relax” mode – soft amber/yellow warm tone fade or pulse
		// You can simulate fade in/out or alternate brightness
		LOG_INF("test case 1 ok");
		LOG_INF("Relax mode not yet implemented");
		break;

	case 2:
		// Mode 2: “Pre-sleep” soft color transitions
		LOG_INF("Pre-sleep mode not yet implemented");
		break;

	case 3:
		// Mode 3: “Wake-up” – simulate sunrise with bright white/yellow
		LOG_INF("Wake-up mode not yet implemented");
		break;

	default:
		LOG_WRN("Unknown mode: %d", cmd->mode);
		break;
	}
}
