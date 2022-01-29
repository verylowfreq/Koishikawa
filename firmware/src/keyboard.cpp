#include "keyboard.h"

#include <Arduino.h>
#include <Wire.h>

#include <debug.h>


/** ユーザーのキー入力を待機する場合のdelay()の共通実装 */
inline static
void sleep_for_userinput(void) {
    delay(3);
}

void serial_printf(const char* format, ...);


/** バイト列の差分をとる
 * @param a [IN] 
 * @param b [IN] 
 * @param dst [OUT]
 * @param len [IN]
 * @return 異なっていたらtrue
 */
static
bool get_diff(byte* a, byte* b, byte* dst, uint8_t len) {
    byte* ptr = dst;
    for (int acnt = 0; acnt < len; ++acnt) {
        byte aval = a[acnt];
        bool found = false;
        for (int bcnt = 0; bcnt < len; ++bcnt) {
            if (aval == b[bcnt]) {
                // 見つかった。差分ではない
                found = true;
                break;
            }
        }
        if (!found) {
            // 見つからなかった。差分
            *ptr++ = aval;
        }
    }
    return ptr != dst;
}


/** バイト列に新しい要素があるかをチェックする
 * @param prev [IN] 
 * @param newer [IN] 
 * @param dst [OUT]
 * @param len [IN]
 * @return 新しい要素が登場していたらtrue
 */
static
bool get_new_appear(byte* prev, byte* newer, byte* dst, uint8_t len) {
    byte* ptr = dst;
    for (int i = 0; i < len; ++i) {
        byte item_from_newer = newer[i];
        bool found_in_prev = false;
        for (int j = 0; j < len; ++j) {
            byte item_from_prev = prev[j];
            if (item_from_prev == item_from_newer) {
                // 過去の配列にも存在する要素
                found_in_prev = true;
            }
        }
        if (!found_in_prev) {
            // 新しい要素が見つかった
            *dst++ = item_from_newer;
        }
    }
    return ptr != dst;
#if false
prev [ 0, 1, 2, 3 ]
newer[ 0, 1, 2, 4 ]  ->> [ 4 ]

prev [0, 1, 2, 3 ]
nerwer[0, 1, 2, 2]  ->> [ ]

#endif

    // byte* ptr = dst;
    // for (int acnt = 0; acnt < len; ++acnt) {
    //     byte aval = a[acnt];
    //     bool found = false;
    //     for (int bcnt = 0; bcnt < len; ++bcnt) {
    //         if (aval == b[bcnt]) {
    //             // 見つかった。差分ではない
    //             found = true;
    //             break;
    //         }
    //     }
    //     if (!found) {
    //         // 見つからなかった。差分
    //         *ptr++ = aval;
    //     }
    // }
    // return ptr != dst;
}

bool Keyboard::read_report_from_keyboardcontroller(rawkeycode_t *dst) {
    // memcpy(this->prev_report, this->latest_report, Keyboard::READDATACOUNT);
    memset(dst, 0x00, Keyboard::READDATACOUNT);

    Wire.requestFrom(this->target_i2c_addr, READDATACOUNT);
    unsigned int err = 150 / 7 * 5;
    for (int readcnt = 0; readcnt < READDATACOUNT && err != 0; ++readcnt) {
        while (Wire.available() < 1) {
            delayMicroseconds(5);
            --err;
        }
        dst[readcnt] = Wire.read();
    }
    bool succeeded = err > 0;
    if (!succeeded) {
        DEBUG("read_report_from_keyboardcontroller() : I2C read failed.");
    }
    return succeeded;
}

bool Keyboard::read_report(void) {

    return this->read_report_from_keyboardcontroller(this->latest_report);
}

void Keyboard::init(void) {
    this->flush();
}


static
Keyboard::keycode_t convert_from_rawkeycode_to_keycode(Keyboard::rawkeycode_t rawkeycode, bool shift, bool fn1, bool fn2) {
    
    uint8_t table_base[][2] = {
        {1, Keyboard::KEYCODE_ESC},

        {2, 'q'},
        {3, 'w'},
        {4, 'e'},
        {5, 'r'},
        {6, 't'},
        {7, 'y'},
        {8, 'u'},
        {9, 'i'},
        {10, 'o'},
        {11, 'p'},

        {23,Keyboard::KEYCODE_BACKSPACE},  // Backspace

        {14, 'a'},
        {15, 's'},
        {16, 'd'},
        {17, 'f'},
        {18, 'g'},
        {19, 'h'},
        {20, 'j'},
        {21, 'k'},
        {22, 'l'},

        {35,Keyboard::KEYCODE_ENTER},

        {26, 'z'},
        {27, 'x'},
        {28, 'c'},
        {29, 'v'},
        {30, 'b'},
        {31, 'n'},
        {32, 'm'},
        {33, ','},
        {34, '.'},

        // 37 = Fn2
        {38, Keyboard::KEYCODE_FUNCTION_1},
        {39, Keyboard::KEYCODE_FUNCTION_2},
        {40, Keyboard::KEYCODE_MUHENKAN},
        {41, Keyboard::KEYCODE_SPACE},
        {42, Keyboard::KEYCODE_HENKAN_HIRA_KANA},
        // 43 = Fn2
        {44, Keyboard::KEYCODE_FUNCTION_3}
        // 45 = Fn1
    };
    uint8_t table_base_length = sizeof(table_base) / sizeof(table_base[0]);;

    uint8_t table_fn1[][2] = {
        {1, Keyboard::KEYCODE_ESC},

        {2, '1'},
        {3, '2'},
        {4, '3'},
        {5, '4'},
        {6, '5'},
        {7, '6'},
        {8, '7'},
        {9, '8'},
        {10, '9'},
        {11, '0'},

        {23,Keyboard::KEYCODE_BACKSPACE},  // Backspace

        {14, '@'},
        {15, '`'},
        {16, '+'},
        {17, '~'},
        {18, '^'},
        {18,'^'},
        {19,'{'},
        {20,'}'},
        {21,'['},
        {22,']'},

        {35,Keyboard::KEYCODE_ENTER},

        {26, 'z'},
        {27, 'x'},
        {28, 'c'},
        {29, 'v'},
        {30, 'b'},
        {31, ':'},
        {32, '?'},
        {33, 0xa5}, // 半角の中黒
        {34, '-'},

        {41,Keyboard::KEYCODE_SPACE}
    };
    uint8_t table_fn1_length = sizeof(table_fn1) / sizeof(table_fn1[0]);;

    uint8_t table_fn2[][2] = {
        {1, Keyboard::KEYCODE_ESC},

        {2, '!'},
        {3, '"'},
        {4, '#'},
        {5, '$'},
        {6, '%'},
        {7, '&'},
        {8, '\''},
        {9, '('},
        {10, ')'},
        {11, '\\'},

        {23,Keyboard::KEYCODE_BACKSPACE},  // Backspace

        {14, Keyboard::KEYCODE_HOME},
        {15, Keyboard::KEYCODE_PAGEUP},
        {16, Keyboard::KEYCODE_PAGEDOWN},
        {17, Keyboard::KEYCODE_END},
        {18, '^'},
        {19, Keyboard::KEYCODE_ARROWLEFT},
        {20, Keyboard::KEYCODE_ARROWDOWN},
        {21, Keyboard::KEYCODE_ARROWUP},
        {22, Keyboard::KEYCODE_ARROWRIGHT},

        {35,Keyboard::KEYCODE_ENTER},

        {26, 'z'},
        {27, 'x'},
        {28, 'c'},
        {29, '<'},
        {30, '>'},
        {31, ';'},
        {32, '|'},
        {33, '*'},
        {34, '_'},

        {41,Keyboard::KEYCODE_SPACE}
    };
    uint8_t table_fn2_length = sizeof(table_fn2) / sizeof(table_fn2[0]);;

    Keyboard::keycode_t keycode = 0x00;

    {
        uint8_t (*table)[][2];
        uint8_t tablelength;
        if (fn1) {
            table = (uint8_t (*)[][2])&table_fn1;
            tablelength = table_fn1_length;
        } else if (fn2) {
            table = (uint8_t (*)[][2])&table_fn2;
            tablelength = table_fn2_length;
        } else {
            table = (uint8_t (*)[][2])&table_base;
            tablelength = table_base_length;
        }
        
        for (int i = 0; i < tablelength; ++i) {
            if ((*table)[i][0] == rawkeycode) {
                keycode = (*table)[i][1];
                break;
            }
        }
    }

    if (shift && 'a' <= keycode && keycode <= 'z') {
        // DEBUG("Upper case");
        keycode -= 0x20;
    }

    if (shift && keycode == ',') {
        keycode = '/';
    }
    if (shift && keycode == '.') {
        keycode = '=';
    }

    return keycode;
}


void Keyboard::update(void) {
    

    // （もう古い）最新レポートをコピーする
    memcpy(this->prev_report, this->latest_report, Keyboard::READDATACOUNT);

    // キーの押下状況を取得して、最新レポートとしてキャッシュする
    if (!this->read_report()) {
        return;
    }

    //DEBUG:
    // {
    //     DEBUG("Raw keycodes: ");
    //     dump_bytes_dec(this->latest_report, 7);
    //     Serial.println();
    // }

    // DEBUG:
    {
        if (memcmp(this->prev_report, this->latest_report, READDATACOUNT) == 0) {
            // DEBUG("Not changed.");
            memset(this->buffered_keydown, 0x00, MAXKEYS);
            return;
        }
    }

    // DEBUG:
    // {
    //     rawkeycode_t *nowpressedkeys = (rawkeycode_t*)memchr(this->latest_report, 0x00, Keyboard::READDATACOUNT);
    //     DEBUG("Keys in down: %d", nowpressedkeys - this->latest_report);
    // }

    // 修飾キーのチェック
    bool is_shift_pressed = false;
    bool is_fn1_pressed = false;
    bool is_fn2_pressed = false;
    for (int i = 0; i < Keyboard::MAXKEYS; ++i) {
        rawkeycode_t cur = this->latest_report[i];
        if (cur == 25 || cur == 46) {
            // DEBUG("Shift key pressed.");
            is_shift_pressed = true;
            this->latest_report[i] = 0x00;
        }
        if (cur == 13 || cur == 45) {
            is_fn1_pressed = true;
        }
        if (cur == 37 || cur == 43) {
            is_fn2_pressed = true;
        }
    }

    // rawkeycodeで前回との差分をとる
    rawkeycode_t new_pressed_keys[Keyboard::MAXKEYS];
    memset(new_pressed_keys, 0x00, Keyboard::MAXKEYS);
    // get_diff(this->prev_report, this->latest_report, new_pressed_keys, Keyboard::MAXKEYS);
    get_new_appear(this->prev_report, this->latest_report, new_pressed_keys, Keyboard::MAXKEYS);
    // uint8_t new_pressed_keys_idx = 0;
    // for (int i = 0; i < MAXKEYS; ++i) {
    //     rawkeycode_t cur_key = this->latest_report[i];
    //     if (cur_key == 0x00) {
    //         continue;
    //     }
    //     bool is_new_key = true;
    //     for (int j = 0; j < MAXKEYS; ++j) {
    //         rawkeycode_t prev_key = this->prev_report[i];
    //         if (prev_key == 0x00) {
    //             continue;
    //         }
    //         if (cur_key == prev_key) {
    //             is_new_key = false;
    //             break;
    //         }
    //     }
    //     if (is_new_key) {
    //         // New key pressed down now
    //         if (new_pressed_keys_idx < MAXKEYS) {
    //             new_pressed_keys[new_pressed_keys_idx] = cur_key;
    //         }
    //         ++new_pressed_keys_idx;
    //     }
    // }

    //DEBUG:
    // {
    //     DEBUG("Prev rawkeycodes: ");
    //     dump_bytes_dec(this->prev_report, 7);
    //     Serial.println();
    //     DEBUG("Cur  rawkeycodes: ");
    //     dump_bytes_dec(this->latest_report, 7);
    //     Serial.println();
    //     DEBUG("New  rawkeycodes: ");
    //     dump_bytes_dec(new_pressed_keys, 7);
    //     Serial.println();
    // }

    //DEBUG:
    // {
    //     rawkeycode_t *tail = (rawkeycode_t*)memchr(new_pressed_keys, 0x00, Keyboard::MAXKEYS);
    //     uint8_t len = tail - new_pressed_keys;
    //     DEBUG("New Pressed Keys : %d", len);
    // }

    // keycodeの配列を初期化する
    // memset(this->keycodes_pressing, 0x00, Keyboard::MAXKEYS);
    memset(this->buffered_keydown, 0x00, Keyboard::MAXKEYS);

    // 新たに押下されたキーについてのみ、キーコードへ変換する
    for (int i = 0; i < Keyboard::MAXKEYS; ++i) {
        rawkeycode_t rawkeycode = new_pressed_keys[i];
        keycode_t keycode = 0x00;
        keycode = convert_from_rawkeycode_to_keycode(rawkeycode, is_shift_pressed, is_fn1_pressed, is_fn2_pressed);
        if (keycode != 0x00) {
            this->buffered_keydown[i] = keycode;
        }
    }

    // DEBUG("Dump this->buffered_keydown\n  ");
    // dump_bytes_hex(this->buffered_keydown, MAXKEYS);
    // Serial.println();
}


uint8_t Keyboard::get_key(void) {
    for (int i = 0; i < Keyboard::MAXKEYS; ++i) {
        uint8_t keycode = this->buffered_keydown[i];
        if (keycode == 0x00) {
            continue;
        } else {
            this->buffered_keydown[i] = 0x00;
            return keycode;
        }
    }
    return 0x00;
}


void Keyboard::flush(void) {
    memset(this->prev_report, 0x00, 7);
    memset(this->latest_report, 0x00, 7);

    // memset(this->keycodes_pressing, 0x00, 6);
    memset(this->buffered_keydown, 0x00, 6);
}


uint8_t Keyboard::get_report(byte* dst, size_t dstlen) {
    size_t cur = 0;
    byte* src = this->latest_report;
    dstlen = min(dstlen, this->MAXKEYS);
    size_t i;
    size_t writtenlen = 0;
    for (i = 0; i < dstlen; i++) {
        byte ch = *src++;
        if (ch != 0x00) {
            *dst++ = ch;
            writtenlen += 1;
        }
    }
    return writtenlen;
}


void Keyboard::wait_any_key(void) {
    while (true) {
        if (!this->read_report()) {
            Serial.println("ERR: Keyboard::wait_any_key() : Failed to read_report()");
        }

        bool is_nokey_pressed = true;

        for (int i = 0; i < MAXKEYS; ++i) {
            if (this->latest_report[i] != 0x00) {
                is_nokey_pressed = false;
                break;
            }
        }

        if (is_nokey_pressed) {
            sleep_for_userinput();
            continue;

        } else {
            DEBUG("Finished.");
            return;
        }
    }
}

void Keyboard::wait_allkey_released(void) {
    while (true) {
        rawkeycode_t buf[READDATACOUNT];
        bool wait_more = false;

        if (!this->read_report_from_keyboardcontroller(buf)) {
            return;

        } else {
            for (int i = 0; i < MAXKEYS; ++i) {
                if (buf[i] != 0x00) {
                    wait_more = true;
                    break;
                }
            }
            if (wait_more) {
                sleep_for_userinput();
                continue;

            } else {
                this->flush();
                return;
            }
        }
    }
}

bool Keyboard::is_key_down_rawkeycode(rawkeycode_t key) {
    for (int i = 0; i < MAXKEYS; ++i) {
        if (this->latest_report[i] == key) {
            return true;
        }
    }
    return false;
}