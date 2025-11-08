/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
/**
 * @file        main.c
 * @brief       I2C + SSD1306 OLED + MAX30102
 */
/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "i2c.h"
#include <max30102.h>
#include <algorithm.h>
#include "dma.h"
#include "gpio.h"
#include "rtc.h"
#include "nvic_table.h"
#ifdef BOARD_EVKIT_V1
#warning This example is not supported by the standard EvKit.
#endif

// Button Pin Definitions
#define SW1_PORT MXC_GPIO0
#define SW1_PIN MXC_GPIO_PIN_2
#define SW2_PORT MXC_GPIO1
#define SW2_PIN MXC_GPIO_PIN_7
#define SW3_PORT MXC_GPIO3
#define SW3_PIN MXC_GPIO_PIN_1
// PMIC LED
#define PMIC_I2C        MXC_I2C1
#define I2C_FREQ        100000    
#define PMIC_SLAVE_ADDR  0x28
#define INIT_LEN        2
#define LED_SET_LEN     4
#define LED_CFG_REG_ADDR 0x2C
#define LED_SET_REG_ADDR 0x2D
#define PMIC_LED_BLUE   0x1
#define PMIC_LED_RED    0x2
#define PMIC_LED_GREEN  0x4

// SSD1306 OLED
#define SSD1306_ADDR    0x3C
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_CMD     0x00
#define SSD1306_DATA    0x40
#define LINE1_PAGE      0   // 
#define LINE2_PAGE      1   // 
#define LINE3_PAGE      2   // 
#define LINE4_PAGE      3   // 
#define LINE5_PAGE      4   // 
#define LINE6_PAGE      5   // 
#define LINE7_PAGE      6   // 
#define LINE8_PAGE      7   // 
#define LINE_X_OFFSET   0   // 


typedef enum {
    MENU_MAIN,
    MENU_TIME_DISPLAY,
    MENU_TIME_SET,
    MENU_HEART_RATE,
    MENU_EXIT
} menu_state_t;

 
typedef struct {
    uint8_t sec;   // 0-59
    uint8_t min;   // 0-59
    uint8_t hr;    // 0-23
} my_rtc_time_t;

typedef enum {
    BTN_NONE,
    BTN_SW1,
    BTN_SW2,
    BTN_SW3
} btn_state_t;

static uint8_t tx_buf[256];  // I2C BUFF
menu_state_t current_menu = MENU_MAIN;
uint8_t menu_selected = 0;
my_rtc_time_t rtc_time = {0};  
uint8_t setting_idx = 0;       // 0:H,1:M,2:S
//uint32_t rtc_total_sec = 0;    // RTC 
my_rtc_time_t current_time;

void SetLEDs(int state);
int OLED_Write(uint8_t type, uint8_t *data, uint8_t len);
void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_SetCursor(uint8_t x, uint8_t y);
void SSD1306_PrintChar(char c);
void SSD1306_PrintString(const char *str);
void Show_Main_Menu(void);           
void Process_Menu(btn_state_t btn);  
void Show_Time(void);                
uint8_t get_data_show(uint32_t *hr, uint32_t *spo2);  
/*****  8x8 ASCII  (32-126) *****/
const uint8_t font8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 32:  
    {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00}, // 33: !
    {0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00}, // 34: "
    {0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00, 0x00, 0x00}, // 35: #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, 0x00, 0x00}, // 36: $
    {0x23, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00}, // 37: %
    {0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x00, 0x00}, // 38: &
    {0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // 39: '
    {0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00}, // 40: (
    {0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00}, // 41: )
    {0x14, 0x08, 0x3E, 0x08, 0x14, 0x00, 0x00, 0x00}, // 42: *
    {0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00}, // 43: +
    {0x00, 0x50, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, // 44: ,
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00}, // 45: -
    {0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}, // 46: .
    {0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00}, // 47: /
    {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, 0x00}, // 48: 0
    {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}, // 49: 1
    {0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00}, // 50: 2
    {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00, 0x00}, // 51: 3
    {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00, 0x00}, // 52: 4
    {0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00}, // 53: 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00}, // 54: 6
    {0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00, 0x00}, // 55: 7
    {0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}, // 56: 8
    {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x00}, // 57: 9
    {0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00}, // 58: :
    {0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00}, // 59: ;
    {0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00}, // 60: <
    {0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00}, // 61: =
    {0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00}, // 62: >
    {0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00, 0x00}, // 63: ?
    {0x32, 0x49, 0x79, 0x41, 0x3E, 0x00, 0x00, 0x00}, // 64: @
    {0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00, 0x00, 0x00}, // 65: A
    {0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00}, // 66: B
    {0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00, 0x00}, // 67: C
    {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00}, // 68: D
    {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00, 0x00}, // 69: E
    {0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, 0x00, 0x00}, // 70: F
    {0x3E, 0x41, 0x49, 0x49, 0x3A, 0x00, 0x00, 0x00}, // 71: G
    {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00, 0x00}, // 72: H
    {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00, 0x00}, // 73: I
    {0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, 0x00, 0x00}, // 74: J
    {0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00}, // 75: K
    {0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00}, // 76: L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, 0x00, 0x00}, // 77: M
    {0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00, 0x00}, // 78: N
    {0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00, 0x00}, // 79: O
    {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00}, // 80: P
    {0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00, 0x00}, // 81: Q
    {0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00, 0x00}, // 82: R
    {0x46, 0x49, 0x49, 0x49, 0x31, 0x00, 0x00, 0x00}, // 83: S
    {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00}, // 84: T
    {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00, 0x00}, // 85: U
    {0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00, 0x00}, // 86: V
    {0x3F, 0x40, 0x30, 0x40, 0x3F, 0x00, 0x00, 0x00}, // 87: W
    {0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00, 0x00}, // 88: X
    {0x07, 0x08, 0x70, 0x08, 0x07, 0x00, 0x00, 0x00}, // 89: Y
    {0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x00}, // 90: Z
    {0x00, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00}, // 91: [
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00}, /* 92: \ */
    {0x00, 0x41, 0x41, 0x7F, 0x00, 0x00, 0x00, 0x00}, // 93: ]
    {0x04, 0x02, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00}, // 94: ^
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00}, // 95: _
    {0x00, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00}, // 96: `
    {0x20, 0x54, 0x54, 0x54, 0x78, 0x00, 0x00, 0x00}, // 97: a
    {0x7F, 0x48, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00}, // 98: b
    {0x38, 0x44, 0x44, 0x44, 0x20, 0x00, 0x00, 0x00}, // 99: c
    {0x38, 0x44, 0x44, 0x48, 0x7F, 0x00, 0x00, 0x00}, // 100: d
    {0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00, 0x00}, // 101: e
    {0x08, 0x7E, 0x09, 0x01, 0x02, 0x00, 0x00, 0x00}, // 102: f
    {0x0C, 0x52, 0x52, 0x52, 0x3E, 0x00, 0x00, 0x00}, // 103: g
    {0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00}, // 104: h
    {0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, 0x00, 0x00}, // 105: i
    {0x20, 0x40, 0x44, 0x3D, 0x00, 0x00, 0x00, 0x00}, // 106: j
    {0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00}, // 107: k
    {0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00}, // 108: l
    {0x7C, 0x04, 0x78, 0x04, 0x78, 0x00, 0x00, 0x00}, // 109: m
    {0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00}, // 110: n
    {0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00}, // 111: o
    {0x7C, 0x14, 0x14, 0x14, 0x08, 0x00, 0x00, 0x00}, // 112: p
    {0x08, 0x14, 0x14, 0x14, 0x7C, 0x00, 0x00, 0x00}, // 113: q
    {0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, 0x00, 0x00}, // 114: r
    {0x48, 0x54, 0x54, 0x54, 0x20, 0x00, 0x00, 0x00}, // 115: s
    {0x04, 0x3F, 0x44, 0x40, 0x20, 0x00, 0x00, 0x00}, // 116: t
    {0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, 0x00, 0x00}, // 117: u
    {0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, 0x00, 0x00}, // 118: v
    {0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, 0x00, 0x00}, // 119: w
    {0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00}, // 120: x
    {0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00, 0x00, 0x00}, // 121: y
    {0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x00, 0x00}, // 122: z
    {0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00}, // 123: {
    {0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00}, // 124: |
    {0x00, 0x41, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00}, // 125: }
    {0x10, 0x08, 0x08, 0x10, 0x08, 0x00, 0x00, 0x00}  // 126: ~
};

// GPIO
void Buttons_Init(void) {
    mxc_gpio_cfg_t sw_cfg;
    
    // SW1 (P0.2) 
    sw_cfg.port = SW1_PORT;
    sw_cfg.mask = SW1_PIN;
    sw_cfg.pad = MXC_GPIO_PAD_PULL_UP;
    sw_cfg.func = MXC_GPIO_FUNC_IN;
    MXC_GPIO_Config(&sw_cfg);
    
    // SW2 (P1.7) 
    sw_cfg.port = SW2_PORT;
    sw_cfg.mask = SW2_PIN;
    MXC_GPIO_Config(&sw_cfg);
    
    // SW3 (P3.1) 
    sw_cfg.port = SW3_PORT;
    sw_cfg.mask = SW3_PIN;
    MXC_GPIO_Config(&sw_cfg);
}


btn_state_t Check_Buttons(void) {
    // SW1
    if (!(MXC_GPIO_InGet(SW1_PORT, SW1_PIN))) {  
        MXC_Delay(MXC_DELAY_MSEC(20)); // Debouncing for button input
        if (!(MXC_GPIO_InGet(SW1_PORT, SW1_PIN))) {
            return BTN_SW1;
        }
    }
    
    // ¼ì²éSW2
    if (!(MXC_GPIO_InGet(SW2_PORT,SW2_PIN))) {  
        MXC_Delay(MXC_DELAY_MSEC(20));
        if (!(MXC_GPIO_InGet(SW2_PORT,SW2_PIN))) {
            return BTN_SW2;
        }
    }
    
    // ¼ì²éSW3
    if (!(MXC_GPIO_InGet(SW3_PORT,SW3_PIN))) {  
        MXC_Delay(MXC_DELAY_MSEC(20));
        if (!(MXC_GPIO_InGet(SW3_PORT,SW3_PIN))) {
            return BTN_SW3;
        }
    }
    
    return BTN_NONE;
}

void sec_to_hms(uint32_t sec, my_rtc_time_t *time) {
    time->hr = (sec / 3600) % 24;
    time->min = (sec / 60) % 60;
    time->sec = sec % 60;
}

uint32_t hms_to_sec(my_rtc_time_t *time) {
    return time->hr * 3600 + time->min * 60 + time->sec;
}

void RTC_Init(void) {
    if (MXC_RTC_Start() != E_NO_ERROR) {
        printf("RTC Initialization Failed\n");
        while(1);
    }
    printf("RTC Initialization Complete\n");
}



/***** Function Implementation *****/
void SetLEDs(int state) {
    int error;
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = LED_SET_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    tx_buf[0] = LED_SET_REG_ADDR;
    tx_buf[1] = (uint8_t)((state & PMIC_LED_BLUE) << 5);
    tx_buf[2] = (uint8_t)((state & PMIC_LED_RED) << 4);
    tx_buf[3] = (uint8_t)((state & PMIC_LED_GREEN) << 3);

    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        printf("Error writing to PMIC: %d\n", error);
        MXC_I2C_Stop(PMIC_I2C);  // On Error: Release Bus
    }
    MXC_I2C_Stop(PMIC_I2C);  // Release Bus
}

int OLED_Write(uint8_t type, uint8_t *data, uint8_t len) {
    mxc_i2c_req_t req;
    req.i2c = PMIC_I2C;
    req.addr = SSD1306_ADDR;
    req.tx_len = len + 1;
    req.rx_len = 0;
    req.restart = 0;

    tx_buf[0] = type;
    memcpy(&tx_buf[1], data, len);
    req.tx_buf = tx_buf;

    int error = MXC_I2C_MasterTransaction(&req);
 
    MXC_Delay(MXC_DELAY_USEC(100));  // Add delay to ensure bus stability
    return error;
}

void SSD1306_Init(void) {
    uint8_t cmd[] = {
        0xAE,       // Display OFF
        0x00, 0x10, // Set column address (low/high)
        0x40,       // Set start line
        0xB0,       // Set page address
        0x81, 0xCF, // Set contrast
        0xA1,       // Segment remap (flip horizontally)
        0xA6,       // Normal display
        0xA8, 0x3F, // Set multiplex ratio
        0xD3, 0x00, // Set display offset
        0xD5, 0x80, // Set oscillator frequency
        0xD9, 0xF1, // Set pre-charge period
        0xDA, 0x12, // Set COM pins configuration
        0xDB, 0x40, // Set VCOMH level
        0xC8,       // Set scan direction (flip vertically)
        0x8D, 0x14, // Enable charge pump
        0xAF        // Display ON
    };
    MXC_Delay(MXC_DELAY_MSEC(10));
    OLED_Write(SSD1306_CMD, cmd, sizeof(cmd));
}

void SSD1306_Clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        SSD1306_SetCursor(0, page);
        uint8_t blank[128] = {0};
        OLED_Write(SSD1306_DATA, blank, 128);
        MXC_Delay(MXC_DELAY_USEC(100));  
    }
}

void SSD1306_SetCursor(uint8_t x, uint8_t y) {
    if (x >= SSD1306_WIDTH || y >= 8) return;
    uint8_t cmd[] = {
        0x21, x, SSD1306_WIDTH-1,
        0x22, y, 7
    };
    OLED_Write(SSD1306_CMD, cmd, sizeof(cmd));
}

void SSD1306_PrintChar(char c) {
    if (c < 32 || c > 126) c = ' ';
    OLED_Write(SSD1306_DATA, (uint8_t*)font8x8[c - 32], 8);
}

void SSD1306_PrintString(const char *str) {
    while (*str) {
        SSD1306_PrintChar(*str++);
        MXC_Delay(MXC_DELAY_USEC(50));  
    }
}
// Process Menu
void Process_Menu(btn_state_t btn) {
     if (btn == BTN_SW1) {
         current_menu = MENU_TIME_DISPLAY;
     } else if (btn == BTN_SW2) {
         current_menu = MENU_HEART_RATE;
     } 
}

// Show Time
void Show_Time(void) {
    uint32_t sec;
    char time_str[20];
    
    MXC_RTC_GetSeconds(&sec);
    sec_to_hms(sec, &current_time);
    
    sprintf(time_str, "Time: %02d:%02d:%02d", 
            current_time.hr, current_time.min, current_time.sec);
    printf("%s\n", time_str);
    
    SSD1306_SetCursor(LINE_X_OFFSET, LINE2_PAGE);
    SSD1306_PrintString("Current Time");
    SSD1306_SetCursor(LINE_X_OFFSET, LINE4_PAGE);
    SSD1306_PrintString(time_str);
    SSD1306_SetCursor(LINE_X_OFFSET, LINE8_PAGE);
    SSD1306_PrintString("SW2: Heart Rate");
}
// Show Main Menu
void Show_Main_Menu(void) {
    SSD1306_Clear();
    SSD1306_SetCursor(LINE_X_OFFSET, LINE1_PAGE);
    SSD1306_PrintString("Main Menu");
    SSD1306_SetCursor(LINE_X_OFFSET, LINE3_PAGE);
    SSD1306_PrintString("SW1: Time");
    SSD1306_SetCursor(LINE_X_OFFSET, LINE4_PAGE);
    SSD1306_PrintString("SW2: Heart Rate");
}

int main(void) {
    MXC_Delay(MXC_DELAY_MSEC(500));  
    printf("\n******** Featherboard I2C Demo*********\n");
    printf("Funtion: 1. RGB LED 2. OLED 3. MAX30102 4.RTC 5.Menu\n");

    int error, i = 0;
    char line_str[20];  // 

    // 1. Init I2C
    error = MXC_I2C_Init(PMIC_I2C, 1, 0);
    if (error != E_NO_ERROR) {
        printf("-->I2C Initialization Failed\n");
        while (1) {}
    }
    MXC_I2C_SetFrequency(PMIC_I2C, I2C_FREQ);
    printf("\n-->I2C Initialization Complete£¨ÆµÂÊ£º%d Hz£©\n", I2C_FREQ);

    // 2. Init PMIC LED
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = INIT_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    tx_buf[0] = LED_CFG_REG_ADDR;
    tx_buf[1] = 1;  // LED 1mA
    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        printf("PMIC LED Config Error: %d\n", error);
        MXC_I2C_Stop(PMIC_I2C);
        while (1) {}
    }
    MXC_I2C_Stop(PMIC_I2C);
    printf("\n-->PMIC LED Initialization Complete\n");

    // Init Buttons¡¢RTC¡¢OLED¡¢LED¡¢MAX30102
    Buttons_Init();
    RTC_Init();
    SSD1306_Init();
    SSD1306_Clear();
    
    Show_Main_Menu();
    
    printf("\n-->SSD1306 Initialization Complete\n");

    // 4. Init MAX30102
    printf("\n-->MAX30102 Initialization\n");
    MAX30102_Config();
    blood_data_update();
    printf("\n-->MAX30102 Initialization Complete\n");  
	uint32_t hr_rate=199,spo2_rate=100;
	uint8_t temp;

    while (1) {
        // Check button input
        btn_state_t btn = Check_Buttons();
        if (btn != BTN_NONE) {
            Process_Menu(btn);
            SSD1306_Clear();
        }

        // Update display based on current menu state
        switch (current_menu) {
            case MENU_TIME_DISPLAY:
                Show_Time(); // Display real-time clock
                MXC_Delay(MXC_DELAY_SEC(1));
                break;
                
            case MENU_HEART_RATE: {  
                enable_temp();
                temp = max30102_temp();
                printf("HeartRate:%d, SPO2:%d, Temp:%d\n", 
                       hr_rate, spo2_rate, temp);                

                SSD1306_SetCursor(LINE_X_OFFSET, LINE2_PAGE);
                SSD1306_PrintString("Heart Monitor");
                
                sprintf(line_str, "Rate:%d   ", hr_rate);
                SSD1306_SetCursor(LINE_X_OFFSET, LINE4_PAGE);
                SSD1306_PrintString(line_str);
                
                sprintf(line_str, "SPO2:%d%%  ", spo2_rate);
                SSD1306_SetCursor(LINE_X_OFFSET, LINE5_PAGE);
                SSD1306_PrintString(line_str);
                
                sprintf(line_str, "Temp:%d ", temp);
                SSD1306_SetCursor(LINE_X_OFFSET, LINE6_PAGE);
                SSD1306_PrintString(line_str);
                SSD1306_SetCursor(LINE_X_OFFSET, LINE8_PAGE);
                SSD1306_PrintString("SW1: Time");                                
                get_data_show(&hr_rate, &spo2_rate);  // Heart rate and SpO2 detection logic
                MXC_Delay(MXC_DELAY_MSEC(100));
                break;
            }  
                
            default:
                MXC_Delay(MXC_DELAY_MSEC(100));
                break;
        }

        // Handle LED cycling
        MXC_Delay(MXC_DELAY_MSEC(100));    
        SetLEDs(i);
        i = (i + 1) % 8;
    }
}