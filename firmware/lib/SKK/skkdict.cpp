#include <assert.h>
#include <stdlib.h>

#include <debug.h>
#include <panic.h>
#include "skkdict.h"
#include "candidatereader.h"
#include <FileAccessWrapper.h>
#include <commondef.h>


using namespace SKK;

bool SkkDict::init(FileAccessWrapper* file) {
    assert(file);
    if (!file->is_opened()) {
        return false;
    }
    this->file = file;
    this->load_headers();
    this->yomiganabuffer = (char*)malloc(this->yomiganamaxlen + 1);
    assert(this->yomiganabuffer);

    return true;
}

void SkkDict::load_headers(void) {
    this->file->seek(0);
    // if (this->file->read() != 'S' || this->file->read() != 'K' || this->file->read() != 'D') {
        // assert("SKD" == nullptr);
        // No check for magic code for versatile use.
    // }
    char magicnumber[3] = { (char)this->file->read(), (char)this->file->read(), (char)this->file->read() };
    this->filesize = this->file->read_uint24();
    DEBUG("filesize=%ld", this->filesize);

    uint16_t commentlen = 0;
    commentlen = this->file->read_uint16();
    // DEBUG("commentlen=%d\n", commentlen);
    this->file->seek_delta(commentlen);  // Skip comment body
    this->yomiganamaxlen = this->file->read_uint16();
    // DEBUG("yomiganamaxlen=%d\n", this->yomiganamaxlen);

    if (this->file->read() != 'I' || this->file->read() != 'D' || this->file->read() != 'X') {
        // assert("IDX" == nullptr);
        PANIC("'IDX' not found.");
    }
    uint32_t indexlen = 0;
    indexlen = this->file->read_uint24();
    this->index_head = this->file->position();  // Here is the head of the index body.
    this->index_tail = this->index_head + indexlen;
    this->file->seek_delta(indexlen);  // Skip index body
    DEBUG("index_head=0x%lx(%ld)", this->index_head, this->index_head);

    if (this->file->read() != 'T' || this->file->read() != 'B' || this->file->read() != 'L') {
        // assert("TBL" == nullptr);
        PANIC("'TBL' not found.");
    }
    uint32_t tablelen = 0;
    tablelen = this->file->read_uint24();
    this->table_head = this->file->position();
    if (tablelen > 0) {
        this->table_tail = this->table_head + tablelen;
    } else if (this->filesize > 0) {
        this->table_tail = this->filesize;
    } else {
        this->table_tail = INVALID_UINT32;
    }
}

/** 指定された読み仮名に対応する検索開始アドレスを取得する
 * @return 対応するアドレスが見つからなかったらINVALID_UINT32、見つかればそのアドレス ( < INVALID_UINT32 )
 */
uint32_t SkkDict::search_startaddr_from_index_for(const char* yomigana, size_t yomiganalen, uint8_t* comparelen_on_abort) {
    this->file->seek(this->index_head);

    while (this->file->position() < this->index_tail) {
        // 現在のインデックス項目の読み仮名の長さ（バイト数）
        uint8_t targetyomiganalen = this->file->read_uint8();
        for (uint8_t i = 0; i < targetyomiganalen; ++i) {
            this->yomiganabuffer[i] = (uint8_t)this->file->read();
        }
        // この索引が示すアドレスの値
        uint32_t jumpaddr = this->file->read_uint24();

        // TODO: ここのヒット条件が緩い？
        //       1文字の検索がかなり早い段階でヒットしてしまっている気がする。（先頭1文字が一致した時点でヒット判定？）
        // ヒット条件： 検索対象ひらがなの長さと、いま検討している索引のキーの長さが、同じか、検索対象ひらがなのほうが長い
        //                かつ、
        //              いま検討している索引のキーの長さの範囲で、両者が一致する
        
        if ((yomiganalen >= targetyomiganalen) && (memcmp(yomigana, this->yomiganabuffer, targetyomiganalen) == 0)) {
            // ヒットした
            // この索引のキーの長さを変数へ格納する
            *comparelen_on_abort = targetyomiganalen;
            return jumpaddr;

        } else {
            // 違うので検索を続行
            continue;
        }

        // if (targetyomiganalen > yomiganalen ||
        //     memcmp(this->yomiganabuffer, yomigana, targetyomiganalen) != 0) {
        //     // Not hit
        //     continue;
        // } else {
        //     // Hit
        //     // DEBUG("found index at %ld(0x%lx)", this->file->position(), this->file->position());
        //     *comparelen_on_abort = targetyomiganalen;
        //     return jumpaddr;
        // }
    }
    DEBUG("not found index. reached to %ld(0x%lx)", this->file->position(), this->file->position());
    return INVALID_UINT32;
}


bool SkkDict::search_henkanentry_for(uint32_t startaddr, bool allow_abort, int compare_bytes, const char* yomigana, size_t yomiganalen, CandidateReader* reader) {
    if (0 < startaddr && startaddr < INVALID_UINT32) {
        this->file->seek(startaddr);
    } else {
        this->file->seek(this->table_head);
    }

    while (this->file->position() < this->table_tail) {
        uint8_t cur_yomiganalen = this->file->read_uint8();
        bool is_disabled = cur_yomiganalen & 0x80;
        for (uint8_t i = 0; i < cur_yomiganalen; ++i) {
            this->yomiganabuffer[i] = (uint8_t)this->file->read();
        }
        uint8_t candidatecount = this->file->read_uint8();
        uint16_t candidatelen = this->file->read_uint16();
        uint32_t cur_addr = this->file->position();

        if (cur_yomiganalen == yomiganalen && memcmp(this->yomiganabuffer, yomigana, yomiganalen) == 0) {
            // Hit
            // DEBUG("Found at %ld, count=%d, len=%d, startaddr=%ld", cur_addr, candidatecount, candidatelen, startaddr);
            reader->init(this, candidatecount, candidatelen, cur_addr);
            return true;

        } else {
            // Not hit
            // DEBUG("Not matched at %ld, count=%d, len=%d, startaddr=%ld", cur_addr, candidatecount, candidatelen, startaddr);

            if (allow_abort) {
                bool do_abort = false;
                // if (compare_chars > 2) {
                //     // 先頭2文字の比較
                //     do_abort = memcmp(this->yomiganabuffer, yomigana, 4) != 0;
                // } else {
                //     // 先頭1文字の比較
                //     do_abort = memcmp(this->yomiganabuffer, yomigana, 2) != 0;
                // }
                // if (cur_yomiganalen < compare_bytes) {
                //     do_abort = true;
                // } else {
                do_abort = memcmp(this->yomiganabuffer, yomigana, compare_bytes) != 0;
                // }

                if (do_abort) {
                    // Abort
                    DEBUG("Not found. Abort. reached to %d(0x%x)\n",this->file->position(), this->file->position());
                    return false;
                }
            }

            // if (allow_abort && this->yomiganabuffer[0] != yomigana[0]) {
            //     // Abort
            //     DEBUG("Not found. Abort. reached to %d(0x%x)", this->file->position(), this->file->position());
            //     return false;
            // }

            // this->file->seek_delta(candidatelen + 1);  // Skip candidates and termination NUL
            this->file->seek_delta(candidatelen);  // Skip current candidates
        }
    }
    DEBUG("Not found. reached to %d(0x%x)", this->file->position(), this->file->position());
    return false;
}
