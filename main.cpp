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
/* EXAMPLE_START(hog_gamepad_demo): HID Gamepad LE
 */
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "hog_keyboard_demo.h"

#include "btstack.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"

#include "btstack_run_loop.h"
#include "pico/stdlib.h"
#include "btstack_event.h"
#include "hal_led.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/btstack_chipset_cyw43.h"
#include "pico/btstack_hci_transport_cyw43.h"
#include "pico/btstack_run_loop_async_context.h"

// from USB HID Specification for Xbox Controller
const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    
    // Buttons (16 buttons)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x10,        //   Usage Maximum (0x10)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x95, 0x10,        //   Report Count (16)
    0x75, 0x01,        //   Report Size (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Left Stick X & Y
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x15, 0x81,        //   Logical Minimum (-127)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Right Stick X & Y
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x81,        //   Logical Minimum (-127)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Left and Right Triggers
    0x05, 0x02,        //   Usage Page (Sim Ctrls)
    0x09, 0xC5,        //   Usage (Brake)
    0x09, 0xC4,        //   Usage (Accelerator)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0xFF,        //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Hat Switch (D-Pad)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x66, 0x14, 0x00,  //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    // Padding
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    
    0xC0,              // End Collection
};

//
// Xbox Controller Button Definitions
#define XBOX_BUTTON_A      0x0001
#define XBOX_BUTTON_B      0x0002
#define XBOX_BUTTON_X      0x0004
#define XBOX_BUTTON_Y      0x0008
#define XBOX_BUTTON_LB     0x0010
#define XBOX_BUTTON_RB     0x0020
#define XBOX_BUTTON_BACK   0x0040
#define XBOX_BUTTON_START  0x0080
#define XBOX_BUTTON_LS     0x0100
#define XBOX_BUTTON_RS     0x0200
#define XBOX_BUTTON_GUIDE  0x0400

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

// Xbox Controller Report Structure
typedef struct {
    uint16_t buttons;     // Button states (16 buttons)
    int8_t left_x;        // Left stick X (-127 to 127)
    int8_t left_y;        // Left stick Y (-127 to 127) 
    int8_t right_x;       // Right stick X (-127 to 127)
    int8_t right_y;       // Right stick Y (-127 to 127)
    uint8_t left_trigger; // Left trigger (0-255)
    uint8_t right_trigger;// Right trigger (0-255)
    uint8_t dpad;         // D-pad direction (0-7, 8=neutral)
} xbox_controller_report_t;

// static btstack_timer_source_t heartbeat;
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
    // Name
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

    // setup SM: Display only
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();

    // setup HID Device service
    hids_device_init(0, hid_descriptor_gamepad, sizeof(hid_descriptor_gamepad));

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
    gap_advertisements_enable(1);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // register for HIDS
    hids_device_register_packet_handler(packet_handler);
}

// Gamepad Report sending
static void send_gamepad_report(xbox_controller_report_t *report)
{
    uint8_t hid_report[9];
    hid_report[0] = report->buttons & 0xFF;        // Low byte of buttons
    hid_report[1] = (report->buttons >> 8) & 0xFF; // High byte of buttons
    hid_report[2] = report->left_x;                // Left stick X
    hid_report[3] = report->left_y;                // Left stick Y
    hid_report[4] = report->right_x;               // Right stick X
    hid_report[5] = report->right_y;               // Right stick Y
    hid_report[6] = report->left_trigger;          // Left trigger
    hid_report[7] = report->right_trigger;         // Right trigger
    hid_report[8] = report->dpad;                  // D-pad
    
    switch (protocol_mode)
    {
    case 0:
        hids_device_send_boot_keyboard_input_report(con_handle, hid_report, sizeof(hid_report));
        break;
    case 1:
        hids_device_send_input_report(con_handle, hid_report, sizeof(hid_report));
        break;
    default:
        break;
    }
}

// Demo Application

// On embedded systems, send constant demo gamepad inputs with fixed period

#define DEMO_PERIOD_MS 100

static int demo_step;
static btstack_timer_source_t demo_timer;
static xbox_controller_report_t current_report;

static void send_gamepad_input(xbox_controller_report_t *report)
{
    memcpy(&current_report, report, sizeof(xbox_controller_report_t));
    hids_device_request_can_send_now_event(con_handle);
}

static void gamepad_can_send_now(void)
{
    send_gamepad_report(&current_report);
}

static void demo_timer_handler(btstack_timer_source_t *ts)
{
    xbox_controller_report_t report = {0};
    
    // Extended demo sequence: cycle through all 16 buttons and all axes
    switch (demo_step % 36) {
        // Individual buttons (0-15)
        case 0:
            report.buttons = XBOX_BUTTON_A;
            printf("Demo: A Button (Button 1)\n");
            break;
        case 1:
            report.buttons = XBOX_BUTTON_B;
            printf("Demo: B Button (Button 2)\n");
            break;
        case 2:
            report.buttons = XBOX_BUTTON_X;
            printf("Demo: X Button (Button 3)\n");
            break;
        case 3:
            report.buttons = XBOX_BUTTON_Y;
            printf("Demo: Y Button (Button 4)\n");
            break;
        case 4:
            report.buttons = XBOX_BUTTON_LB;
            printf("Demo: Left Bumper (Button 5)\n");
            break;
        case 5:
            report.buttons = XBOX_BUTTON_RB;
            printf("Demo: Right Bumper (Button 6)\n");
            break;
        case 6:
            report.buttons = XBOX_BUTTON_BACK;
            printf("Demo: Back Button (Button 7)\n");
            break;
        case 7:
            report.buttons = XBOX_BUTTON_START;
            printf("Demo: Start Button (Button 8)\n");
            break;
        case 8:
            report.buttons = XBOX_BUTTON_LS;
            printf("Demo: Left Stick Button (Button 9)\n");
            break;
        case 9:
            report.buttons = XBOX_BUTTON_RS;
            printf("Demo: Right Stick Button (Button 10)\n");
            break;
        case 10:
            report.buttons = XBOX_BUTTON_GUIDE;
            printf("Demo: Guide Button (Button 11)\n");
            break;
        case 11:
            report.buttons = 0x0800; // Button 12
            printf("Demo: Button 12\n");
            break;
        case 12:
            report.buttons = 0x1000; // Button 13
            printf("Demo: Button 13\n");
            break;
        case 13:
            report.buttons = 0x2000; // Button 14
            printf("Demo: Button 14\n");
            break;
        case 14:
            report.buttons = 0x4000; // Button 15
            printf("Demo: Button 15\n");
            break;
        case 15:
            report.buttons = 0x8000; // Button 16
            printf("Demo: Button 16\n");
            break;
            
        // Left stick axes
        case 16:
            report.left_x = -127;
            printf("Demo: Left Stick X - Full Left\n");
            break;
        case 17:
            report.left_x = 127;
            printf("Demo: Left Stick X - Full Right\n");
            break;
        case 18:
            report.left_y = -127;
            printf("Demo: Left Stick Y - Full Up\n");
            break;
        case 19:
            report.left_y = 127;
            printf("Demo: Left Stick Y - Full Down\n");
            break;
            
        // Right stick axes
        case 20:
            report.right_x = -127;
            printf("Demo: Right Stick X - Full Left\n");
            break;
        case 21:
            report.right_x = 127;
            printf("Demo: Right Stick X - Full Right\n");
            break;
        case 22:
            report.right_y = -127;
            printf("Demo: Right Stick Y - Full Up\n");
            break;
        case 23:
            report.right_y = 127;
            printf("Demo: Right Stick Y - Full Down\n");
            break;
            
        // Triggers
        case 24:
            report.left_trigger = 255;
            printf("Demo: Left Trigger - Full Press\n");
            break;
        case 25:
            report.right_trigger = 255;
            printf("Demo: Right Trigger - Full Press\n");
            break;
            
        // D-pad directions
        case 26:
            report.dpad = DPAD_UP;
            printf("Demo: D-Pad Up\n");
            break;
        case 27:
            report.dpad = DPAD_UP_RIGHT;
            printf("Demo: D-Pad Up-Right\n");
            break;
        case 28:
            report.dpad = DPAD_RIGHT;
            printf("Demo: D-Pad Right\n");
            break;
        case 29:
            report.dpad = DPAD_DOWN_RIGHT;
            printf("Demo: D-Pad Down-Right\n");
            break;
        case 30:
            report.dpad = DPAD_DOWN;
            printf("Demo: D-Pad Down\n");
            break;
        case 31:
            report.dpad = DPAD_DOWN_LEFT;
            printf("Demo: D-Pad Down-Left\n");
            break;
        case 32:
            report.dpad = DPAD_LEFT;
            printf("Demo: D-Pad Left\n");
            break;
        case 33:
            report.dpad = DPAD_UP_LEFT;
            printf("Demo: D-Pad Up-Left\n");
            break;
            
        // Combination tests
        case 34:
            report.buttons = XBOX_BUTTON_A | XBOX_BUTTON_B | XBOX_BUTTON_X | XBOX_BUTTON_Y;
            report.left_trigger = 127;
            report.right_trigger = 127;
            printf("Demo: Multiple Buttons + Half Triggers\n");
            break;
        case 35:
            // All neutral
            report.dpad = DPAD_NEUTRAL;
            printf("Demo: All Neutral\n");
            break;
    }
    
    demo_step++;
    send_gamepad_input(&report);

    // set next timer
    btstack_run_loop_set_timer(ts, DEMO_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

static void hid_embedded_start_demo(void)
{
    printf("Start gamepad demo..\n");

    demo_step = 0;
    memset(&current_report, 0, sizeof(current_report));
    current_report.dpad = DPAD_NEUTRAL;
    
    // set one-shot timer
    demo_timer.process = &demo_timer_handler;
    btstack_run_loop_set_timer(&demo_timer, DEMO_PERIOD_MS);
    btstack_run_loop_add_timer(&demo_timer);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return;

    switch (hci_event_packet_get_type(packet))
    {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        con_handle = HCI_CON_HANDLE_INVALID;
        printf("Disconnected\n");
        break;
    case SM_EVENT_JUST_WORKS_REQUEST:
        printf("Just Works requested\n");
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        printf("Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
        sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        printf("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
        break;
    case HCI_EVENT_HIDS_META:
        switch (hci_event_hids_meta_get_subevent_code(packet))
        {
        case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
            printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));
            hid_embedded_start_demo();
            break;
        case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
            printf("Boot Gamepad Characteristic Subscribed %u\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_PROTOCOL_MODE:
            protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
            printf("Protocol Mode: %s mode\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
            break;
        case HIDS_SUBEVENT_CAN_SEND_NOW:
            gamepad_can_send_now();
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}

int btstack_main(void);
int btstack_main(void)
{
    le_gamepad_setup();

    // turn on!
    hci_power_control(HCI_POWER_ON);

    return 0;
}

int picow_bt_example_init(void)
{
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init())
    {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }
    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    return 0;
}
/* EXAMPLE_END */
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
    
    btstack_main();
    btstack_run_loop_execute();
}