#pragma once

#include "skkdict.h"

#include <commondef.h>
#include <debug.h>


namespace SKK {

    /** 変換候補を順番に取り出すためのクラス
     */
    class CandidateReader {
    public:
        SkkDict* parentDict;
        uint8_t candidates_count = 0;
        uint16_t candidateslen = 0;
        uint32_t startaddr = INVALID_UINT32;
        uint32_t current_candidate_head = INVALID_UINT32;
        uint8_t current_candidate_len = 0;
        uint8_t current_remains = 0;
        uint8_t current_candidate_count = 0;

        /**
         * NOTE: 内部で読み込み動作をする。
         * @param parent [IN] 変換候補を収録している辞書へのポインタ
         * @param candidatescnt [IN] 変換候補の個数
         * @param candidateslen [IN] 変換候補の総バイト数 （終端NULを含まない）
         * @param startaddr [IN] 最初の変換候補の先頭アドレス（このアドレス位置に最初の変換候補のバイト数がある）
         */
        void init(SkkDict* parent, uint8_t candidatescnt, uint16_t candidateslen, uint32_t startaddr);

        uint8_t get_candidates_count(void);

        // int get_next_candidate_length(void) {
        //     return this->candidateslen;
        // }

        uint16_t get_current_candidate_length(void);

        int read(void);

        bool move_next(void);

        bool move_head(void);

        bool is_reached_end(void);
    };
}
