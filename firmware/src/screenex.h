#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "screen.h"
#include "font.h"

#include <Print.h>

/* ShiftJIS文字列との併用を前提とした拡張クラス

   ## 制約・条件・挙動：
     - ShiftJISが前提で、2バイト文字は全角幅、1バイト文字は半角幅を占有する。
     - print系は、横幅をはみ出した場合は表示しない。（自動折り返しや自動改行はしない）。はみ出しは文字単位で判定する。

 */

/* ShiftJIS文字列との併用を前提とした拡張クラス */
class ScreenEx : public Screen, public Print {
public:


private:
    uint8_t cursor_top = 0;
    uint8_t cursor_left = 0;

    char inputbuffer[2] = { 0, 0 };

    FontManager* font;

public:

    void set_font(FontManager& font);
    
    void move_next_line(void);

    // カーソル位置を設定する
    void set_cursor(uint8_t left, uint8_t top);

    // 文字を1バイト追加する。バッファされる。
    void put(char ch);
    // 文字列を追加する
    void put(const char* s);
    // 文字列ブロックを追加する
    void put(const char* s, size_t len);

    void print_at(uint8_t left, uint8_t top, const char* s);
    void print_at(uint8_t left, uint8_t top, const char* s, size_t len);
    void println_at(uint8_t left, uint8_t top, const char* s);
    void println_at(uint8_t left, uint8_t top, const char* s, size_t len);

    // Printが必要とするwrite()を実装する
    virtual size_t write(uint8_t);
};
