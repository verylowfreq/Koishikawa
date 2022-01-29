#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <skkdict.h>
#include <candidatereader.h>

namespace SKK {

    class SkkEngine {
    public:
        SkkDict* userdict = nullptr;
        SkkDict* systdict = nullptr;


        bool init(void);

        /** ユーザー辞書を設定する（任意）
         * TODO: 辞書へ追記できるようにする。
         * @return 成功したらtrue
         */
        bool set_userdict(SkkDict* dict);

        /** システム辞書を設定する（必須）。読み取り専用
         * @return 成功したらtrue
         */
        bool set_sysdict(SkkDict* dict);


        /** 読み仮名から変換を実行し、変換候補を探す
         * @param yomigana [IN] 
         * @param yomiganalen [IN]
         * @param candidates [OUT]
         * @return 変換候補が１つ以上見つかったらtrue
         */
        bool henkan(const char* yomigana, size_t yomiganalen, CandidateReader* candidates);
        
        /** 読み仮名から変換を実行し、変換候補を探す
         * @param yomigana [IN] ゼロ終端の読み仮名の文字列
         * @param candidates [OUT]
         * @return 変換候補が１つ以上見つかったらtrue
         */
        bool henkan(const char* yomigana, CandidateReader* candidates);
    };
}
