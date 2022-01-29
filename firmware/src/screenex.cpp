#include "screenex.h"

#include <alloca.h>
#include <stdint.h>

#include "sjis.h"

#include <debug.h>


// WORKAROUND:
static constexpr int GLYPH_MAX_BYTES = 28;


void ScreenEx::set_font(FontManager& font) {
    this->font = &font;
}


void ScreenEx::set_cursor(uint8_t left, uint8_t top) {
    this->cursor_top = top;
    this->cursor_left = left;
}

void ScreenEx::move_next_line(void) {
    this->cursor_top += this->font->FONT_HEIGHT;
    if ((this->cursor_top + this->font->FONT_HEIGHT) >= this->SCREEN_HEIGHT) {
        this->cursor_top = 0;
    }
}

void ScreenEx::put(char ch) {
    if (!this->font) {
        return;
    }

    // DEBUG("Received 0x%02x(%d)", ch, ch);

    if (ch == '\n') {
        this->move_next_line();
        this->cursor_left = 0;
        return;

    } else if (ch == '\r') {
        this->cursor_left = 0;
        return;
    }

    /* SJISを考慮した、画面への1文字表示：
        NOTE: SJISでは、ある1バイトが第1バイトか第2バイトかをそのバイトだけで判定することはできない。

        1. バッファが空で、SJISの第1バイト → バッファする
        2. バッファが空で、SJISの第1バイトではない → 1バイト文字なので、すぐ表示
        3. バッファが空でない → わからないので、バッファと合わせて表示する（そうしかない）
    */

   size_t currentbufstrlen = strlen(this->inputbuffer);

    uint8_t ku, ten;

    if (currentbufstrlen == 0) {
        if (sjis_is_first_byte(ch)) {
            // Append to buffer
            this->inputbuffer[0] = ch;
            this->inputbuffer[1] = '\0';
            return;

        } else {
            // Estimate as single byte char. So display it immediately.
            ku = 0;
            ten = ch;
        }
    } else {
        // Estimate as SJIS double byte char.
        this->inputbuffer[1] = ch;
        convert_mb_to_kuten_sjis(this->inputbuffer, &ku, &ten);
    }

    uint8_t* glyphbuffer = (uint8_t*)alloca(GLYPH_MAX_BYTES);
    uint8_t w, h;

    if (this->cursor_left < this->SCREEN_WIDTH) {
        // DEBUG("Print a char...");
        // STOPWATCH_BLOCK_START(get_glyph_from_kuten);
        this->font->get_glyph_from_kuten(ku, ten, glyphbuffer, &w, &h);
        // STOPWATCH_BLOCK_END(get_glyph_from_kuten);
        uint8_t x1 = this->cursor_left;
        uint8_t y1 = this->cursor_top;
        uint8_t x2 = x1 + w;
        uint8_t y2 = y1 + h;
        // STOPWATCH_BLOCK_START(draw_glyph_2);
        this->draw_glyph_2(this->cursor_top, this->cursor_left, glyphbuffer, w, h);
        // STOPWATCH_BLOCK_END(draw_glyph_2);
        this->cursor_left += w;
    }

    this->inputbuffer[0] = '\0';
}

void ScreenEx::put(const char* s) {
    size_t len = strlen(s);
    this->put(s, len);
}

void ScreenEx::put(const char* s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        this->put(s[i]);
    }
}

void ScreenEx::print_at(uint8_t left, uint8_t top, const char* s) {
    this->print_at(left, top, s, strlen(s));
}

void ScreenEx::print_at(uint8_t left, uint8_t top, const char* s, size_t len) {

    uint8_t org_left = this->cursor_left;
    uint8_t org_top = this->cursor_top;

    this->cursor_left = left;
    this->cursor_top = top;

    for (size_t i = 0; i < len; i++) {
        this->put(s[i]);
    }

    this->cursor_left = org_left;
    this->cursor_top = org_top;
}

void ScreenEx::println_at(uint8_t left, uint8_t top, const char* s) {
    this->println_at(left, top, s, strlen(s));
}

void ScreenEx::println_at(uint8_t left, uint8_t top, const char* s, size_t len) {
    this->print_at(left, top, s, len);
    this->put('\n');
}

size_t ScreenEx::write(uint8_t ch) {
    this->put(ch);
    return 1;
}
