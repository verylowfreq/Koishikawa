#pragma once

#include <stdint.h>
#include <stdbool.h>


/** そのバイトがSJISの第1バイト目かを判定する
 * @param ch [IN]
 * @param 第1バイト目ならtrue
 */
bool sjis_is_first_byte(uint8_t ch);


/** 文字列の先頭文字のバイト数を取得する
 * @param sjis [IN]
 * @return 文字数
 */
uint8_t count_bytes_of_a_char_sjis(const char* sjis);

/**
 * @brief SJIS文字列の文字数を数える
 * @param sjis [IN] NUL終端のSJIS文字列
 * @return 文字数（バイト数ではない）
 */
uint16_t count_chars_sjis(const char* sjis);

/**
 * @brief SJIS文字列の、先頭からn文字目へのポインタを取得する
 * @param sjis  [IN] NUL終端のShiftJIS文字列
 * @param count [IN] 後ろから数えた文字数
 * @return 該当する文字へのポインタ、もしくは終端NULへのポインタ
 */
const char* get_n_char_ptr_sjis(const char* sjis, uint8_t count);

/**
 * @brief SJIS文字列の、末尾からn文字目へのポインタを取得する
 * @param sjis  [IN] NUL終端のShiftJIS文字列
 * @param count [IN] 後ろから数えた文字数
 * @return 該当する文字へのポインタ、もしくは先頭の文字、長さ0ならば終端NUL
 */
const char* get_last_n_char_ptr_sjis(const char* sjis, uint8_t count);

/** ShiftJIS表現の次の文字へのポインタを取得する
  @param sjis [IN]
  @return 次の文字の先頭バイトへのポインタ。次の文字がないか不正なバイト列の場合はnullptr
  */
char* get_next_char_ptr_sjis(char* sjis);

/**
 * @brief SJIS文字列の、末尾からn文字目へのポインタを取得する
 * @param sjis [IN] NUL終端のShiftJIS文字列
 * @param head [IN] 文字列の先頭ポインタ
 * @return 該当する文字へのポインタ、もしくは先頭の文字、長さ0ならば終端NUL
 */
const char* get_prev_char_ptr_sjis(const char* sjis, const char* head);


bool convert_mb_to_kuten_sjis(const char* sjis, uint8_t* ku, uint8_t* ten);

void convert_kuten_2_sjis(uint8_t ku, uint8_t ten, char* sjis);


void convert_hiragana_to_katakana_sjis(const char* sjis, char* dst);

/** 末尾の1文字を消す
 * @param sjis [IN/OUT]
 */
void sjis_remove_tail_char(char* sjis);

// void convert_katanaka_to_hiragana_sjis(const char* sjis, char* dst);

#if false
#define count_bytes_of_a_char(mbstr) count_bytes_of_a_char_sjis(mbstr)
#define get_next_char_ptr(mbstr) get_next_char_ptr_sjis(mbstr)
#define convert_mb_to_kuten(mbstr, ku, ten) convert_mb_to_kuten_sjis(mbstr, ku, ten)
#endif
