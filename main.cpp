/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

// *****************************************************************************
// Generic Bluetooth LE Gamepad for Raspberry Pi Pico W
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "btstack.h"
#include "hog_keyboard_demo.h"
#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Windows-compatible HID Gamepad Descriptor
const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    
    // Buttons (12 buttons - standard gamepad layout)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0C,        //   Usage Maximum (0x0C) - 12 buttons
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x95, 0x0C,        //   Report Count (12)
    0x75, 0x01,        //   Report Size (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Padding for buttons (4 bits to align to byte boundary)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x04,        //   Report Size (4)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Left Stick X & Y
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Right Stick X & Y
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Left and Right Triggers
    0x05, 0x02,        //   Usage Page (Sim Ctrls)
    0x09, 0xC5,        //   Usage (Brake)
    0x09, 0xC4,        //   Usage (Accelerator)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Hat Switch (D-Pad)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Degree)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Padding for hat switch
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x04,        //   Report Size (4)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    0xC0,              // End Collection
};

//
// Generic Gamepad Button Definitions
#define GAMEPAD_BUTTON_1       0x0001  // Face Button 1 (A/Cross)
#define GAMEPAD_BUTTON_2       0x0002  // Face Button 2 (B/Circle)  
#define GAMEPAD_BUTTON_3       0x0004  // Face Button 3 (X/Square)
#define GAMEPAD_BUTTON_4       0x0008  // Face Button 4 (Y/Triangle)
#define GAMEPAD_BUTTON_L1      0x0010  // Left Shoulder Button
#define GAMEPAD_BUTTON_R1      0x0020  // Right Shoulder Button
#define GAMEPAD_BUTTON_SELECT  0x0040  // Select/Back Button
#define GAMEPAD_BUTTON_START   0x0080  // Start/Menu Button
#define GAMEPAD_BUTTON_L3      0x0100  // Left Stick Click
#define GAMEPAD_BUTTON_R3      0x0200  // Right Stick Click
#define GAMEPAD_BUTTON_HOME    0x0400  // Home/Guide Button
#define GAMEPAD_BUTTON_EXTRA   0x0800  // Extra Button (12th button)

// D-Pad directions
#define DPAD_UP           0
#define DPAD_UP_RIGHT     1
#define DPAD_RIGHT        2
#define DPAD_DOWN_RIGHT   3
#define DPAD_DOWN         4
#define DPAD_DOWN_LEFT    5
#define DPAD_LEFT         6
#define DPAD_UP_LEFT      7
#define DPAD_NEUTRAL      8

// Generic Gamepad Report Structure (Windows-compatible)
typedef struct {
    uint16_t buttons;     // Button states (12 buttons used)
    int16_t left_x;       // Left stick X (-32768 to 32767)
    int16_t left_y;       // Left stick Y (-32768 to 32767) 
    int16_t right_x;      // Right stick X (-32768 to 32767)
    int16_t right_y;      // Right stick Y (-32768 to 32767)
    uint8_t left_trigger; // Left trigger (0-255)
    uint8_t right_trigger;// Right trigger (0-255)
    uint8_t dpad;         // D-pad direction (0-7, 8=neutral)
} gamepad_report_t;

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    0x06,
    // Name - "BT Gamepad" (shorter name for advertising space)
    0x0b,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'B',
    'T',
    ' ',
    'G',
    'a',
    'm',
    'e',
    'p',
    'a',
    'd',
    // 16-bit Service UUIDs
    0x03,
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Gamepad (Category 15, Sub-Category 4)
    0x03,
    BLUETOOTH_DATA_TYPE_APPEARANCE,
    0xC4,
    0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

static void le_gamepad_setup(void)
{
    l2cap_init();

    // Setup Security Manager
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // Setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // Setup services
    battery_service_server_init(battery);
    device_information_service_server_init();
    hids_device_init(0, hid_descriptor_gamepad, sizeof(hid_descriptor_gamepad));

    // Setup advertisements
    uint16_t adv_int_min = 0x0020;  // 20ms
    uint16_t adv_int_max = 0x0040;  // 40ms
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
    gap_advertisements_enable(1);

    // Register event handlers
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);
    hids_device_register_packet_handler(packet_handler);
}

// Send HID gamepad report
static void send_gamepad_report(gamepad_report_t *report)
{
    uint8_t hid_report[13];
    
    // Buttons: 12 bits + 4 bits padding = 2 bytes
    hid_report[0] = report->buttons & 0xFF;         // Low 8 bits of buttons
    hid_report[1] = (report->buttons >> 8) & 0x0F; // High 4 bits of buttons + 4 bits padding
    
    // Left stick: 2 × 16-bit values = 4 bytes
    hid_report[2] = report->left_x & 0xFF;
    hid_report[3] = (report->left_x >> 8) & 0xFF;
    hid_report[4] = report->left_y & 0xFF;
    hid_report[5] = (report->left_y >> 8) & 0xFF;
    
    // Right stick: 2 × 16-bit values = 4 bytes
    hid_report[6] = report->right_x & 0xFF;
    hid_report[7] = (report->right_x >> 8) & 0xFF;
    hid_report[8] = report->right_y & 0xFF;
    hid_report[9] = (report->right_y >> 8) & 0xFF;
    
    // Triggers: 2 × 8-bit values = 2 bytes
    hid_report[10] = report->left_trigger;
    hid_report[11] = report->right_trigger;
    
    // D-pad: 4 bits + 4 bits padding = 1 byte
    hid_report[12] = report->dpad & 0x0F;
    
    // Debug output for button presses
    if (report->buttons != 0) {
        printf("Sending buttons: 0x%04X (bytes: 0x%02X 0x%02X)\n", 
               report->buttons, hid_report[0], hid_report[1]);
    }
    
    if (protocol_mode) {
        hids_device_send_input_report(con_handle, hid_report, sizeof(hid_report));
    } else {
        hids_device_send_boot_keyboard_input_report(con_handle, hid_report, sizeof(hid_report));
    }
}

// Demo functionality
#define DEMO_PERIOD_MS 100

static int demo_step;
static btstack_timer_source_t demo_timer;
static gamepad_report_t current_report;

static void send_gamepad_input(gamepad_report_t *report)
{
    current_report = *report;
    printf("Requesting send for buttons: 0x%04X\n", report->buttons);
    hids_device_request_can_send_now_event(con_handle);
}

static void gamepad_can_send_now(void)
{
    send_gamepad_report(&current_report);
}

static void demo_timer_handler(btstack_timer_source_t *ts)
{
    gamepad_report_t report = {0};
    
    printf("Demo step %d: ", demo_step % 32);
    
    // Demo sequence: cycle through buttons, axes, triggers, and d-pad
    switch (demo_step % 32) {
        // Test all 12 buttons individually
        case 0:  report.buttons = GAMEPAD_BUTTON_1; printf("Button 1 (A/Cross)\n"); break;
        case 1:  report.buttons = GAMEPAD_BUTTON_2; printf("Button 2 (B/Circle)\n"); break;
        case 2:  report.buttons = GAMEPAD_BUTTON_3; printf("Button 3 (X/Square)\n"); break;
        case 3:  report.buttons = GAMEPAD_BUTTON_4; printf("Button 4 (Y/Triangle)\n"); break;
        case 4:  report.buttons = GAMEPAD_BUTTON_L1; printf("Left Shoulder (L1)\n"); break;
        case 5:  report.buttons = GAMEPAD_BUTTON_R1; printf("Right Shoulder (R1)\n"); break;
        case 6:  report.buttons = GAMEPAD_BUTTON_SELECT; printf("Select/Back\n"); break;
        case 7:  report.buttons = GAMEPAD_BUTTON_START; printf("Start/Menu\n"); break;
        case 8:  report.buttons = GAMEPAD_BUTTON_L3; printf("Left Stick Click (L3)\n"); break;
        case 9:  report.buttons = GAMEPAD_BUTTON_R3; printf("Right Stick Click (R3)\n"); break;
        case 10: report.buttons = GAMEPAD_BUTTON_HOME; printf("Home/Guide\n"); break;
        case 11: report.buttons = GAMEPAD_BUTTON_EXTRA; printf("Extra Button\n"); break;
            
        // Test analog sticks
        case 12: report.left_x = -32767; printf("Demo: Left Stick X - Full Left\n"); break;
        case 13: report.left_x = 32767; printf("Demo: Left Stick X - Full Right\n"); break;
        case 14: report.left_y = -32767; printf("Demo: Left Stick Y - Full Up\n"); break;
        case 15: report.left_y = 32767; printf("Demo: Left Stick Y - Full Down\n"); break;
        case 16: report.right_x = -32767; printf("Demo: Right Stick X - Full Left\n"); break;
        case 17: report.right_x = 32767; printf("Demo: Right Stick X - Full Right\n"); break;
        case 18: report.right_y = -32767; printf("Demo: Right Stick Y - Full Up\n"); break;
        case 19: report.right_y = 32767; printf("Demo: Right Stick Y - Full Down\n"); break;
            
        // Test triggers
        case 20: report.left_trigger = 255; printf("Demo: Left Trigger - Full Press\n"); break;
        case 21: report.right_trigger = 255; printf("Demo: Right Trigger - Full Press\n"); break;
            
        // Test D-pad
        case 22: report.dpad = DPAD_UP; printf("Demo: D-Pad Up\n"); break;
        case 23: report.dpad = DPAD_UP_RIGHT; printf("Demo: D-Pad Up-Right\n"); break;
        case 24: report.dpad = DPAD_RIGHT; printf("Demo: D-Pad Right\n"); break;
        case 25: report.dpad = DPAD_DOWN_RIGHT; printf("Demo: D-Pad Down-Right\n"); break;
        case 26: report.dpad = DPAD_DOWN; printf("Demo: D-Pad Down\n"); break;
        case 27: report.dpad = DPAD_DOWN_LEFT; printf("Demo: D-Pad Down-Left\n"); break;
        case 28: report.dpad = DPAD_LEFT; printf("Demo: D-Pad Left\n"); break;
        case 29: report.dpad = DPAD_UP_LEFT; printf("Demo: D-Pad Up-Left\n"); break;
            
        // Test combinations
        case 30:
            report.buttons = GAMEPAD_BUTTON_1 | GAMEPAD_BUTTON_2 | GAMEPAD_BUTTON_3 | GAMEPAD_BUTTON_4;
            report.left_trigger = 127; report.right_trigger = 127;
            printf("Demo: Multiple Buttons + Half Triggers\n");
            break;
        case 31: report.dpad = DPAD_NEUTRAL; printf("Demo: All Neutral\n"); break;
    }
    
    demo_step++;
    send_gamepad_input(&report);
    btstack_run_loop_set_timer(ts, DEMO_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

static void start_demo(void)
{
    printf("Starting gamepad demo...\n");
    demo_step = 0;
    memset(&current_report, 0, sizeof(current_report));
    current_report.dpad = DPAD_NEUTRAL;
    
    demo_timer.process = &demo_timer_handler;
    btstack_run_loop_set_timer(&demo_timer, DEMO_PERIOD_MS);
    btstack_run_loop_add_timer(&demo_timer);
    printf("Demo timer started\n");
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected\n");
            break;
            
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just Works authentication requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
            
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            printf("Numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
            
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
            
        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)) {
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    printf("Input report subscribed: %u\n", hids_subevent_input_report_enable_get_enable(packet));
                    start_demo();
                    break;
                    
                case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
                    printf("Boot report subscribed: %u\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
                    break;
                    
                case HIDS_SUBEVENT_PROTOCOL_MODE:
                    protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
                    printf("Protocol mode: %s\n", protocol_mode ? "Report" : "Boot");
                    break;
                    
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    gamepad_can_send_now();
                    break;
            }
            break;
    }
}

int main()
{
    stdio_init_all();

    // Initialize CYW43 architecture
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43\n");
        return -1;
    }
    
    // Enable Bluetooth
    cyw43_arch_enable_sta_mode();

    btstack_memory_init();
    
    // Setup and start gamepad
    le_gamepad_setup();
    hci_power_control(HCI_POWER_ON);
    
    btstack_run_loop_execute();
    return 0;
}