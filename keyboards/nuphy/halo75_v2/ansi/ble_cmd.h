#pragma once

#include <stdint.h>
#include <stdbool.h>

// Consumer HID usage base for BLE commands.
// Values 0xF000-0xF01F are vendor-defined / unassigned in the Consumer page.
#define BLE_CMD_BASE        0xF000
#define BLE_CMD_COUNT       32

// VIA custom channel and value IDs
#define BLE_CMD_VIA_CHANNEL  6
#define BLE_CMD_VIA_ENABLED  1
#define BLE_CMD_VIA_SLOT     2

// Called from process_record_kb() before the switch statement.
// Returns true if the keycode was a BLE_CMD and was consumed.
bool process_record_ble_cmd(uint16_t keycode, bool pressed);

// Returns the link mode to report to the nRF module in CMD_RF_STS_SYSC.
// When BLE cmd is enabled and keyboard is on USB, returns the configured BLE slot
// so the nRF module stays connected over BLE.
uint8_t ble_cmd_sync_mode(void);

// Returns true if 'mode' is the BLE slot we're using for commands.
// Used in RF_Protocol_Receive to accept the nRF sync response without triggering
// the error counter that would fight the BLE connection.
bool ble_cmd_accepts_sync(uint8_t mode);
