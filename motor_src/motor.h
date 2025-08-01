#ifndef MOTOR_H
#define MOTOR_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int vibration_count;

/** @brief Detect vibration and update count */
int motor_on(void);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_H