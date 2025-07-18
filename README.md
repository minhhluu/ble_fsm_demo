# BLE FSM Demo — nRF52832 + Zephyr RTOS

## Overview

- **Idle** – Waiting for initialization.
- **Advertising** – Broadcasting device presence for connection.
- **Connected** – Active BLE connection with a peer.
- **Disconnected** – Handling disconnects and reset logic.

## Features

- ✅ Built on Zephyr RTOS (modular and scalable)
- ✅ Designed for nRF52 DK (nRF52832 SoC)
- ✅ FSM-based BLE control logic
- ✅ Clean separation of states and transitions
- ✅ Lightweight and portable codebase

## Dependencies

- Zephyr SDK
- Nordic nRF52 DK (nRF52832)
- west (Zephyr build tool)
- Python 3.x (for Zephyr scripts)

## Build & Flash

```bash
# Navigate to the project folder
cd zephyr_proj/projects/ble_fsm_demo

# Build for nRF52DK with nRF52832
west build -b nrf52dk/nrf52832 -p

# Flash to board
west flash
