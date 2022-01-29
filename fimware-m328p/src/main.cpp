/*
 Copyright 2022 verylowfreq ( https://github.com/verylowfreq/ )

 Permission is hereby granted, free of charge, to any person obtaining a copy 
 of this software and associated documentation files (the "Software"), to 
 deal in the Software without restriction, including without limitation the 
 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 sell copies of the Software, and to permit persons to whom the Software is 
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in 
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 DEALINGS IN THE SOFTWARE.
*/


#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>

#include "lock.h"

#define PIN_DEBUG_LED PB6

/* キーの物理レイアウトとキー番号の対応
 1,  2,  3,  4,    5,  6,  7,  8,  9, 10,  11,  [23],
13, 14, 15, 16,   17, 18, 19, 20, 21, 22, [35],
25, 26, 27, 28,  29,  30, 31, 32, 33, 46,
37, 38, 39, 40, [41], 42, 43, 44, 45
*/

#define KEYMATRIX_COL0 9
#define KEYMATRIX_COL1 10
#define KEYMATRIX_COL2 11
#define KEYMATRIX_COL3 12
#define KEYMATRIX_COL4 13
#define KEYMATRIX_COL5 A0
constexpr uint8_t KEYMATRIX_COL_PINS[] = { KEYMATRIX_COL0,KEYMATRIX_COL1,KEYMATRIX_COL2,KEYMATRIX_COL3,KEYMATRIX_COL4,KEYMATRIX_COL5 };
constexpr uint8_t KEYMATRIX_COLS_COUNT = sizeof(KEYMATRIX_COL_PINS) / sizeof(KEYMATRIX_COL_PINS[0]);
#define KEYMATRIX_ROW0 8
#define KEYMATRIX_ROW1 7
#define KEYMATRIX_ROW2 6
#define KEYMATRIX_ROW3 5
constexpr uint8_t KEYMATRIX_ROW_PINS[] = { KEYMATRIX_ROW0,KEYMATRIX_ROW1,KEYMATRIX_ROW2,KEYMATRIX_ROW3 };
constexpr uint8_t KEYMATRIX_ROWS_COUNT = sizeof(KEYMATRIX_ROW_PINS) / sizeof(KEYMATRIX_ROW_PINS[0]);

constexpr uint8_t KEYMATRIX_LAYOUT_COLS_COUNT = 12;
constexpr uint8_t KEYMATRIX_LAYOUT_ROWS_COUNT = 4;

// 43 keys < 6bytes * 8bits/byte = 48bits
uint8_t keystatus[6] = { 0 };

void keymatrix_init(void) {
    for (int col = 0; col < KEYMATRIX_COLS_COUNT; ++col) {
        pinMode(KEYMATRIX_COL_PINS[col], INPUT_PULLUP);
    }
    for (int row = 0; row < KEYMATRIX_ROWS_COUNT; ++row) {
        pinMode(KEYMATRIX_ROW_PINS[row], INPUT_PULLUP);
    }
}


void keymatrix_set_key_status(uint8_t col, uint8_t row, bool status) {
    uint8_t keyindex = col + row * 12;
    uint8_t byteindex = keyindex / 8;
    uint8_t bitindex = keyindex % 8;
    if (status) {
        keystatus[byteindex] |= (0x01 << bitindex);
    } else {
        keystatus[byteindex] &= ~(0x01 << bitindex);
    }
}

bool keymatrix_get_key_status_by_keyindex(uint8_t keyindex) {
    uint8_t byteindex = keyindex / 8;
    uint8_t bitindex = keyindex % 8;
    return keystatus[byteindex] & (0x01 << bitindex);
}

void keymatrix_dump_status(void) {
    Serial.println("keymatrix status:");
    // バイトごと
    for (int i = 0; i < 6; ++i) {
        // ビットごと
        for (int j = 0; j < 8; ++j) {
            bool pressed = keystatus[i] & (0x01 << j);
            if (pressed) {
                Serial.print("("); Serial.print(i * 8 + j + 1); Serial.print("), ");
            }
        }
    }
    Serial.println();
}


void keymatrix_clear_status(void) {
    memset(keystatus, 0x00, sizeof(keystatus));
}

#define SMALL_DELAY()   delayMicroseconds(30)


void keymatrix_col2row(void) {
    //        Selected       Released
    // COL : INPUT_PULLUP   INPUT_PULLUP
    // ROW : OUTPUT LOW     INPUT_PULLUP

    // Unselect rows
    for (int row = 0; row < KEYMATRIX_ROWS_COUNT; ++row) {
        pinMode(KEYMATRIX_ROW_PINS[row], INPUT_PULLUP);
    }
    
    // Loop for each col
    for (int col = 0; col < KEYMATRIX_COLS_COUNT; ++col) {
        // Loop for each row
        for (int row = 0; row < KEYMATRIX_ROWS_COUNT; ++row) {
            // Select one row
            pinMode(KEYMATRIX_ROW_PINS[row], OUTPUT);
            digitalWrite(KEYMATRIX_ROW_PINS[row], LOW);
            SMALL_DELAY();
            bool pressed = digitalRead(KEYMATRIX_COL_PINS[col]) == LOW;
            keymatrix_set_key_status(col * 2, row, pressed);
            // Unselect one row
            pinMode(KEYMATRIX_ROW_PINS[row], INPUT_PULLUP);
        }
    }
}

void keymatrix_row2col(void) {
    //        Selected       Released
    // ROW : INPUT_PULLUP   INPUT_PULLUP
    // COL : OUTPUT LOW     INPUT_PULLUP

    // Unselect cols
    for (int col = 0; col < KEYMATRIX_COLS_COUNT; ++col) {
        pinMode(KEYMATRIX_COL_PINS[col], INPUT_PULLUP);
    }

    // Loop for each row
    for (int row = 0; row < KEYMATRIX_ROWS_COUNT; ++row) {
        // Loop for each col
        for (int col = 0; col < KEYMATRIX_COLS_COUNT; ++col) {
            // Select one col
            pinMode(KEYMATRIX_COL_PINS[col], OUTPUT);
            digitalWrite(KEYMATRIX_COL_PINS[col], LOW);
            SMALL_DELAY();
            bool pressed = digitalRead(KEYMATRIX_ROW_PINS[row]) == LOW;
            keymatrix_set_key_status(col * 2 + 1, row, pressed);
            // Unselect one col
            pinMode(KEYMATRIX_COL_PINS[col], INPUT_PULLUP);
        }
    }
}

void update_keystatus(void);

Mutex keystatus_mutex;

uint8_t i2c_addr = 0x21;

uint8_t prev_report[6] = { 0 };

void wire_onRequest(void) {

    update_keystatus();

    int writtencount = 0;
    for (uint8_t key = 0; key < 8 * 6 && writtencount < 6; ++key) {
        if (keymatrix_get_key_status_by_keyindex(key)) {
            Wire.write(key + 1);
            ++writtencount;
        }
    }
    for (int i = writtencount; i < 6; ++i) {
        Wire.write(0x00);
    }
    // Termination byte
    Wire.write(0xFF);

    return;
}


void setup(void) {
    Serial.begin(57600);

    Serial.println("Starting...");

    // Debug LED as OUTPUT
    DDRB |= DDB6;

    for (int row = 0; row < KEYMATRIX_ROWS_COUNT; ++row) {
        pinMode(KEYMATRIX_ROW_PINS[row], OUTPUT);
        digitalWrite(KEYMATRIX_ROW_PINS[row], LOW);
    }
    for (int col = 0; col < KEYMATRIX_COLS_COUNT; ++col) {
        pinMode(KEYMATRIX_COL_PINS[col], OUTPUT);
        digitalWrite(KEYMATRIX_COL_PINS[col], LOW);
    }

    update_keystatus();

    Serial.print("Init Wire...");
    Wire.begin(i2c_addr);
    Wire.onRequest(wire_onRequest);
    Serial.print(" OK. My address is 0x"); Serial.println(i2c_addr, HEX);

    sleepMode(SLEEP_IDLE);

    // Debug LED to ON
    PORTB |= PORTB6;
    
    Serial.println("OK.");
}

bool is_first_loop = true;


void update_keystatus(void) {

    keymatrix_clear_status();
    keymatrix_col2row();
    keymatrix_row2col();
}


void loop(void) {

    sleep();
}
