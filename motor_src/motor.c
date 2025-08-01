#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include "motor.h"
#include <stdio.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec signal = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, signal_gpios);

int vibration_count = 0; 

int motor_on(void)
{
    /* Configure the pin */
    gpio_pin_configure_dt(&signal, GPIO_INPUT | signal.dt_flags);
    

    /* Set the pin to its active level */
    gpio_pin_set_dt(&signal, 1);

    // read pin
    int val = gpio_pin_get_dt(&signal);
    if (val == 0) {
        k_sleep(K_MSEC(2));
        if (val == 0){
            vibration_count++;
            // printk("RUNG OK [%d]\n", vibration_count);
            // prevent repeated count on same event
            while (gpio_pin_get_dt(&signal) == 0) {
                k_sleep(K_MSEC(1));              // Wait until signal released
            }

            return 1;
        }
    }
    return 0;
}