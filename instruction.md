## 🛠️ Tips

- Always send **exactly 6 bytes**
- All values must be in **hex** format
- If using a BLE terminal (e.g. nRF Connect, Serial Bluetooth Terminal), ensure to send in **Hex mode**, not ASCII

# LED Strip Control Examples (Hex Command Format)

This guide explains how to control an LED strip using 6-byte hex commands sent from a phone or other BLE terminal.

## 🔧 Format (6 bytes)
[mode][R][G][B][brightness][duration] = 6 bytes

| Byte     | Description          | Note                                      |
|----------|----------------------|-------------------------------------------|
| Byte 0   | Mode                 | `00`: Manual RGB<br>`01`: Relax mode<br>`02`: Blue night mode |
| Byte 1   | Red (R)              | Used only in mode `00`                    |
| Byte 2   | Green (G)            | Used only in mode `00`                    |
| Byte 3   | Blue (B)             | Used only in mode `00`                    |
| Byte 4   | Brightness (0–255)   | All modes                                 |
| Byte 5   | Duration (0–255)     | Developing...                             |

> 💡 All values are in hexadecimal (00–FF)

---

## 🟢 Mode 00 – Manual RGB Color

LED uses exact RGB values with specified brightness.

### Example 1 – White, 100% brightness
00 FF FF FF 64 00
- RGB: White (`FF FF FF`)
- Brightness: `64` (100%)

### Example 2 – Blue, 50% brightness
00 00 00 FF 32 00
- RGB: Blue (`00 00 FF`)
- Brightness: `32` (50%)

### Example 3 – Red, 10% brightness
00 FF 00 00 0A 00
- RGB: Red (`FF 00 00`)
- Brightness: `0A` (10%)

---

## 🟠 Mode 01 – Relax Mode

Amber tone with warm color. Ignores R/G/B input.

### Example 4 – Relax, 100% brightness
01 00 00 00 64 00

### Example 5 – Relax, 25% brightness
01 00 00 00 19 00

---

## 🌙 Mode 02 – Blue Light Night Mode

Cool blue tone with soft brightness. Ignores R/G/B input.

### Example 6 – Blue night, 50% brightness
02 00 00 00 32 00

### Example 7 – Blue night, 10% brightness
02 00 00 00 0A 00

---

## 🧪 Reserved: Duration Field

- Currently unused
- Can be set to `00`
- Reserved for future effects like blinking, timed fades, further functions, etc.
