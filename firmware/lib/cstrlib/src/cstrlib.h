#pragma once

#include <stddef.h>
#include <stdint.h>


/** 末尾に文字列を追加する
  @param cstr [IN/OUT]
  @param cstrmaxlen [IN] cstrの最大長（終端のNULを除くバイト数）
  @param src [IN]
  @param len [IN]
  @return 新しい長さ
  */
size_t cstr_append(char* cstr, size_t cstrmaxlen, const char* src, size_t len);


/** 末尾に1文字（1バイト）を追加する
  @param cstr [IN/OUT]
  @param cstrmaxlen [IN] cstrの最大長（終端のNULを除くバイト数）
  @param src [IN]
  @return 追加出来たらtrue
  */
bool cstr_append_char(char* cstr, size_t cstrmaxlen, char ch);


/** 指定のcountバイトだけ要素を削除し、空き領域を詰める
  @param cstr [IN/OUT] 操作対象の文字列（NUL終端のメモリ領域）
  @param from_head [IN] 先頭から削除したい要素数（バイト数）
  @param from_end [IN] 末尾から削除したい要素数（バイト数）
  @return 文字列の新しい長さ（バイト数）
  */
size_t cstr_remove(char* cstr, size_t from_head, size_t from_end);


/** アルファベットを1文字追加する
 * - TODO: `romajibuffer` の長さをきちんとチェックする
 * @param ch [IN]
 * @return 追加後の `romajibuffer` のバイト数
 */
// uint16_t push_romaji(char ch);
