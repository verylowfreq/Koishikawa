#pragma once

#include <stdint.h>
#include <stdbool.h>

// For "byte" typedef
#include <Arduino.h>


// #define SCREEN_WIDTH 122
// #define SCREEN_HEIGHT 32


// SG12232Cを駆動し描画するクラス
class Screen {
public:
    // 横は122ドット (0-121)
    static constexpr uint8_t SCREEN_WIDTH = 122;
    // ひとつのチップが担当する横幅は61ドット (0-60)
    static constexpr uint8_t SCREEN_WIDTH_PER_CHIP = 61;
    // 縦は32ドット (0-31)
    static constexpr uint8_t SCREEN_HEIGHT = 32;
    // 縦方向に8ドットの塊であるページが4つ並んでいる
    static constexpr uint8_t PAGE_COUNT = 4;

    void init_pins(void);
    void send_byte(bool is_command, byte data);
    /** チップ1, 2を選択する (1: Chip1, 2: Chip2, 3: 両方) */
    void select_chip(bool chip1, bool chip2);
    /** ページ選択 (0-3) */
    void select_page(uint8_t page);
    /** 列選択 (0-60) */
    void select_col(uint8_t col);

    void init(void);
    void clear(void);
    /** 与えられたバイトをスクリーン全体に設定する */
    void fill(uint8_t pattern);

    void draw_dot(uint8_t x, uint8_t y);
    void clear_dot(uint8_t x, uint8_t y);
    void invert_dot(uint8_t x, uint8_t y);

    void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    void clear_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    void invert_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    // 四角形を描く（輪郭線のみ）
    void draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    // 矩形領域を塗りつぶす
    void fill_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    // 矩形領域を消去する
    void clear_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    // 矩形領域を反転する
    void invert_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    /** OBSOLUTE.
     * @param chip 
     * @param page 
     * @param start_col 
     * @param end_col 
     */
    void invert_block(uint8_t chip, uint8_t page, uint8_t start_col, uint8_t end_col);

    // void invert_rect_pagealigned(uint8_t toppage, uint8_t bottompage, uint8_t left, uint8_t right);

    /** バッファの内容を転送する 
     * @param top [IN] 
     * @param left [IN] 
     * @param buffer [IN] LSBからMSBへ向けて、topからbottomへドットが並んでいるビットマップ
     * @param width [IN] バッファが表す領域の幅
     * @param height [IN] バッファが表す領域の高さ（バイト単位で列を成し、列内末尾の余剰ドットは無視される）
     */
    void draw_glyph(uint8_t top, uint8_t left, byte* buffer, uint8_t width, uint8_t height);


    /** OBSOLUTE. グリフを描画する
      @param line [IN] 0 or 1（1行目、もしくは2行目）
      @param col [IN] 0-121
      @param glyph [IN]
      @param glyphwidth [IN] width of the glyph
      @param glyphheight [IN] height of the glyph
      */
    void draw_glyph_obsolute(uint8_t line, uint8_t col, byte* glyph, uint8_t glyphwidth, uint8_t glyphheight);

    // ---- 特定条件下の最適化をしたメソッド ----

    // ページ単位で上書き動作をする
    void draw_glyph_2(uint8_t top, uint8_t left, byte* buffer, uint8_t width, uint8_t height);

    void draw_hline(uint8_t x, uint8_t y1, uint8_t y2);

    // void clear_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    void fill_rect_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
    void clear_rect_pagealined(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
};
