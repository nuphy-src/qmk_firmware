#include "ansi.h"
#include "ble_cmd.h"

// rf.c
extern void    UART_Send_Bytes(uint8_t *Buffer, uint32_t Length);
extern uint8_t get_checksum(uint8_t *buf, uint8_t len);

// ansi.c
extern bool           f_dial_sw_init_ok;
extern DEV_INFO_STRUCT dev_info;
extern user_config_t  user_config;

// -------------------------------------------------------------------------
// Internal: send a consumer HID report directly via UART to the nRF module.
// Bypasses uart_send_report()'s link_mode and rf_state guards intentionally —
// we want the nRF to forward this over BLE regardless of the main link mode.
// -------------------------------------------------------------------------
static void ble_cmd_send(uint16_t usage) {
    if (!f_dial_sw_init_ok) return;

    uint8_t buf[7];
    buf[0] = UART_HEAD;        // 0x5A
    buf[1] = CMD_RPT_CONSUME;  // 0xE3
    buf[2] = 0x01;
    buf[3] = 2;                // payload length
    buf[4] = (uint8_t)(usage & 0xFF);
    buf[5] = (uint8_t)(usage >> 8);
    buf[6] = get_checksum(&buf[4], 2);
    UART_Send_Bytes(buf, 7);
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

bool process_record_ble_cmd(uint16_t keycode, bool pressed) {
    if (keycode < BLE_CMD_0 || keycode > BLE_CMD_31) return false;

    // Consume the keycode even when disabled/wrong mode to prevent passthrough
    if (!f_ble_cmd_enabled) return true;
    if (dev_info.link_mode != LINK_USB) return true;

    uint8_t cmd_id = keycode - BLE_CMD_0;
    ble_cmd_send(pressed ? (uint16_t)(BLE_CMD_BASE + cmd_id) : 0x0000);
    return true;
}

uint8_t ble_cmd_sync_mode(void) {
    if (f_ble_cmd_enabled && dev_info.link_mode == LINK_USB) {
        return user_config.ee_ble_cmd_slot;
    }
    return dev_info.link_mode;
}

bool ble_cmd_accepts_sync(uint8_t mode) {
    return f_ble_cmd_enabled &&
           dev_info.link_mode == LINK_USB &&
           mode == user_config.ee_ble_cmd_slot;
}

// -------------------------------------------------------------------------
// VIA custom value callback (only compiled when VIA_ENABLE = yes)
// -------------------------------------------------------------------------
#ifdef VIA_ENABLE
#include "via.h"

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value... ]
    if (data[1] != BLE_CMD_VIA_CHANNEL) {
        data[0] = id_unhandled;
        return;
    }

    uint8_t command_id = data[0];
    uint8_t value_id   = data[2];

    switch (command_id) {
        case id_custom_get_value:
            switch (value_id) {
                case BLE_CMD_VIA_ENABLED:
                    data[3] = f_ble_cmd_enabled ? 1 : 0;
                    break;
                case BLE_CMD_VIA_SLOT:
                    // VIA dropdown is 0-based: 0=BLE1, 1=BLE2, 2=BLE3
                    data[3] = user_config.ee_ble_cmd_slot - LINK_BT_1;
                    break;
                default:
                    data[0] = id_unhandled;
                    break;
            }
            break;

        case id_custom_set_value:
            switch (value_id) {
                case BLE_CMD_VIA_ENABLED:
                    f_ble_cmd_enabled = data[3] ? true : false;
                    break;
                case BLE_CMD_VIA_SLOT:
                    if (data[3] <= 2) {
                        user_config.ee_ble_cmd_slot = LINK_BT_1 + data[3];
                    }
                    break;
                default:
                    data[0] = id_unhandled;
                    break;
            }
            break;

        case id_custom_save:
            eeconfig_update_user_datablock(&user_config);
            break;

        default:
            data[0] = id_unhandled;
            break;
    }
}
#endif // VIA_ENABLE
