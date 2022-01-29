#include <Arduino.h>

#include "screen.h"


// GLCD Pin1 : Vcc (5V)
//  Connected to GND.
// GLCD Pin2 : GND
//   Connected to Vcc.
// GLCD Pin3 : Contrast adjustment
//   Connected to Potentiometer.
// GLCD Pin4
#define GLCD_A0 23
// GLCD Pin 5
#define GLCD_CS1 22
// GLCD Pin 6
#define GLCD_CS2 20
// GLCD Pin 7 : Clock for driving LCD. 1kHz - 3kHz required.
#define GLCD_CL 8
// GLCD Pin 8
#define GLCD_E 19
// GLCD Pin 9
#define GLCD_RW 18
// GLCD Pin 10-17
#define GLCD_D0 17
#define GLCD_D1 16
#define GLCD_D2 15
#define GLCD_D3 14
#define GLCD_D4 13
#define GLCD_D5 12
#define GLCD_D6 11
#define GLCD_D7 10
// GLCD Pin 18
#define GLCD_RES 9
// GLCD Pin 19; Backlight Vcc
// #define GLCD_BKL 6
// GLCD Pin 20; Backlight GND


#define nop() __asm__ volatile ("nop")

void glcd_select_chip(bool select_cs1, bool select_cs2) {
  digitalWriteFast(GLCD_CS1, select_cs1 ? LOW : HIGH);
  digitalWriteFast(GLCD_CS2, select_cs2 ? LOW : HIGH);
}


void glcd_set_databus_as(uint8_t mode) {
    uint8_t pins[8] = { GLCD_D0, GLCD_D1, GLCD_D2, GLCD_D3,
                        GLCD_D4, GLCD_D5, GLCD_D6, GLCD_D7 };
    for (int i = 0; i < 8; ++i) {
        pinMode(pins[i], mode);
    }
}

void glcd_set_databus_as_output(void) {
    glcd_set_databus_as(OUTPUT);
}

void glcd_set_databus_as_input(void) {
    glcd_set_databus_as(INPUT);
}


void glcd_send_byte(bool is_command, uint8_t data) {
    glcd_set_databus_as_output();

    digitalWriteFast(GLCD_RW, LOW);  // Write mode
    digitalWriteFast(GLCD_A0, is_command ? LOW : HIGH);  // Command mode or data mode
    digitalWriteFast(GLCD_D0, data & 0x01 ? HIGH : LOW);
    digitalWriteFast(GLCD_D1, data & 0x02 ? HIGH : LOW);
    digitalWriteFast(GLCD_D2, data & 0x04 ? HIGH : LOW);
    digitalWriteFast(GLCD_D3, data & 0x08 ? HIGH : LOW);
    digitalWriteFast(GLCD_D4, data & 0x10 ? HIGH : LOW);
    digitalWriteFast(GLCD_D5, data & 0x20 ? HIGH : LOW);
    digitalWriteFast(GLCD_D6, data & 0x40 ? HIGH : LOW);
    digitalWriteFast(GLCD_D7, data & 0x80 ? HIGH : LOW);
    // delayMicroseconds(1);
    digitalWriteFast(GLCD_E, HIGH);
    // delayMicroseconds(1);
    nop();
    nop();
    digitalWriteFast(GLCD_E, LOW);
    // delayMicroseconds(1);
}

volatile
byte glcd_read_byte(bool is_status_read) {
    // Read mode
    digitalWriteFast(GLCD_RW, HIGH);
    digitalWriteFast(GLCD_A0, is_status_read ? LOW : HIGH);
    glcd_set_databus_as_input();

    digitalWriteFast(GLCD_E, HIGH);
    // delayMicroseconds(1);
    nop();
    nop();

    byte val = 0;
    val |= (digitalReadFast(GLCD_D0) == HIGH) ? 0x01 : 0;
    val |= (digitalReadFast(GLCD_D1) == HIGH) ? 0x02 : 0;
    val |= (digitalReadFast(GLCD_D2) == HIGH) ? 0x04 : 0;
    val |= (digitalReadFast(GLCD_D3) == HIGH) ? 0x08 : 0;
    val |= (digitalReadFast(GLCD_D4) == HIGH) ? 0x10 : 0;
    val |= (digitalReadFast(GLCD_D5) == HIGH) ? 0x20 : 0;
    val |= (digitalReadFast(GLCD_D6) == HIGH) ? 0x40 : 0;
    val |= (digitalReadFast(GLCD_D7) == HIGH) ? 0x80 : 0;

    digitalWriteFast(GLCD_E, LOW);
    // delayMicroseconds(1);

    return val;
}

void glcd_select_page(uint8_t page) {
    glcd_send_byte(true, 0b10111000 | page);
}

void glcd_select_col(uint8_t col) {
    glcd_send_byte(true, 0b000000000 | col);
}


static
void glcd_init_sequence(void) {
    
  // Prepare Reset pin before power on.
  digitalWriteFast(GLCD_RES, LOW);

  // Supply power for GLCD
  // digitalWriteFast(GLCD_VDD, HIGH);

  // Select both chip
  glcd_select_chip(true, true);

  // Reset and set into 68 mode.
  // digitalWriteFast(GLCD_RES, LOW);
  delay(1);
  digitalWriteFast(GLCD_RES, HIGH);
  delay(1);
  
  // Send Reset command
  glcd_send_byte(true, 0b11100010);
  delay(5);

  // Send static drive ON/OFF command
  // glcd_send_byte(true, 0b10100100);
  // delay(1);

  // Send duty select command
  // NOTE: SG12232C requires 1/32.
  //glcd_send_byte(true, 0b10101001);
  //delay(1);
  
  // Send display ON/OFF command
  glcd_send_byte(true, 0b10101111);
  delay(5);
}


void readmodifywrite_end(void) {
    // Leave Read-modify-write mode
    glcd_send_byte(true, 0xEE);
}


void readmodifywrite_start(void) {
    readmodifywrite_end();
    // Enter to Read-modify-write mode
    glcd_send_byte(true, 0xE0);
}

void readmodifywrite_write_or(byte orbyte) {
    // Dummy read
    glcd_read_byte(false);
    // Read data
    byte b = glcd_read_byte(false);
    b |= orbyte;
    glcd_send_byte(false, b);
}

void readmodifywrite_write_and(byte andbyte) {
    // Dummy read
    glcd_read_byte(false);
    // Read data
    byte b = glcd_read_byte(false);
    b &= andbyte;
    glcd_send_byte(false, b);
}

void readmodifywrite_write_not(void) {
    // Dummy read
    glcd_read_byte(false);
    // Read data
    byte b = glcd_read_byte(false);
    b = ~b & 0xFF;
    glcd_send_byte(false, b);
}

void readmodifywrite_write_xor(byte xorbyte) {
    // Dummy read
    glcd_read_byte(false);
    // Read data
    byte b = glcd_read_byte(false);
    b ^= xorbyte;
    glcd_send_byte(false, b);
}


void Screen::init_pins(void) {
      constexpr int OUTPUTPINS_COUNT = 15;
  int outputpins[OUTPUTPINS_COUNT] = {
    GLCD_A0, GLCD_CS1, GLCD_CS2, GLCD_CL, GLCD_E, GLCD_RW, GLCD_RES,
    GLCD_D0, GLCD_D1, GLCD_D2, GLCD_D3, GLCD_D4, GLCD_D5, GLCD_D6, GLCD_D7
  };
  for (int i = 0; i < OUTPUTPINS_COUNT; ++i) {
    pinMode(outputpins[i], OUTPUT);
    digitalWrite(outputpins[i], LOW);
  }

  //Serial.println("Clock out");
  
  // Start clock out
  analogWrite(GLCD_CL, 128);
}

void Screen::select_chip(bool chip1, bool chip2) {
    glcd_select_chip(chip1, chip2);
}

void Screen::select_page(uint8_t page) {
    page = min(page, 3);
    glcd_select_page(page);
}

void Screen::select_col(uint8_t col) {
    col = min(col, SCREEN_WIDTH_PER_CHIP - 1);
    glcd_select_col(col);
}

void Screen::send_byte(bool is_command, byte data) {
    glcd_send_byte(is_command, data);
}


void Screen::init(void) {
    this->init_pins();
    glcd_init_sequence();
}


void Screen::fill(uint8_t pattern) {
    glcd_select_chip(true, true);
    for (int page = 0; page < 4; ++page) {
        glcd_select_page(page);
        glcd_select_col(0);
        for (int col = 0; col < 61; ++col) {
            glcd_send_byte(false, pattern);
        }
    }
}


void Screen::clear(void) {
    this->fill(0x00);
}


/** カラムを連続して書き込む。チップをまたがないように呼び出し側で調整する
  @param page [IN] 
  @param startcol [IN]
  @param data [IN]
  @param datalen [IN] 
  */
static
void send_cols(uint8_t page, uint8_t startcol, byte* data, uint8_t datalen) {
    glcd_select_page(page);
    glcd_select_col(startcol);

    for (int i = 0; i < datalen; ++i) {
        glcd_send_byte(false, data[i]);
    }
}


/** ページ境界に揃えられたフォントグリフの描画
  @param line [IN] 1行目か、2行目か。（ドット単位のrowの選択ではない）
  @param col [IN] 先頭の列番号（ドット単位のcolの選択）
  @param glyph [IN] 行がするフォントグリフのデータへのポインタ
  @param glyphwidth [IN] フォントグリフの幅（ドット単位）
  @param glyphheight [IN] フォントグリフの高さ（ドット単位）
 */
void Screen::draw_glyph_obsolute(uint8_t line, uint8_t col, byte* glyph, uint8_t glyphwidth, uint8_t glyphheight) {
    uint8_t startcol_in_chip = col % 61;
    bool across_chip = (col + glyphwidth < 122) && ((startcol_in_chip + glyphwidth) > 60);
    uint8_t startchip = ((col / 61) % 2) + 1;
    uint8_t page = line * 2;
    glcd_select_chip(startchip & 0x01, startchip & 0x02);
    if (!across_chip) {
        uint8_t drawwidth;
        if (col + glyphwidth >= 122 - 1) {
            drawwidth = 122 - col - 1;
        } else {
            drawwidth = glyphwidth;
        }
        // Serial.print("draw_glyph(): col,drawidth=");
        // Serial.print(col);
        // Serial.print(",");
        // Serial.println(drawwidth);
        send_cols(page, startcol_in_chip, &glyph[0], drawwidth);
        page = page + 1;
        send_cols(page, startcol_in_chip, &glyph[glyphwidth], drawwidth);
    } else {
        uint8_t width_in_firstchip = 61 - col;
        uint8_t width_in_secondchip = glyphwidth - width_in_firstchip;
        uint8_t nextpage = page + 1;
        send_cols(page, startcol_in_chip, &glyph[0], width_in_firstchip);
        send_cols(nextpage, startcol_in_chip, &glyph[glyphwidth], width_in_firstchip);
        uint8_t nextchip = ((startchip - 1 + 1) % 2) + 1;
        glcd_select_chip(nextchip & 0x01, nextchip & 0x02);
        send_cols(page, 0, &glyph[width_in_firstchip], width_in_secondchip);
        send_cols(nextpage, 0, &glyph[glyphwidth + width_in_firstchip], width_in_secondchip);
    }

#if false
    uint8_t startpage = 2 * line;
    uint8_t startchip = (col / 61) + 1;
    col = col % 61;
    uint8_t start_col_in_chip = col % 61;
    uint8_t end_col_inchip = (col + glyphwidth) % 61;
    uint8_t bufidx = 0;
    glcd_select_chip(startchip & 0x01, startchip & 0x02);
    glcd_select_col(col);

    /* FIXME:
    チップまたぎの時、ページごとに横方向へ一気に描画しているので、
    上側のページ描画時にチップを切り替えて進行してしまうと、
    下側のページの描画が切り替え後のチップの身になってしまう。
    本来は、上側でチップ切り替えがあったら、下側は旧チップと新チップの両方に描画しないといけない。

    */

   for (int page = startpage; page < startpage + 2; ++page) {
        glcd_select_page(page);

        bool chip_changed = false;

        for (int colcnt = 0; colcnt < glyphwidth; ++colcnt) {
            uint8_t col_in_page = start_col_in_chip + colcnt;
            if (!chip_changed && col_in_page > 60) {
                // ページの右端に来た場合、チップを切り替えて、左端へ移動する
                uint8_t newchip = ((startchip + 1) % 2) + 1;
                glcd_select_chip(newchip & 0x01, newchip & 0x02);
                glcd_select_page(page);
                glcd_select_col(0);
                chip_changed = true;
            }
            if (col_in_page > 60) {
                col_in_page = colcnt;
            }
            glcd_send_byte(false, glyph[bufidx]);
            ++bufidx;
        }

        if (chip_changed) {
            glcd_select_chip(startchip & 0x01, startchip & 0x02);
        }
   }




   if (false) {
        for (int page = startpage; page < startpage + 2; ++page) {
            glcd_select_page(page);
            glcd_select_col(start_col_in_chip);

            // for (int col_in_chip = start_col_in_chip; col_in_chip < (start); ++c) {
            for (int colcnt = 0; colcnt < glyphwidth; ++colcnt) {
                uint8_t col_in_chip = start_col_in_chip + colcnt;
                if (col_in_chip >= 61) {
                    ++startchip;
                    if (startchip > 2) {
                        startchip = 1;
                    }
                    glcd_select_chip(startchip & 0x01, startchip & 0x02);
                    col_in_chip = 0;
                    start_col_in_chip = 0;
                    glcd_select_page(page);
                    glcd_select_col(col_in_chip);
                }
                glcd_send_byte(false, glyph[bufidx]);
                ++bufidx;


                // glcd_send_byte(false, glyph[bufidx]);
                // ++bufidx;
                // if (c >= 61) {
                //     ++startchip;
                //     glcd_select_chip(startchip & 0x01, startchip & 0x02);
                //     glcd_select_page(page);
                //     glcd_select_col(c % 61);
                // }
            }
        }
   }
#endif
}


void Screen::invert_block(uint8_t chip, uint8_t page, uint8_t start_col, uint8_t end_col) {
    page = page % 4;
    start_col = start_col % 61;
    end_col = end_col % 61;

    select_chip(chip & 0x01, chip & 0x02);
    select_page(page);
    select_col(start_col);

    // Enter to Read modify write mode
    glcd_send_byte(true, 0xE0);
    for (int col = start_col; col < end_col + 1; ++col) {
        // Dummy read
        glcd_read_byte(false);
        // Read data
        byte current = glcd_read_byte(false);
        byte invertedbyte = ~current;
        // Write data
        glcd_send_byte(false, invertedbyte);
    }
    // Leave Read modify write mode
    glcd_send_byte(true, 0xEE);
}

// Not works correctly. Obsolute. (Use invert_rect())
// void Screen::invert_rect_pagealigned(uint8_t toppage, uint8_t bottompage, uint8_t left, uint8_t right) {
//     uint8_t pagecount = bottompage - toppage + 1;
//     uint8_t width = right - left;
//     bool left_switched = false;
//     bool right_switched = false;
//     for (uint8_t page = 0; page < pagecount; page++) {
//         for (uint8_t x = 0; x < width; x++) {
//             if (!left_switched && (left+x) < 61) {
//                 select_chip(true, false);
//                 select_page(toppage + page);
//                 select_col(left + x);
//                 readmodifywrite_start();
//             } else if (!right_switched && 61 <= (left+x)) {
//                 readmodifywrite_end();
//                 select_chip(false, true);
//                 select_page(toppage + page);
//                 select_col((left + x) % 61);
//                 readmodifywrite_start();
//             }
//             readmodifywrite_write_not();
//         }
//         readmodifywrite_end();
//     }
// }


enum class DrawMode {
    put,
    clear,
    invert
};

/** 指定ドットを描画もしくは反転する
 * @param x 
 * @param y 
 * @param is_draw 黒い点を置くならtrue、反転させるならfalse
 */
static void draw_or_invert_dot(Screen* screen, uint8_t x, uint8_t y, DrawMode mode) {
    if (x >= screen->SCREEN_WIDTH || y >= screen->SCREEN_HEIGHT) {
        return;
    }

    if (x < 61) {
        glcd_select_chip(true, false);
    } else {
        glcd_select_chip(false, true);
    }
    glcd_select_page(y / 8);
    glcd_select_col(x % 61);
    
    // Enter to Read modify write mode
    readmodifywrite_start();

    // Write data

    uint8_t bitshift = y % 8;
    byte b;
    b = 0x01 << bitshift;

    if (mode == DrawMode::put) {
        readmodifywrite_write_or(b);

    } else if (mode == DrawMode::clear) {
        readmodifywrite_write_and(~b);

    } else if (mode == DrawMode::invert) {
        readmodifywrite_write_xor(b);
    }

    // Leave Read modify write mode
    readmodifywrite_end();
}


void Screen::draw_dot(uint8_t x, uint8_t y) {
    draw_or_invert_dot(this, x, y, DrawMode::put);
}


void Screen::clear_dot(uint8_t x, uint8_t y) {
    draw_or_invert_dot(this, x, y, DrawMode::clear);
}

void Screen::invert_dot(uint8_t x, uint8_t y) {
    draw_or_invert_dot(this, x, y, DrawMode::invert);
}


static void draw_or_invert_line(Screen* screen, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode mode) {
    bool steep = abs((int)y2 - y1) > abs((int)x2 - x1);
    if (steep) {
        // Swap x1 and y1
        uint8_t tmp = x1;
        x1 = y1;
        y1 = tmp;
        // Swap x2 and y2
        tmp = x2;
        x2 = y2;
        y2 = tmp;
    }
    if (x1 > x2) {
        // Swap x1 and x2
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
        // Swap y1 and y2
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    uint8_t deltax = x2 - x1;
    uint8_t deltay = abs(y2 - y1);
    float error = deltax / 2.0f;
    uint8_t y = y1;
    int8_t ystep = (y1 < y2) ? 1 : -1;
    for (uint8_t x = x1; x <= x2; x++) {
        if (steep) {
            draw_or_invert_dot(screen, y, x, mode);
        } else {
            draw_or_invert_dot(screen, x, y, mode);
        }
        error = error - deltay;
        if (error < 0) {
            y = y + ystep;
            error = error + deltax;
        }
    }
    
/* Reference code in Python:

    steep = abs(y1 - y0) > abs(x1 - x0)
    if steep:
        x0, y0 = y0, x0
        x1, y1 = y1, x1
    if x0 > x1:
        x0, x1 = x1, x0
        y0, y1 = y1, y0
    deltax = x1 - x0
    deltay = abs(y1 - y0)
    error = deltax / 2
    ystep = 1 if (y0 < y1) else -1
    y = y0
    for x in range(x0, x1 + 1):
        if steep:
            screen.set_at((y, x), color)
        else:
            screen.set_at((x, y), color)
        error = error - deltay
        if error < 0:
            y = y + ystep
            error = error + deltax
*/
}

void Screen::draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    draw_or_invert_line(this, x1, y1, x2, y2, DrawMode::put);
}

void Screen::clear_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    draw_or_invert_line(this, x1, y1, x2, y2, DrawMode::clear);
}

void Screen::invert_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    draw_or_invert_line(this, x1, y1, x2, y2, DrawMode::invert);
}

static void fill_or_invert_rect(Screen* screen, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode mode) {
    if (y1 > y2) {
        // Swap y1 and y2
        uint8_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    if (x1 > x2) {
        // Swap x1 and x2
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    x2 = min(x2, screen->SCREEN_WIDTH);
    y2 = min(y2, screen->SCREEN_HEIGHT);

    // Old impl. (Slow but simple)
    // for (uint8_t y = y1; y <= y2; y++) {
    //     draw_or_invert_line(screen, x1, y, x2, y, is_draw);
    // }

    for (uint8_t page = 0; page < screen->PAGE_COUNT; page++) {
        if (page < (y1 / 8)) {
            // このページに描画対象ドットはない
            continue;
        } else if (((((y2 + 7) / 8) - 1) < page)) {
            // 描画対象ドットをすべて描画し終えた
            break;
        }
        uint8_t b = 0xFF;
        if ((y1 / 8) == page) {
            uint8_t leftshift = y1 % 8;
            b <<= leftshift;
        }
        if ((((y2 + 7) / 8) - 1) == page) {
            uint16_t mask = 0x00FF;
            uint8_t rightshift = 7 - (y2 % 8);
            mask = mask >> rightshift;
            b &= (byte)(mask & 0xFF);
        }
        bool left_switched = false;
        bool right_switched = false;
        for (uint8_t x = x1; x <= x2; x++) {
            if (!left_switched && x < screen->SCREEN_WIDTH_PER_CHIP) {
                screen->select_chip(true, false);
                screen->select_page(page);
                screen->select_col(x);
                readmodifywrite_start();
                left_switched = true;
            } else if (!right_switched && screen->SCREEN_WIDTH_PER_CHIP <= x) {
                readmodifywrite_end();
                screen->select_chip(false, true);
                screen->select_page(page);
                screen->select_col(x % screen->SCREEN_WIDTH_PER_CHIP);
                readmodifywrite_start();
                right_switched = true;
            }
            if (mode == DrawMode::put) {
                readmodifywrite_write_or(b);

            } else if (mode == DrawMode::clear) {
                readmodifywrite_write_and(~b);

            } else if (mode == DrawMode::invert) {
                readmodifywrite_write_xor(b);
            }
        }
        readmodifywrite_end();
    }
}

void Screen::draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    this->draw_line(x1, y1, x2, y1);
    this->draw_line(x2, y1, x2, y2);
    this->draw_line(x2, y2, x1, y2);
    this->draw_line(x1, y2, x1, y1);
}

void Screen::fill_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    fill_or_invert_rect(this, x1, y1, x2, y2, DrawMode::put);
}

void Screen::clear_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    fill_or_invert_rect(this, x1, y1, x2, y2, DrawMode::clear);
}

void Screen::invert_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    fill_or_invert_rect(this, x1, y1, x2, y2, DrawMode::invert);
}

void Screen::draw_glyph(uint8_t top, uint8_t left, byte* buffer, uint8_t width, uint8_t height) {

    // Serial.printf("draw_buffer(): top=%d,left=%d,width=%d,height=%d\n", top, left, width, height);

    uint8_t toppage = top / 8;
    uint8_t bottom = min((top + height + 7) / 8, 3+1);
    uint8_t right = left + width;
    uint8_t verticalstride = (height + 7) / 8;
    uint8_t leftshift = top % 8;
    uint8_t buffertailoffset = verticalstride * width;

    // ページごとに → チップを切り替えながら列ごとに、ループを回す
    for (uint8_t page = 0; (toppage + page) < bottom; page++) {
        uint8_t col = 0;  // current drawing column number (in range of 0-121)
        bool left_switched = false;
        bool right_switched = false;
        for (uint8_t col = 0; col < width; col++) {
            if (!left_switched && left < 61) {
                select_chip(true, false);
                select_page(toppage + page);
                select_col(left + col);
                readmodifywrite_start();
                left_switched = true;

            } else if (!right_switched && 61 <= (left+col)) {
                readmodifywrite_end();
                select_chip(false, true);
                select_page(toppage + page);
                select_col((left + col) % 61);
                select_page(toppage + page);
                readmodifywrite_start();
                right_switched = true;
            }

            uint8_t byteoffset = (page * width) + col;
            // 上のページから溢れた分を合成する
            byte b = 0;
            if (page >= 1) {
                // 2番目以降のページでは、上のページから溢れた分を回収する必要がある（かもしれない）
                uint8_t prevbyteoffset = ((page-1) * width) + col;
                uint16_t tmp = (uint16_t)(buffer[prevbyteoffset] << leftshift);
                b |= ((uint16_t)tmp >> 8) & 0xFF;
            }
            if (byteoffset < buffertailoffset) {
                b |= (byte)((uint16_t)buffer[byteoffset] << leftshift);
            }
            readmodifywrite_write_or(b);
        }

        readmodifywrite_end();
    }

}

void Screen::draw_glyph_2(uint8_t top, uint8_t left, byte* buffer, uint8_t width, uint8_t height) {

    // Serial.printf("draw_buffer(): top=%d,left=%d,width=%d,height=%d\n", top, left, width, height);

    uint8_t toppage = top / 8;
    uint8_t bottom = min((top + height + 7) / 8, 3+1);
    // uint8_t bottompage = (top+height) / 8;
    uint8_t right = left + width;
    uint8_t verticalstride = (height + 7) / 8;
    uint8_t leftshift = top % 8;
    uint8_t buffertailoffset = verticalstride * width;
    bool page_aligned = (top % 8) == 0;

    // ページごとに → チップを切り替えながら列ごとに、ループを回す
    for (uint8_t page = 0; (toppage + page) < bottom; page++) {
    // for (uint8_t page = 0; page < this->PAGE_COUNT; page++) {
        uint8_t col = 0;  // current drawing column number (in range of 0-121)
        bool left_switched = false;
        bool right_switched = false;
        for (uint8_t col = 0; col < width; col++) {
            if (!left_switched && left < 61) {
                select_chip(true, false);
                select_page(toppage + page);
                select_col(left + col);
                left_switched = true;

            } else if (!right_switched && 61 <= (left+col)) {
                select_chip(false, true);
                select_page(toppage + page);
                select_col((left + col) % 61);
                select_page(toppage + page);
                right_switched = true;
            }

            uint8_t byteoffset = (page * width) + col;
            // 上のページから溢れた分を合成する
            byte b = 0;
            if (!page_aligned && (page >= 1)) {
                // 2番目以降のページでは、上のページから溢れた分を回収する必要がある（かもしれない）
                uint8_t prevbyteoffset = ((page-1) * width) + col;
                uint16_t tmp = (uint16_t)(buffer[prevbyteoffset] << leftshift);
                b |= ((uint16_t)tmp >> 8) & 0xFF;
            }
            if (byteoffset < buffertailoffset) {
                if (page_aligned) {
                    b = buffer[byteoffset];
                } else {
                    b |= (byte)((uint16_t)buffer[byteoffset] << leftshift);
                }
            }
            send_byte(false, b);
        }

        // if (page == bottompage) {
        //     break;
        // }
    }
}


void Screen::draw_hline(uint8_t x, uint8_t y1, uint8_t y2) {
    if (x >= 61) {
        select_chip(false, true);
        x = x % 61;
    } else {
        select_chip(true, false);
    }

    uint8_t toppage = y1 / 8;
    uint8_t bottompage = y2 / 8;

    for (uint8_t page = 0; page < this->PAGE_COUNT; page++) {
        if (page < toppage) {
            continue;
        }
        uint8_t b;
        if (page == toppage) {
            b = (uint8_t)(0xFF << (y1 % 8));
        } else if (page == bottompage) {
            b = (uint8_t)(0x00FF >> (7 - (y2 % 8)));
        } else {
            b = 0xFF;
        }
        select_page(page);
        select_col(x);
        send_byte(false, b);
        
        if (page == bottompage) {
            break;
        }
    }
}

#if false
#if false
void Screen::clear_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    uint8_t toppage = y1 / 8;
    uint8_t bottompage = (y2 + 7) / 8;
    uint8_t width = x2 - x1;
    for (uint8_t page = 0; page < this->PAGE_COUNT; page++) {
        if (page < toppage) {
            continue;
        } else if (page > bottompage) {
            break;
        }
        bool left_switched = false;
        bool right_switched = false;
        for (uint8_t col = 0; col < width; col++) {
            if (!left_switched && (x1 + col) < this->SCREEN_WIDTH_PER_CHIP) {
                select_chip(true, false);
                select_page(page);
                select_col(x1 + col);
                left_switched = true;
            } else if (!right_switched && this->SCREEN_WIDTH_PER_CHIP <= (x1 + col)) {
                select_chip(false, true);
                select_page(page);
                select_col((x1 + col) % this->SCREEN_WIDTH_PER_CHIP);
                right_switched = true;
            }
            send_byte(false, 0x00);
        }
    }
}
#endif

void Screen::clear_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    if (x1 > x2) {
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y1 > y2) {
        uint8_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    x2 = min(x2, this->SCREEN_WIDTH);
    y2 = min(y2, this->SCREEN_HEIGHT);

    uint8_t toppage = y1 / 8;
    uint8_t bottompage = y2 / 8;
    uint8_t width = x2 - x1;

    enum class ChipSelectionState : uint8_t {
        none,
        left,
        right
    };

    for (uint8_t page = 0; page < this->PAGE_COUNT; page++) {
        if (page < toppage) {
            continue;
        }
        ChipSelectionState chipstate = ChipSelectionState::none;

        for (uint8_t col = 0; col < width; col++) {
            uint8_t col_in_screen = x1 + col;
            if ((chipstate == ChipSelectionState::none) && col_in_screen < this->SCREEN_WIDTH_PER_CHIP) {
                select_chip(true, false);
                select_page(page);
                select_col(col_in_screen);
                chipstate = ChipSelectionState::left;

            } else if ((chipstate != ChipSelectionState::right) && this->SCREEN_WIDTH_PER_CHIP <= col_in_screen) {
                select_chip(false, true);
                select_page(page);
                select_col(col_in_screen % this->SCREEN_WIDTH_PER_CHIP);
                chipstate = ChipSelectionState::right;
            }

            send_byte(false, 0x00);
        }
        
        if (page == bottompage) {
            break;
        }
    }
}
#endif

static
void fill_or_clear_rect_pagealined(Screen* screen, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool is_clear) {
    if (y1 > y2) {
        // Swap y1 and y2
        uint8_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    if (x1 > x2) {
        // Swap x1 and x2
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    x2 = min(x2, screen->SCREEN_WIDTH);
    y2 = min(y2, screen->SCREEN_HEIGHT);

    uint8_t toppage = y1 / 8;
    uint8_t bottompage = (y2 + 7) / 8;

    for (uint8_t page = 0; page < screen->PAGE_COUNT; page++) {
        if (page < toppage) {
            // このページに描画対象ドットはない
            continue;
        }

        uint8_t b;
        if (is_clear) {
            b = 0x00;

        } else {
            b = 0xFF;
            if (page == toppage) {
                uint8_t leftshift = y1 % 8;
                b <<= leftshift;
            }
            if (page == bottompage) {
                uint8_t rightshift = 7 - (y2 % 8);
    #if false
                uint16_t mask = 0x00FF;
                mask = mask >> rightshift;
                b &= (byte)(mask & 0xFF);
    #else
                uint8_t mask = 0xFF;
                b = mask >> rightshift;
    #endif
            }
        }

        bool left_switched = false;
        bool right_switched = false;
        for (uint8_t x = x1; x <= x2; x++) {
            if (!left_switched && x < screen->SCREEN_WIDTH_PER_CHIP) {
                screen->select_chip(true, false);
                screen->select_page(page);
                screen->select_col(x);
                left_switched = true;
            } else if (!right_switched && screen->SCREEN_WIDTH_PER_CHIP <= x) {
                screen->select_chip(false, true);
                screen->select_page(page);
                screen->select_col(x % screen->SCREEN_WIDTH_PER_CHIP);
                right_switched = true;
            }
            
            screen->send_byte(false, b);
        }

        if (page == bottompage) {
            // 描画対象ドットをすべて描画し終えた
            break;
        }
    }
}


void Screen::fill_rect_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    fill_or_clear_rect_pagealined(this, x1, y1, x2, y2, false);
}

void Screen::clear_rect_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    fill_or_clear_rect_pagealined(this, x1, y1, x2, y2, true);
}
