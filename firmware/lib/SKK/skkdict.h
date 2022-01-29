#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <FileAccessWrapper.h>

namespace SKK {

    class CandidateReader;


    /** SKK辞書ファイルを読み書きし、変換辞書に対する抽象的操作を実現するためのクラス。
     * ファイルシステム依存の操作はSkkDictFileへ、変換全体の工程はSkkEngineが担う。
     */
    class SkkDict {
    // private:
    public:
        FileAccessWrapper* file = nullptr;
        uint32_t filesize = 0;
        uint16_t yomiganamaxlen = 0;
        uint32_t index_head = 0;
        uint32_t index_tail = 0;
        uint32_t table_head = 0;
        uint32_t table_tail = 0;
        char* yomiganabuffer = nullptr;

    public:
        bool init(FileAccessWrapper* file);

        void load_headers(void);

        /** 指定された読み仮名に対応する検索開始アドレスを取得する
         * @return 対応するアドレスが見つからなかったらINVALID_UINT32、見つかればそのアドレス ( < INVALID_UINT32 )
         */
        uint32_t search_startaddr_from_index_for(const char* yomigana, size_t yomiganalen, uint8_t* comparelen_on_abort);

        /** 指定された読み仮名に対応する変換候補を取得する
         * @param startaddr [IN] 検索を開始するアドレス
         * @param allow_abort [IN] 検索を途中で打ち切ることを許可するか否か
         * @param compare_chars [IN] 検索打ち切りの際の、先頭から比較するバイト数
         * @param yomigana [IN]
         * @param yomiganalen [IN]
         * @param reader [OUT] 変換候補のリーダー
         * @return 変換候補が見つかればtrue、見つからなければfalse
         */
        bool search_henkanentry_for(uint32_t startaddr, bool allow_abort, int compare_bytes, const char* yomigana, size_t yomiganalen, CandidateReader* reader);
    };
}
