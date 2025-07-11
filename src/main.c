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
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/* FSM States */
typedef enum {
	STATE_IDLE,
	STATE_PERIPHERAL,
	STATE_CONNECTED,
	STATE_SWITCH_TO_CENTRAL,
	STATE_CENTRAL_SCANNING,
	STATE_CENTRAL_CONNECTED
} ble_state_t;

static ble_state_t current_state = STATE_IDLE;
static struct bt_conn *current_conn = NULL;

/* Custom GATT Service */
#define BT_UUID_CUSTOM_SERVICE_VAL  BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_CUSTOM_CHAR_VAL     BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

/* Callback on write from phone */
ssize_t write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	char cmd[20] = {0};
	snprintf(cmd, sizeof(cmd), "%.*s", len, (char *)buf);
	printk("Received command: %s\n", cmd);

	if (strcmp(cmd, "SCAN") == 0) {
		current_state = STATE_SWITCH_TO_CENTRAL;
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	return len;
}
/*
custom_svc: is the service UUID
custom_service_uuid: UUID of the characteristic
BT_GATT_CHRC_WRITE: allows writing
BT_GATT_PERM_WRITE: permission for writing
write_cb: function to handle the write
*/
BT_GATT_SERVICE_DEFINE(custom_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
	BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
				BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_WRITE,
				NULL, write_cb, NULL),
);

/* Peripheral Callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		current_conn = conn;
		current_state = STATE_CONNECTED;
		printk("Phone connected\n");
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

	// TODO: match UUID/name then connect
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

void main(void)
{
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

	printk("BLE FSM example start\n");

	if (bt_enable(NULL) == 0) {
		printk("Bluetooth enabled\n");
		current_state = STATE_PERIPHERAL;
	} else {
		printk("Bluetooth init failed\n");
		return;
	}

	while (1) {
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
		default:
			k_sleep(K_MSEC(500));
		}
	}
}
