#pragma once

#include <zephyr/device.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/** Initialize LED strip (configure device, check ready state) */
int led_strip_init(void);

/** Run LED animation in a loop */
void led_strip_animate(void);

/** LED command format from BLE write */
typedef struct {
	uint8_t mode;        // 0–5
	uint8_t r, g, b;     // RGB values
	uint8_t brightness;  // 0–100%
	uint8_t duration;    // 1–250 (unit: 50ms)
} led_cmd_t;

/** Control LED strip once based on command data */
void led_strip_control(const led_cmd_t *cmd);

#ifdef __cplusplus
}
#endif

