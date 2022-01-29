#include "sjis.h"

#include <string.h>

#include <debug.h>



bool sjis_is_first_byte(uint8_t ch) {
    if (0 <= ch && ch <= 0x80) {
        // ASCIIの制御文字と通常の文字
        return false;

    } else if (0x81 <= ch && ch <= 0x9F) {
        // 半角カナより前のSJIS第1バイトの領域
        return true;

    } else if (0xA0 <= ch && ch <= 0xDF) {
        // 半角カナ
        return false;

    } else if (0xE0 <= ch && ch <= 0xFF) {
        // 半角カナより後ろのSJIS第1バイトの領域
        // 暫定処理として、ASCII範囲外の末尾の空間はまるごと2バイト文字と推定する
        // NOTE: ShiftJIS本来は、0xE0から0xEFまで。
        return true;
        
    } else {
        // Should not be reached.
        return false;
    }
}


uint8_t count_bytes_of_a_char_sjis(const char* sjis) {
    // uint8_t b0 = sjis[0];
    // // uint8_t b1 = sjis[1];

    // if (0 <= b0 && b0 <= 0x80) {
    //     return 1;
    // } else if (0x81 <= b0 && b0 <= 0x9F) {
    //     return 2;
    // } else if (0xA0 <= b0 && b0 <= 0xDF) {
    //     return 1;
    // } else if (0xE0 <= b0 && b0 <= 0xFF) {
    //     // 暫定処理として、ASCII範囲外の末尾の空間はまるごと2バイト文字と推定する
    //     // NOTE: ShiftJIS本来は、0xE0から0xEFまで。
    //     return 2;
    // } else {
    //     // Not reached.
    //     return 1;
    // }
    return sjis_is_first_byte((uint8_t)sjis[0]) ? 2 : 1;
}


uint16_t count_chars_sjis(const char* sjis) {
    uint16_t total_chars = 0;
    const char* ptr = sjis;
    uint16_t sjislen = strlen(sjis);
    const char* tail = sjis + sjislen;

    if (sjislen == 0) {
        // 長さ0なので、0文字
        return 0;
    }

    while (ptr != tail) {
        uint8_t curbytes = count_bytes_of_a_char_sjis(ptr);
        total_chars += 1;
        ptr += curbytes;
    }
    return total_chars;
}


const char* get_n_char_ptr_sjis(const char* sjis, uint8_t count) {
    uint8_t current_chars = 0;
    const char* cur = sjis;
    uint16_t sjislen = strlen(sjis);
    const char* tail = sjis + sjislen;
    // uint16_t total_chars = count_chars_sjis(sjis);

    while ((current_chars != count) && (cur != tail)) {
        uint8_t curbytes = count_bytes_of_a_char_sjis(cur);
        cur += curbytes;
        current_chars += 1;
    }

    return cur;
}


const char* get_last_n_char_ptr_sjis(const char* sjis, uint8_t count) {
    uint16_t sjislen = strlen(sjis);
    if (sjislen == 0) {
        return sjis;
    }
    uint16_t current_chars = 0;
    const char* cur = sjis;
    const char* tail = sjis + sjislen;
    uint16_t total_chars = count_chars_sjis(sjis);
    uint16_t count_from_head = total_chars - count;

    while ((current_chars != count_from_head) && (cur != tail)) {
        uint8_t curbytes = count_bytes_of_a_char_sjis(cur);
        cur += curbytes;
        current_chars += 1;
    }

    return cur;
}


/** ShiftJIS表現の次の文字へのポインタを取得する
  @param sjis [IN]
  @return 次の文字の先頭バイトへのポインタ。次の文字がないか不正なバイト列の場合はnullptr
  */
char* get_next_char_ptr_sjis(char* sjis) {
    if (sjis[0] == '\0') {
        return nullptr;
    }
    uint8_t curchar_bytelen = count_bytes_of_a_char_sjis(sjis);
    char* ptr = &sjis[curchar_bytelen];
    if (*ptr == '\0') {
        return nullptr;
    }
    return ptr;
}

bool convert_mb_to_kuten_sjis(const char* sjis, uint8_t* ku, uint8_t* ten) {
    uint8_t b0 = sjis[0];
    uint8_t b1 = sjis[1];

    uint8_t chbytes = count_bytes_of_a_char_sjis(sjis);
    if (chbytes == 1) {
        *ku = 0;
        *ten = b0;
        return true;
    }

    uint8_t _ku, _ten;

    // ShiftJISのバイト表現上の1バイト目は、
    // ひとつのバイトで2区画分を収容している
    _ku = b0;
    if (0x81 <= _ku && _ku <= 0x9F) {
        // この範囲で合計62区分を収容する
        _ku = _ku - 0x81;
    } else if (0xE0 <= _ku) {
        // この範囲で63区以降を収容する
        _ku = _ku - 0xE0 + 62 / 2;
    } else {
        _ku = 0xFF / 2;
    }
    // 区番号へ展開し、1始まりに直す
    _ku = _ku * 2 + 1;

    _ten = b1;
    if (_ten >= 0x9F) {
        // 偶数区
        _ku += 1;
        _ten = _ten - 0x9F + 1;
    } else {
        if (_ten >= 0x80) {
            _ten = _ten - 0x80 + 63 + 1;
        } else {
            _ten = _ten - 0x7E + 63;
        }
    }
    *ku = _ku;
    *ten = _ten;

    return true;
}


const char* get_prev_char_ptr_sjis(const char* sjis, const char* head) {

    const char* prev_1 = sjis - 1;
    const char* prev_2 = sjis - 2;

    if (' ' <= *prev_1 && *prev_1 < '/') {
        // ASCII文字なので、ここで確定
        return (const char*)prev_1;
    }

    if (count_bytes_of_a_char_sjis(prev_2) == 2) {
        return (const char*)prev_2;

    } else {
        return (const char*)prev_1;
    }

    // DEBUG("get_prev_char_ptr_sjis(): Invalid SJIS string ?");
    // return nullptr;

}



void convert_hiragana_to_katakana_sjis(const char* sjis, char* dst) {
    // DEBUG("Called. src=%p, dst=%p", sjis, dst);
    const char* tail = sjis + strlen(sjis);
    while (sjis != tail) {
        uint8_t curcharbytes = count_bytes_of_a_char_sjis(sjis);
        if (curcharbytes == 1) {
            // ASCII文字なので変換外
            *dst = *sjis;
            ++sjis;
            ++dst;
            continue;
        } else {
            uint8_t ku, ten;
            if (!convert_mb_to_kuten_sjis(sjis, &ku, &ten)) {
                DEBUG("convert_hiragana_to_katakana_sjis() encouted invalid ShiftJIS bytes.");
                return;
            }
            if (ku == 4) {
                // ひらがなは4区に入っている。これを5区にする
                ku = 5;
                convert_kuten_2_sjis(ku, ten, dst);
                sjis += curcharbytes;
                dst += curcharbytes;
            } else {
                // ひらがなではないのでそのままコピー
                *dst++ = *sjis++;
                *dst++ = *sjis++;
            }
        }
    }
}


void convert_kuten_2_sjis(uint8_t ku, uint8_t ten, char* sjis) {
    if (ku == 0) {
        sjis[0] = ten;
        return;
    }

    if (ku < 63) {
        sjis[0] = 0x81 + (ku - 1) / 2;
    } else {
        sjis[0] = 0xE0 + (ku - 63) / 2;
    }

    if (ku & 0x01) {
        if (ten < 64) {
            sjis[1] = 0x40 + (ten - 1);
        } else {
            sjis[1] = 0x80 + (ten - 64);
        }
    } else {
        sjis[1] = 0x9F + (ten - 1);
    }
}

void sjis_remove_tail_char(char* sjis) {
    char* tailcharptr = (char*)get_last_n_char_ptr_sjis((const char*)sjis, 1);
    uint8_t tailcharbytes = count_bytes_of_a_char_sjis(tailcharptr);
    memset(tailcharptr, '\0', tailcharbytes);
}
