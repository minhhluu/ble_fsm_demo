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

#define LOG_LEVEL_INF   3
#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(ble_fsm_demo, LOG_LEVEL_INF);

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
extern led_cmd_t led_cmd_data;  // the struct filled in write_cb

// global variables
int sensor_flag = 0; // flag to detect when motor is on
static int sensor_led_mode = 0; // mode state flag
static int last_vibration_count = 0;

/* FSM States */
typedef enum {
	STATE_IDLE,
	STATE_PERIPHERAL,
	STATE_CONNECTED,
	STATE_SWITCH_TO_CENTRAL,
	STATE_LED_CTRL,
	STATE_MOTOR_CTRL,
	STATE_CENTRAL_SCANNING,
	STATE_CENTRAL_CONNECTED
} ble_state_t;

static ble_state_t current_state = STATE_IDLE;
static struct bt_conn *current_conn = NULL;

/* Custom GATT Service */
#define BT_UUID_CONTROL_SERVICE_VAL  BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001000)
#define BT_UUID_LED_CONTROL_CHAR_VAL     BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001001)
#define BT_UUID_MOTOR_CHAR_VAL     BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001002)

static struct bt_uuid_128 control_service_uuid = BT_UUID_INIT_128(BT_UUID_CONTROL_SERVICE_VAL);
static struct bt_uuid_128 led_char_uuid = BT_UUID_INIT_128(BT_UUID_LED_CONTROL_CHAR_VAL);
static struct bt_uuid_128 motor_char_uuid = BT_UUID_INIT_128(BT_UUID_MOTOR_CHAR_VAL);


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
			const led_cmd_t *cmd = (const led_cmd_t *)buf;
			memcpy(&led_cmd_data, cmd, sizeof(led_cmd_t));  // Store data for later use
			current_state = STATE_LED_CTRL;

			
    	} else if (len >= 4 && memcmp(buf, "SCAN", 4) == 0) {
        	current_state = STATE_SWITCH_TO_CENTRAL;
        	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    	} 
	
	// compare UUID motor
	} else if (bt_uuid_cmp(attr->uuid, &motor_char_uuid.uuid) == 0){
    	current_state = STATE_MOTOR_CTRL;
	}
    

	// print string
	printk("String: '%s'\n", str_buf);

	// debug
	printk("len = %d\n", len);
	printk("sizeof(led_cmd_t) = %d\n", sizeof(led_cmd_t));
	printk("First byte: 0x%02X\n", ((const uint8_t *)buf)[0]);

    return len;
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

	BT_GATT_CHARACTERISTIC(&motor_char_uuid.uuid,
	BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
	BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
	read_cb, write_cb, NULL),
);

/* Peripheral Callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		current_conn = conn;
		current_state = STATE_CONNECTED;
		printk("Phone is connected\n");
		gpio_pin_set_dt(&led1, 1);	
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	current_conn = NULL;
	printk("Phone disconnected\n");
	gpio_pin_set_dt(&led1, 0);
	if (current_state == STATE_SWITCH_TO_CENTRAL) {
		bt_disable();
		current_state = STATE_CENTRAL_SCANNING;
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
	current_state = STATE_CENTRAL_CONNECTED;
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

void main(void)
{    
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

	if (bt_enable(NULL) == 0) {
		printk("Bluetooth enabled\n");
		current_state = STATE_PERIPHERAL;
	} else {
		printk("Bluetooth init failed\n");
		return;
	}

	// print motor state log
	LOG_INF("Motor state: %s", motor_status());


	// turn on sensor & detect flag
	while (1) {	
		if (motor_on() == 1){
			sensor_flag = 1;
		}
	
		if (vibration_count != last_vibration_count) {
			last_vibration_count = vibration_count;

			// Cycle to next mode
		    sensor_led_mode = (sensor_led_mode + 1) % 3;
			// sensor_led_mode++; // increase state every time tapping

			if (sensor_led_mode > 2) {
            	sensor_led_mode = 0;
        	}

			led_cmd_t sensor_cmd = {
           		.mode = sensor_led_mode,
           		.r = 0, .g = 0, .b = 0,
           		.brightness = 128,
           		.duration = 0
       		};
				led_strip_control(&sensor_cmd);
		}
		k_sleep(K_MSEC(10));

		switch (current_state) {
			case STATE_PERIPHERAL:
				printk("Acting as Peripheral...\n");
				bt_le_adv_start(BT_LE_ADV_CONN_NAME, NULL, 0, NULL, 0);
				current_state = STATE_IDLE;
				break;

			case STATE_CENTRAL_SCANNING:
				printk("Switching to Central mode...\n");
				bt_enable(NULL);
				start_scan();
				current_state = STATE_IDLE;
				break;

			case STATE_LED_CTRL: {
				printk("Entering LED control mode...\n");

				led_strip_init();
				k_sleep(K_MSEC(100));

				// LED Handle
				led_strip_control(&led_cmd_data);

				// Wait sensor_flag to change state
				while (!sensor_flag) {
					k_sleep(K_MSEC(100));
				}

				printk("Exiting LED control mode...\n");

				// Change state when sensor_flag is TRUE
				current_state = STATE_MOTOR_CTRL;

				break;
			}

			case STATE_MOTOR_CTRL:
				printk(">> Motor control state entered <<\n");
				k_sleep(K_MSEC(100));
				
				// Motor control logic
				current_state = STATE_IDLE;
				break;
		}
	}	
}
