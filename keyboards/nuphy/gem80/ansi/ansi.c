// Copyright 2023 Persama (@Persama)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "ansi.h"
#include "usb_main.h"
#include "rf_driver.h"


#define RF_LONG_PRESS_DELAY   30
#define DEV_RESET_PRESS_DELAY 30
#define RGB_TEST_PRESS_DELAY  30

user_config_t user_config;

DEV_INFO_STRUCT dev_info =
    {
        .rf_baterry = 100,
        .link_mode  = LINK_USB,
        .rf_state   = RF_IDLE,
};

uint16_t       rf_linking_time       = 0;
uint16_t       rf_link_show_time     = 0;
uint8_t        rf_blink_cnt          = 0;
uint16_t       no_act_time           = 0;
uint8_t        rf_sw_temp            = 0;
uint16_t       dev_reset_press_delay = 0;
uint16_t       rf_sw_press_delay     = 0;
uint16_t       rgb_test_press_delay  = 0;
uint8_t        host_mode;
host_driver_t *m_host_driver = 0;

extern uint8_t side_mode;
extern uint8_t side_light;
extern uint8_t side_speed;
extern uint8_t side_rgb;
extern uint8_t side_colour;
extern uint8_t logo_mode;
extern uint8_t logo_light;
extern uint8_t logo_speed;
extern uint8_t logo_rgb;
extern uint8_t logo_colour;
extern uint8_t uart_bit_report_buf[32];
extern uint8_t bitkb_report_buf[32];
extern uint8_t bytekb_report_buf[8];

bool f_goto_sleep       = 0;
bool f_bat_show         = 0;
bool f_bat_hold         = 0;
bool f_chg_show         = 1;
bool f_sys_show         = 0;
bool f_sleep_show       = 0;
bool f_func_save        = 0;
bool f_rf_read_data_ok  = 0;
bool f_rf_sts_sysc_ok   = 0;
bool f_rf_new_adv_ok    = 0;
bool f_rf_reset         = 0;
bool f_send_channel     = 0;
bool f_rf_send_bitkb    = 0;
bool f_rf_send_byte     = 0;
bool f_rf_send_consume  = 0;
bool f_dial_sw_init_ok  = 0;
bool f_rf_sw_press      = 0;
bool f_wakeup_prepare   = 0; 
bool f_dev_reset_press  = 0;
bool f_rgb_test_press   = 0;
bool f_win_lock         = 0;
bool f_uart_ack         = 0;
bool f_rf_hand_ok       = 0;


void rf_uart_init(void);
void rf_device_init(void);
void m_side_led_show(void);
void dev_sts_sync(void);
void uart_send_report_func(void);
void uart_receive_pro(void);
void Sleep_Handle(void);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack_cnt, uint8_t delayms);
void uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);
void device_reset_show(void);
void device_reset_init(void);
void rgb_test_show(void);

extern void light_speed_control(uint8_t fast);
extern void light_level_control(uint8_t brighten);
extern void side_colour_control(uint8_t dir);
extern void side_mode_control(uint8_t dir);
extern void logo_light_speed_control(uint8_t fast);
extern void logo_light_level_control(uint8_t brighten);
extern void logo_side_colour_control(uint8_t dir);
extern void logo_side_mode_control(uint8_t dir);
extern report_nkro_t *nkro_report;

void flash_data_manage(void)
{
    if (f_func_save) {
        f_func_save = 0;
        uart_send_cmd(CMD_WRITE_DATA, 20, 30);
    }
}

/**
 * @brief  gpio initial.
 */
void m_gpio_init(void)
{
    setPinOutput(DC_BOOST_PIN); writePinHigh(DC_BOOST_PIN);

    setPinInput(DRIVER_LED_CS_PIN);
    setPinInput(DRIVER_SIDE_CS_PIN);
    setPinOutput(DRIVER_SIDE_PIN); writePinLow(DRIVER_SIDE_PIN);
#if(WORK_MODE == THREE_MODE)
    setPinOutput(NRF_WAKEUP_PIN);
    writePinHigh(NRF_WAKEUP_PIN);

    setPinInputHigh(NRF_BOOT_PIN);

    setPinOutput(NRF_RESET_PIN); writePinLow(NRF_RESET_PIN);
    wait_ms(50);
    writePinHigh(NRF_RESET_PIN);

    setPinInputHigh(DEV_MODE_PIN);
#endif
    setPinInputHigh(SYS_MODE_PIN);

    // open power
    setPinOutput(DC_BOOST_PIN);
    writePinHigh(DC_BOOST_PIN);

    setPinOutput(DRIVER_LED_CS_PIN);
    writePinLow(DRIVER_LED_CS_PIN);

    setPinOutput(DRIVER_SIDE_CS_PIN);
    writePinLow(DRIVER_SIDE_CS_PIN);
}

/**
 * @brief  long press key process.
 */
void long_press_key(void)
{
    static uint32_t long_press_timer = 0;

    if (timer_elapsed32(long_press_timer) < 100) return;
    long_press_timer = timer_read32();
    if (f_rf_sw_press) {
        rf_sw_press_delay++;
        if (rf_sw_press_delay >= RF_LONG_PRESS_DELAY)  {
            f_rf_sw_press = 0;
            dev_info.link_mode   = rf_sw_temp;
            dev_info.rf_channel  = rf_sw_temp;
            dev_info.ble_channel = rf_sw_temp;
            uint8_t timeout = 5;
            while (timeout--) {
                uart_send_cmd(CMD_NEW_ADV, 0, 1);
                wait_ms(20);
                if (f_rf_new_adv_ok) break;
            }
        }
    } else {
        rf_sw_press_delay = 0;
    }
    if (f_dev_reset_press) {
        dev_reset_press_delay++;
        if (dev_reset_press_delay >= DEV_RESET_PRESS_DELAY)  {
            f_dev_reset_press = 0;
#if(WORK_MODE == THREE_MODE)
            if (dev_info.link_mode != LINK_USB) {
                if (dev_info.link_mode != LINK_RF_24) {
                    dev_info.link_mode      = LINK_BT_1;
                    dev_info.ble_channel    = LINK_BT_1;
                    dev_info.rf_channel     = LINK_BT_1;
                }
            } else {
                dev_info.ble_channel = LINK_BT_1;
                dev_info.rf_channel  = LINK_BT_1;
            }

            uart_send_cmd(CMD_SET_LINK, 10, 10);
            wait_ms(500);
            uart_send_cmd(CMD_CLR_DEVICE, 10, 10);
#endif
            eeconfig_init();
            device_reset_show();
            device_reset_init();

            keymap_config.no_gui = 0;
            f_win_lock = 0;

            if (dev_info.sys_sw_state == SYS_SW_MAC) {
                default_layer_set(1 << 0);
                keymap_config.nkro = 0;
            } else {
                default_layer_set(1 << 2);
                keymap_config.nkro = 1;
            }
        }
    } else {
        dev_reset_press_delay = 0;
    }


    if (f_rgb_test_press) {
        rgb_test_press_delay++;
        if (rgb_test_press_delay >= RGB_TEST_PRESS_DELAY) {
            f_rgb_test_press = 0;
            rgb_test_show();
        }
    } else {
        rgb_test_press_delay = 0;
    }
}

/**
 * @brief  Release all keys, clear keyboard report.
 */
void m_break_all_key(void)
{
    uint8_t report_buf[16];
    bool nkro_temp = keymap_config.nkro;

    clear_weak_mods();
    clear_mods();
    clear_keyboard();

    keymap_config.nkro = 1;
    memset(nkro_report, 0, sizeof(report_nkro_t));
    host_nkro_send(nkro_report);
    wait_ms(10);

    keymap_config.nkro = 0;
    memset(keyboard_report, 0, sizeof(report_keyboard_t));
    host_keyboard_send(keyboard_report);
    wait_ms(10);

    keymap_config.nkro = nkro_temp;


    if (dev_info.link_mode != LINK_USB) {
        memset(report_buf, 0, 16);
        uart_send_report(CMD_RPT_BIT_KB, report_buf, 16);
        wait_ms(10);
        uart_send_report(CMD_RPT_BYTE_KB, report_buf, 8);
        wait_ms(10);
    }

    memset(uart_bit_report_buf, 0, sizeof(uart_bit_report_buf));
    memset(bitkb_report_buf, 0, sizeof(bitkb_report_buf));
    memset(bytekb_report_buf, 0, sizeof(bytekb_report_buf));
}

/**
 * @brief  switch device link mode.
 * @param mode : link mode
 */
#if(WORK_MODE == THREE_MODE)
static void switch_dev_link(uint8_t mode) {
    if (mode > LINK_USB) return;

    m_break_all_key();

    dev_info.link_mode = mode;

    dev_info.rf_state = RF_IDLE;
    f_send_channel    = 1;

    if (mode == LINK_USB) {
        host_mode = HOST_USB_TYPE;
        host_set_driver(m_host_driver);
        rf_link_show_time = 0;
    } else {
        host_mode = HOST_RF_TYPE;

        host_set_driver(&rf_host_driver);
    }
}

#endif

/**
 * @brief  scan dial switch.
 */
void dial_sw_scan(void)
{
    uint8_t dial_scan               = 0;
    static uint8_t dial_save        = 0xf0;
    static uint8_t debounce         = 0;
    static uint32_t dial_scan_timer = 0;
    static bool flag_power_on       = 1;

    if (!flag_power_on) {
        if (timer_elapsed32(dial_scan_timer) < 20) return;
    }
    dial_scan_timer = timer_read32();

#if(WORK_MODE == THREE_MODE)
    setPinInputHigh(DEV_MODE_PIN);
#endif
    setPinInputHigh(SYS_MODE_PIN);
#if(WORK_MODE == THREE_MODE)
    if (readPin(DEV_MODE_PIN)) dial_scan |= 0X01;
#endif
    if (readPin(SYS_MODE_PIN)) dial_scan |= 0X02;

    if (dial_save != dial_scan) {
        m_break_all_key();
        dial_save         = dial_scan;
        no_act_time         = 0;
        rf_linking_time     = 0;
        debounce            = 25;
        f_dial_sw_init_ok   = 0;
        return;
    } else if (debounce) {
        debounce--;
        return;
    }

#if(WORK_MODE == THREE_MODE)

    if (dial_scan & 0x01) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

#endif

    if (dial_scan & 0x02) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            f_sys_show = 1;
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            f_win_lock            = keymap_config.no_gui;
            m_break_all_key();
        }
        keymap_config.nkro   = 0;
        keymap_config.no_gui = 0;
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            f_sys_show = 1;
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.no_gui  = f_win_lock;
            m_break_all_key();
        }
        keymap_config.nkro = 1;
    }

    if (f_dial_sw_init_ok == 0) {
        f_dial_sw_init_ok = 1;
        flag_power_on     = 0;

#if(WORK_MODE == THREE_MODE)

        if (dev_info.link_mode != LINK_USB) {
            host_set_driver(&rf_host_driver);
        }
#endif
    }
}

/**
 * @brief  power on scan dial switch.
 */
void m_power_on_dial_sw_scan(void)
{
    uint8_t dial_scan_dev = 0;
    uint8_t dial_scan_sys = 0;
    uint8_t dial_check_dev = 0;
    uint8_t dial_check_sys = 0;
    uint8_t debounce = 0;

    f_win_lock = 0;
#if(WORK_MODE == THREE_MODE)
    setPinInputHigh(DEV_MODE_PIN);
#endif
    setPinInputHigh(SYS_MODE_PIN);

    for(debounce=0; debounce<10; debounce++) {
        dial_scan_dev = 0;
        dial_scan_sys = 0;
#if(WORK_MODE == THREE_MODE)
        if (readPin(DEV_MODE_PIN))  dial_scan_dev = 0x01;
        else                        dial_scan_dev = 0;
#endif
        if (readPin(SYS_MODE_PIN))  dial_scan_sys = 0x01;
        else                        dial_scan_sys = 0;
        if((dial_scan_dev != dial_check_dev)||(dial_scan_sys != dial_check_sys))
        {
            dial_check_dev = dial_scan_dev;
            dial_check_sys = dial_scan_sys;
            debounce = 0;
        }
        wait_ms(1);
    }
#if(WORK_MODE == THREE_MODE)
    if (dial_scan_dev) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }
#endif
    if (dial_scan_sys) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            f_win_lock            = keymap_config.no_gui;
            keymap_config.no_gui  = 0;
            m_break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            m_break_all_key();
        }
    }
}

/* qmk process record */
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if(!process_record_user(keycode, record)){
        return false;
    }
    no_act_time = 0;
    
    switch (keycode) {
        case RF_DFU:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) return false;
                uart_send_cmd(CMD_RF_DFU, 10, 20);
            }
            return false;

        case LNK_USB:
            if (record->event.pressed) {
                m_break_all_key();
            } else {
                dev_info.link_mode = LINK_USB;
                uart_send_cmd(CMD_SET_LINK, 10, 10);
                rf_blink_cnt = 3;
            }
            return false;

        case LNK_RF:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_RF_24;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE1:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_1;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE2:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_2;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE3:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_3;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case MAC_TASK:
            if (record->event.pressed) {
                host_consumer_send(0x029F);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_SEARCH:
            if (record->event.pressed) {
                register_code(KC_LGUI);
                register_code(KC_SPACE);
                wait_ms(50);
                unregister_code(KC_LGUI);
                unregister_code(KC_SPACE);
            }
            return false;

        case MAC_VOICE:
            if (record->event.pressed) {
                host_consumer_send(0xcf);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_CONSOLE:
            if (record->event.pressed) {
                host_consumer_send(0x02A0);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_DND:
            if (record->event.pressed) {
                host_system_send(0x9b);
            } else {
                host_system_send(0);
            }
            return false;

        case SIDE_VAI:
            if (record->event.pressed) {
                light_level_control(1);
            }
            return false;

        case SIDE_VAD:
            if (record->event.pressed) {
                light_level_control(0);
            }
            return false;

        case SIDE_MOD:
            if (record->event.pressed) {
                side_mode_control(1);
            }
            return false;

        case SIDE_HUI:
            if (record->event.pressed) {
                side_colour_control(1);
            }
            return false;

        case SIDE_SPI:
            if (record->event.pressed) {
                light_speed_control(1);
            }
            return false;

        case SIDE_SPD:
            if (record->event.pressed) {
                light_speed_control(0);
            }
            return false;

        case LOGO_VAI:
            if (record->event.pressed) {
                logo_light_level_control(1);
            }
            return false;
        case LOGO_VAD:
            if (record->event.pressed) {
                logo_light_level_control(0);
            }
            return false;
        case LOGO_MOD:
            if (record->event.pressed) {
                logo_side_mode_control(1);
            }
            return false;
        case LOGO_HUI:
            if (record->event.pressed) {
                logo_side_colour_control(1);
            }
            return false;
        case LOGO_SPI:
            if (record->event.pressed) {
                logo_light_speed_control(1);
            }
            return false;
        case LOGO_SPD:
            if (record->event.pressed) {
                logo_light_speed_control(0);
            }
            return false;

        case DEV_RESET:
            if (record->event.pressed) {
                f_dev_reset_press = 1;
                m_break_all_key();
            } else {
                f_dev_reset_press = 0;
            }
            return false;

        case SLEEP_MODE:
            if (record->event.pressed) {
                f_dev_sleep_enable = !f_dev_sleep_enable;
                f_sleep_show       = 1;
                eeconfig_update_user_datablock(&user_config);
            }
            return false;

        case BAT_SHOW:
            if (record->event.pressed) {
                f_bat_hold = !f_bat_hold;
            }
            return false;

        case WIN_LOCK:
            if (record->event.pressed) {
            keymap_config.no_gui = !keymap_config.no_gui;
            eeconfig_update_keymap(keymap_config.raw);
            m_break_all_key();
            } else  unregister_code16(keycode);
            break;

        case RGB_TEST:
            if (record->event.pressed) {
                f_rgb_test_press = 1;
            } else {
                f_rgb_test_press = 0;
            }
            return false;

        case SHIFT_GRV:
            if (record->event.pressed) {
                register_code(KC_LSFT);
                register_code(KC_GRV);
            }
            else {
                unregister_code(KC_LSFT);
                unregister_code(KC_GRV);
            }
            return false;

        case RGB_VAI:

            if(rgb_matrix_config.hsv.v == RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2) {
                if(dev_info.link_mode == LINK_USB) {
                   rgb_matrix_increase_val(); 
                } 
                return true;
            }
            else if(rgb_matrix_config.hsv.v == RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP) {
                return false;
            }
            return true;

        case RGB_VAD:
            if(rgb_matrix_config.hsv.v == RGB_MATRIX_MAXIMUM_BRIGHTNESS) {
                rgb_matrix_decrease_val(); 
            }
            return true;

        default:
            return true;
    }

    return true;
}

/**
 * @brief  timer process.
 */
void timer_pro(void) {
    static uint32_t interval_timer = 0;
    static bool     f_first        = true;

    if (f_first) {
        f_first        = false;
        interval_timer = timer_read32();
        m_host_driver  = host_get_driver();
    }

    if (timer_elapsed32(interval_timer) < 10)
        return;
    else
        interval_timer = timer_read32();

    if (rf_link_show_time < RF_LINK_SHOW_TIME) rf_link_show_time++;

    if (no_act_time < 0xffff) no_act_time++;

    if (rf_linking_time < 0xffff) rf_linking_time++;

#if(WORK_MODE == THREE_MODE)
    if (rf_linking_time < 0xffff)
        rf_linking_time++;
#endif
}

/**
 * @brief  londing eeprom data.
 */
void m_londing_eeprom_data(void)
{
    eeconfig_read_user_datablock(&user_config);
    if (user_config.default_brightness_flag != 0xA5) {
        rgb_matrix_sethsv(  RGB_DEFAULT_COLOUR,
                            255,
                            RGB_MATRIX_MAXIMUM_BRIGHTNESS - RGB_MATRIX_VAL_STEP * 2);
        user_config.default_brightness_flag = 0xA5;
        user_config.ee_side_mode            = side_mode;
        user_config.ee_side_light           = side_light;
        user_config.ee_side_speed           = side_speed;
        user_config.ee_side_rgb             = side_rgb;
        user_config.ee_side_colour          = side_colour;
        user_config.ee_logo_mode            = logo_mode;
        user_config.ee_logo_light           = logo_light;
        user_config.ee_logo_speed           = logo_speed;
        user_config.ee_logo_rgb             = logo_rgb;
        user_config.ee_logo_colour          = logo_colour;
#if(WORK_MODE == THREE_MODE)
        f_dev_sleep_enable                  = 1;
#else
        f_dev_sleep_enable                  = 0;
#endif
        eeconfig_update_user_datablock(&user_config);
    } else {
        side_mode   = user_config.ee_side_mode;
        side_light  = user_config.ee_side_light;
        side_speed  = user_config.ee_side_speed;
        side_rgb    = user_config.ee_side_rgb;
        side_colour = user_config.ee_side_colour;
        logo_mode   = user_config.ee_logo_mode;
        logo_light  = user_config.ee_logo_light;
        logo_speed  = user_config.ee_logo_speed;
        logo_rgb    = user_config.ee_logo_rgb;
        logo_colour = user_config.ee_logo_colour;
    }

}

/* qmk keyboard post init */
void keyboard_post_init_kb(void)
{
    m_gpio_init();
#if(WORK_MODE == THREE_MODE)
    rf_uart_init();
    wait_ms(500);
    rf_device_init();
#endif

    m_break_all_key();
    m_londing_eeprom_data();
    m_power_on_dial_sw_scan();
    keyboard_post_init_user();

#if(WORK_MODE == USB_MODE)
    rf_link_show_time = 0;
#endif
}


bool rgb_matrix_indicators_kb(void)
{
    if(!rgb_matrix_indicators_user()){
        return false;
    }
    return true;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
    if (keymap_config.no_gui) {
        rgb_matrix_set_color(16, 0x00, 0x80, 0x00);
    }

    rgb_matrix_set_color(RGB_MATRIX_LED_COUNT-1, 0, 0, 0);
    return true;
}

/* qmk housekeeping task */
void housekeeping_task_kb(void)
{
    timer_pro();

#if(WORK_MODE == THREE_MODE)
    uart_receive_pro();

    uart_send_report_func();

    dev_sts_sync();

#endif

    long_press_key();

    dial_sw_scan();

    flash_data_manage();

    m_side_led_show();

    Sleep_Handle();

}
