[English below]

Ở phiên bản đầu tiên này, workflow sẽ hoạt động như sau:

Ban đầu: nRF52833 hoạt động ở chế độ Peripheral (GATT Server)
- phát sóng Bluetooth (advertising)
- chờ thiết bị Central (như điện thoại) kết nối
Sau khi kết nối thành công:
- Central gửi lệnh “SCAN” bằng cách 'write' vào một characteristic tuỳ định nghĩa
- nRF52833 nhận lệnh đó, dừng vai trò Peripheral, gọi bt_le_scan_start()
	=> giờ nó chuyển sang vai trò Central
	=> bắt đầu dò thiết bị Peripheral khác
	
# 🔄 BLE Finite State Machine (FSM) – Version 1

This project demonstrates a **dynamic role-switching** behavior using the **nRF52833** of Noridc Semiconductor and the **Zephyr RTOS Bluetooth stack**. The device acts as a **Peripheral** initially and switches to **Central** mode based on a GATT command from a connected device (e.g., a smartphone).

---

## 📡 Workflow Overview

### 1️⃣ Initial State – Peripheral Mode (GATT Server)
- The `nRF52833` starts in **Peripheral** mode.
- It advertises its presence over Bluetooth.
- Awaits a connection from a **Central device** (such as a smartphone).

### 2️⃣ GATT Command Trigger
- Once connected, the **Central** device writes a command `"SCAN"` to a predefined **GATT characteristic**.
- The `nRF52833` receives this command and performs the following actions:
  - Stops advertising (disabling Peripheral role).
  - Calls `bt_le_scan_start()` to begin scanning for nearby Bluetooth devices.

### 3️⃣ Switch to Central Mode
- After transitioning, the `nRF52833` now acts as a **Central** device.
- It scans and lists nearby **Peripheral** devices (e.g., speakers or sensors).

---

## 🔧 Technical Notes
- GATT service and characteristic UUIDs are defined in the firmware to handle incoming BLE commands.
- BLE stack features like **dual-role (Central + Peripheral)**, **observer mode**, and **GATT write handling** are enabled via `prj.conf`.

---

## 📱 Use Case Example
This behavior is ideal for applications where a device:
- Starts in advertising mode to receive control input.
- Then scans for other devices (e.g., to relay commands or collect data).

