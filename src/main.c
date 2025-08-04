/*
 * FSM-based BLE workflow on nRF52832 (Zephyr RTOS)
 * Peripheral (GATT Service) -> Central (BLE Scanner/Controller)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/uuid.h>
#include "led_strip_src/led_strip.h"
#include "motor_src/motor.h"
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include "ble_uuids.h"

#define LOG_LEVEL_INF   3
#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led1)
#define BUTTON0_NODE DT_ALIAS(brightness_incr)
#define BUTTON1_NODE DT_ALIAS(brightness_decr)

LOG_MODULE_REGISTER(ble_fsm_demo, LOG_LEVEL_INF);

extern led_cmd_t led_cmd_data;  // the struct filled in write_cb

// led pins
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

// button pins
static const struct gpio_dt_spec brightness_incr = GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios); // button 0: brightness up
static const struct gpio_dt_spec brightness_decr = GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios); // button 1: brightness down

// global variables
int sensor_flag = 0; // flag to detect when motor is on
static int sensor_led_mode = 0; // mode state flag
static int last_vibration_count = 0;

// Global brightness control (0-255)
static int global_brightness = 255; // Default brightness

// flags for command received
volatile bool ble_command_received = false;
volatile bool motor_command_received = false;
volatile bool motor_config_received = false;

/* FSM States */
typedef enum {
	STATE_IDLE,
	STATE_PERIPHERAL,
	STATE_LED_CTRL,
	STATE_MOTOR_CONFIG,
} ble_state_t;

static ble_state_t current_state = STATE_IDLE;
static struct bt_conn *current_conn = NULL;

// Characteristic names for UI/tools debug
#define CONTROL_SERVICE_NAME     "SLB Control Service"
#define LED_CTRL_NAME            "LED Control"
#define MOTOR_CTRL_NAME          "Motor Control"
#define MOTOR_CFG_NAME           "Motor Config"


/* Callback on read from phone*/
static ssize_t read_cb(struct bt_conn *conn,
		       const struct bt_gatt_attr *attr,
		       void *buf, uint16_t len,
		       uint16_t offset)
{
	const char *value = "LED On"; // read result: 4C 45 44 20 4F 6E
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

/* Callback on write from phone */
ssize_t write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	// data declare
    const uint8_t *data = (const uint8_t *)buf;

	// Convert to string for UTF-8 command detection
    char str_buf[64] = {0};
    uint16_t str_len = len < sizeof(str_buf) - 1 ? len : sizeof(str_buf) - 1;
    memcpy(str_buf, data, str_len); 
    str_buf[str_len] = '\0';
	
	printk("\nReceived command:");
    for (int i = 0; i < len; i++) {
        printk(" %02X", ((const uint8_t *)buf)[i]);
    }
    printk("\n");

	// compare UUID LED
	if (bt_uuid_cmp(attr->uuid, &led_char_uuid.uuid) == 0) {
		if (len == sizeof(led_cmd_t)) {
			ble_command_received = true;
			const led_cmd_t *cmd = (const led_cmd_t *)buf;
			memcpy(&led_cmd_data, cmd, sizeof(led_cmd_t));
			printk(">> Switching to STATE_LED_CTRL (brightness: %d) <<\n", global_brightness);
			current_state = STATE_LED_CTRL;
		} 
	} else if (bt_uuid_cmp(attr->uuid, &motor_char_uuid.uuid) == 0) {
		motor_command_received = true;
	} else if (bt_uuid_cmp(attr->uuid, &motor_config_char_uuid.uuid) == 0) {
		printk(">> Switching to STATE_MOTOR_CONFIG <<\n");
		motor_config_received = true;
		current_state = STATE_MOTOR_CONFIG;
	}

	// print string & debug
	// printk("String: '%s'\n", str_buf);
	// printk("len = %d\n", len);
	// printk("sizeof(led_cmd_t) = %d\n", sizeof(led_cmd_t));
	// printk("First byte: 0x%02X\n", ((const uint8_t *)buf)[0]);

    // return len;
}
/*
custom_svc: is the service UUID
control_service_uuid: UUID of the characteristic
BT_GATT_CHRC_WRITE: allows writing
BT_GATT_PERM_WRITE: permission for writing
write_cb: function to handle the write
read_cb : function to handle the read
*/

/* Custom GATT services */
BT_GATT_SERVICE_DEFINE(custom_svc,
	BT_GATT_PRIMARY_SERVICE(&control_service_uuid),

	BT_GATT_CHARACTERISTIC(&led_char_uuid.uuid,
	BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
	BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
	read_cb, write_cb, NULL),
	BT_GATT_CUD(LED_CTRL_NAME, BT_GATT_PERM_READ),

	BT_GATT_CHARACTERISTIC(&motor_char_uuid.uuid,
	BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
	BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
	read_cb, write_cb, NULL),
	BT_GATT_CUD(MOTOR_CTRL_NAME, BT_GATT_PERM_READ),

	BT_GATT_CHARACTERISTIC(&motor_config_char_uuid.uuid,
	BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
	BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
	read_cb, write_cb, NULL),
	BT_GATT_CUD(MOTOR_CFG_NAME, BT_GATT_PERM_READ),
);

/* Peripheral Callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		current_conn = conn;
		current_state = STATE_IDLE;
		printk("Phone is connected\n");
		gpio_pin_set_dt(&led1, 1);	
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	current_conn = NULL;
	printk("Phone disconnected\n");
	gpio_pin_set_dt(&led1, 0);
	if (current_state == STATE_IDLE) {
		bt_disable();
		current_state = STATE_IDLE;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Central: Scan callback */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN], name[32] = {0};
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Found device %s, RSSI: %d dBm\n", addr_str, rssi);
	
	// Extract name from advertising payload
    for (int i = 0; i < ad->len - 1;) {
        uint8_t len = ad->data[i++];
        if (len == 0 || (i + len - 1) > ad->len) break;
        uint8_t type = ad->data[i++];
        if (type == BT_DATA_NAME_COMPLETE || type == BT_DATA_NAME_SHORTENED) {
            memcpy(name, &ad->data[i], MIN(len - 1, sizeof(name) - 1));
            printk("Device name: %s\n", name);
            break;
        }
        i += len - 2;
    }

	// TODO: match UUID/name then connect (preserved)
	gpio_pin_set_dt(&led2, 1);
	current_state = STATE_IDLE;
	bt_le_scan_stop();
}

void start_scan(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0010,
		.window     = 0x0010,
	};

	int err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Scan failed (err %d)\n", err);
    } else {
        printk("Scanning started...\n");
    }
}

const char *motor_status(void)
	{
		motor_on();
    	return "ON";
	}

// button brightness handle
void button_handler(void){
	static bool last_incr_state = false;
	static bool last_decr_state = false;
	static bool config_initialized = false;

	// Initialize config state only once
	if (!config_initialized) {
		printk(">> STATE_MOTOR_CONFIG : ON <<\n");
		printk("Global brightness control: 0-255 (10 units per press)\n");
		printk("Current brightness: %d\n", global_brightness);
		config_initialized = true;
	}

	// Check buttons once per loop iteration (non-blocking)
	bool incr_pressed = gpio_pin_get_dt(&brightness_incr) == 0;
	bool decr_pressed = gpio_pin_get_dt(&brightness_decr) == 0;

	// Get the last command to preserve color settings
	const led_cmd_t* last_cmd = get_last_led_cmd();

	// Only print when button state changes from not pressed to pressed
	if (incr_pressed && !last_incr_state) {
		global_brightness += 10;
		if (global_brightness > 100) {
			global_brightness = 100;
		}
		printk(">> Global Brightness ++: %d\n", global_brightness);

		led_cmd_t brightness_cmd = *last_cmd;  // Copy last command
		brightness_cmd.brightness = global_brightness; // Sync it
		led_strip_control(&brightness_cmd); // Use latest brightness

	}

	if (decr_pressed && !last_decr_state) {
		global_brightness -= 10;
		if (global_brightness < 0) {
			global_brightness = 0;
		}
		printk(">> Global Brightness --: %d\n", global_brightness);

		led_cmd_t brightness_cmd = *last_cmd;  // Copy last command
		brightness_cmd.brightness = global_brightness; // Sync it
		led_strip_control(&brightness_cmd); // Use latest brightness
	}

	// Update last states
	last_incr_state = incr_pressed;
	last_decr_state = decr_pressed;

	// Small delay to prevent too rapid changes
	if (incr_pressed || decr_pressed) {
		k_msleep(50); // debounce
	}

	// safety check
	if (last_cmd == NULL) {
		printk("No previous command available\n");
		return;
	}
}

void main(void)
{    
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

	// button pins
	gpio_pin_configure_dt(&brightness_incr, GPIO_INPUT);
	gpio_pin_configure_dt(&brightness_decr, GPIO_INPUT);
	gpio_pin_set_dt(&brightness_incr, 1);
	gpio_pin_set_dt(&brightness_decr, 1);

	// delay
	k_sleep(K_MSEC(150));

	if (bt_enable(NULL) == 0) {
		printk("Bluetooth enabled\n");
		k_sleep(K_MSEC(150));
		current_state = STATE_PERIPHERAL;
	} else {
		printk("Bluetooth init failed\n");
		return;
	}

	// turn on sensor & detect flag
	while (1) {
		// FSM for BLE and sensor/LED logic
		switch (current_state) {
			case STATE_PERIPHERAL:
				printk("Acting as Peripheral...\n");
				bt_le_adv_start(BT_LE_ADV_CONN_NAME, NULL, 0, NULL, 0);
				current_state = STATE_IDLE;
				break;
			case STATE_IDLE:
				// Handle motor command if received
				if (motor_command_received) {
					printk(">> Motor control triggered <<\n");
					LOG_INF("Motor state: %s", motor_status());
					motor_command_received = false;
				}
				
				// Tap detection and LED mode cycling
				if (vibration_count != last_vibration_count) {
					motor_command_received = true;
					last_vibration_count = vibration_count;
					sensor_led_mode = (sensor_led_mode + 1) % 3;
					printk("Tap detected! New mode: %d\n", sensor_led_mode);
					led_cmd_t sensor_cmd = {
						.mode = sensor_led_mode,
						.r = 255, .g = 0, .b = 0, // Only used in mode 0
						.brightness = global_brightness, // Uses global brightness
						.duration = 0
					};
					k_sleep(K_MSEC(100));
					led_strip_control(&sensor_cmd);
				}
				k_sleep(K_MSEC(10));
				break;
			case STATE_LED_CTRL:
				// Handle BLE LED command
				led_strip_control(&led_cmd_data);
				current_state = STATE_IDLE; // Always return to IDLE
				break;
			case STATE_MOTOR_CONFIG:				
				// Check if motor config flag is enabled before executing
				if (motor_config_received) {

					button_handler();

				} else {
					// Exit motor config state when flag is cleared
					printk("Motor configuration: done\n");
					motor_config_received = false;
					current_state = STATE_IDLE;
				}
				break;
		}
	}	
}
