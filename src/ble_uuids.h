#ifndef BLE_UUIDS_H
#define BLE_UUIDS_H

#include <zephyr/bluetooth/uuid.h>

// Custom 128-bit UUIDs
#define BT_UUID_CONTROL_SERVICE_VAL         BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001000)
#define BT_UUID_LED_CONTROL_CHAR_VAL        BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001001)
#define BT_UUID_MOTOR_CHAR_VAL              BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001002)
#define BT_UUID_MOTOR_CONFIG_CHAR_VAL       BT_UUID_128_ENCODE(0x534C4220, 0x4441, 0x4E54, 0x494E, 0x4F0000001003)

// Structs for binding
static struct bt_uuid_128 control_service_uuid     = BT_UUID_INIT_128(BT_UUID_CONTROL_SERVICE_VAL);
static struct bt_uuid_128 led_char_uuid             = BT_UUID_INIT_128(BT_UUID_LED_CONTROL_CHAR_VAL);
static struct bt_uuid_128 motor_char_uuid           = BT_UUID_INIT_128(BT_UUID_MOTOR_CHAR_VAL);
static struct bt_uuid_128 motor_config_char_uuid    = BT_UUID_INIT_128(BT_UUID_MOTOR_CONFIG_CHAR_VAL);

#endif /* BLE_UUIDS_H */