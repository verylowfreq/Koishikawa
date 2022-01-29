#include "cstrlib.h"

#include <assert.h>
#include <string.h>


/** 末尾に追加する
  @param cstr [IN/OUT]
  @param cstrmaxlen [IN] cstrの最大長（終端のNULを除くバイト数）
  @param src [IN]
  @param len [IN]
  @return 新しい長さ
  */
size_t cstr_append(char* cstr, size_t cstrmaxlen, const char* src, size_t len) {
    assert(cstr);
    size_t cstr_len = strlen(cstr);
    // size_t append_len = strlen(src);
    size_t append_len = len;
    if (append_len < 1) {
        // 追加する文字列が空の場合
        return cstr_len;
    }
    if (cstrmaxlen < cstr_len + append_len) {
        // 文字列を追加するとがバッファの長さを超える場合、収まる分だけ追加する
        // D("Warn cstr_append() buffer length not enough. (require %d bytes but %d bytes presented.)\n", cstr_len + append_len, cstrmaxlen);
        append_len = cstrmaxlen - cstr_len;
    }
    memmove(&cstr[cstr_len], src, append_len);
    size_t newlen = cstr_len + append_len;
    cstr[newlen] = '\0';
    return newlen;
}


/** 末尾に追加する
  @param cstr [IN/OUT]
  @param cstrmaxlen [IN] cstrの最大長（終端のNULを除くバイト数）
  @param src [IN]
  @return 追加出来たらtrue
  */
bool cstr_append_char(char* cstr, size_t cstrmaxlen, char ch) {
    assert(cstr);
    size_t cstr_len = strlen(cstr);
    if (cstr_len == cstrmaxlen) {
        return false;
    } else {
        cstr[cstr_len] = ch;
        cstr[cstr_len + 1] = '\0';
        return true;
    }
}

/** 指定のcountバイトだけ要素を削除し、空き領域を詰める
  @param cstr [IN/OUT] 操作対象の文字列（NUL終端のメモリ領域）
  @param from_head [IN] 先頭から削除したい要素数（バイト数）
  @param from_end [IN] 末尾から削除したい要素数（バイト数）
  @return 文字列の新しい長さ（バイト数）
  */
size_t cstr_remove(char* cstr, size_t from_head, size_t from_end) {
    assert(cstr);
    size_t totallen = strlen(cstr);
    
    if (totallen <= from_head || totallen <= from_end) {
        // 長さを超える要素数の削除が要求されていたら、空文字列にして終了する
        cstr[0] = '\0';
        return 0;
    }

    if (from_head > 0) {
        size_t _len = totallen - from_head;
        memmove(cstr, &cstr[from_head], _len);
    }
    size_t tail = totallen - from_head - from_end;
    cstr[tail] = '\0';

    return strlen(cstr);
}

/** アルファベットを1文字追加する
 * - TODO: `romajibuffer` の長さをきちんとチェックする
 * @param ch [IN]
 * @return 追加後の `romajibuffer` のバイト数
 */
// uint16_t push_romaji(char ch) {
//     uint16_t romajibuffer_bytelen = strlen(romajibuffer);
//     romajibuffer[romajibuffer_bytelen] = ch;
//     romajibuffer[romajibuffer_bytelen+1] = '\0';
//     return strlen(romajibuffer);
// }


