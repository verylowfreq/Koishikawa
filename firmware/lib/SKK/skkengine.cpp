#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <debug.h>

#include <skkdict.h>
#include <candidatereader.h>
#include <skkengine.h>



using namespace SKK;

bool SkkEngine::init(void) {
    return true;
}

bool SkkEngine::set_userdict(SkkDict* dict) {
    assert(dict);
    this->userdict = dict;
    return true;
}

bool SkkEngine::set_sysdict(SkkDict* dict) {
    assert(dict);
    this->systdict = dict;
    return true;
}


/** 指定の辞書を使って変換候補を探す
 * @return 変換候補が１つ以上見つかったらtrue
 */
bool henkan_with_dict(const char* yomigana, size_t yomiganalen, CandidateReader* candidates, SkkDict* dict, bool allow_abort) {
    uint8_t index_comparelen;
    // unsigned long searchaddr_timer = millis();
    uint32_t indexedaddress;

    STOPWATCH_BLOCK_START(search_startaddr_from_index_for);
    indexedaddress = dict->search_startaddr_from_index_for(yomigana, yomiganalen, &index_comparelen);
    STOPWATCH_BLOCK_END(search_startaddr_from_index_for);

    if (indexedaddress == INVALID_UINT32) {
        DEBUG("No matching index entry found.");
        return false;
    }
    // unsigned long search_henkanentry_timer = millis();
    bool result;
    STOPWATCH_BLOCK_START(search_henkanentry_for);
    result = dict->search_henkanentry_for(indexedaddress, allow_abort, index_comparelen, yomigana, yomiganalen, candidates);
    STOPWATCH_BLOCK_END(search_henkanentry_for);
    // unsigned long elapsed_search_henkanentry = millis() - search_henkanentry_timer;
    // DEBUG("%lu[msec] elapsed in search_henkanentry() execution.", elapsed_search_henkanentry);

    return result;
}

/** 読み仮名から変換を実行し、変換候補を探す
 * @param yomigana [IN] 
 * @param yomiganalen [IN]
 * @param candidates [OUT]
 * @return 変換候補が１つ以上見つかったらtrue
 */
bool SkkEngine::henkan(const char* yomigana, size_t yomiganalen, CandidateReader* candidates) {
    bool result;

    // DEBUG_PRINTF("--------\n");
    // DEBUG_PRINTF("henkan() for (%d bytes) ", yomiganalen);
    // dump_bytes_hex((const uint8_t*)yomigana, yomiganalen);
    // DEBUG_PRINTF("\n");

    STOPWATCH_BLOCK_START(henkan);

    if (this->userdict && henkan_with_dict(yomigana, yomiganalen, candidates, this->userdict, false)) {
        DEBUG("Found in User dict.");
        result = true;

    } else if (this->systdict && henkan_with_dict(yomigana, yomiganalen, candidates, this->systdict, true)) {
        DEBUG("Found in System dict.");
        result = true;

    } else {
        DEBUG("Nothing found in any dict.");
        result = false;
    }

    // DEBUG_HEADER();
    STOPWATCH_BLOCK_END(henkan);

    // DEBUG_PRINTF("--------\n");

    return result;
}

/** 読み仮名から変換を実行し、変換候補を探す
 * @param yomigana [IN] ゼロ終端の読み仮名の文字列
 * @param candidates [OUT]
 * @return 変換候補が１つ以上見つかったらtrue
 */
bool SkkEngine::henkan(const char* yomigana, CandidateReader* candidates) {
    return this->henkan(yomigana, strlen(yomigana), candidates);
}
