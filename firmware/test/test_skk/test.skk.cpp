#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unity.h>

// Impl for debug utils
#include "../debug_impl.h"

#include <FileAccessWrapper.h>

// Implement of FileAccessWrapper in Host PC.
#include "../CstdioFileAccessor.h"

#include <skkdict.h>
#include <skkengine.h>

CstdioFileAccessor dictfile;
SKK::SkkDict skkdict;
SKK::SkkEngine skkengine;

// NOTE: test is executed on the root of this project.
const char* FILEPATH_TEST_skkdict = "test/test_skk/test_skkdict.skd";

void test_skk_1(void) {
    SKK::CandidateReader reader;

    TEST_ASSERT_TRUE(dictfile.open(FILEPATH_TEST_skkdict, FileAccessWrapper::FileMode::READ));
    TEST_ASSERT_TRUE(skkdict.init(&dictfile));
    TEST_ASSERT_TRUE(skkengine.init());
    TEST_ASSERT_TRUE(skkengine.set_sysdict(&skkdict));
    // "こくみん" in ShiftJIS
    unsigned char hiragana[] = { 0x82, 0xb1, 0x82, 0xad, 0x82, 0xdd, 0x82, 0xf1 };
    TEST_ASSERT_TRUE(skkengine.henkan((const char*)hiragana, 8, &reader));
    char buf[128];
    memset(buf, 0x00, 128);

    int ch;
    for (int i = 0; i < 128 && (ch = reader.read()) >= 0; i++) {
        buf[i] = (uint8_t)ch;
    }

    // "国民" in ShiftJIS
    unsigned char ref_buf[] = { 0x8d, 0x91, 0x96, 0xaf };
    TEST_ASSERT(reader.get_candidates_count() == 1);
    TEST_ASSERT(memcmp(buf, ref_buf, 4) == 0);

    // "んじゃめな" in ShiftJIS. Should not exist in SKK dictionary.
    unsigned char test2_buf[] = { 0x82,0xf1,0x82,0xb6,0x82,0xe1,0x82,0xdf,0x82,0xc8 };
    TEST_ASSERT_FALSE(skkengine.henkan((const char*)test2_buf, sizeof(test2_buf), &reader));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_skk_1);

    return UNITY_END();    
}
