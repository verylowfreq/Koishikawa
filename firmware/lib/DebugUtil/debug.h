#pragma once

// デバッグ用のメソッドを宣言するヘッダー
// 関数の実態は各プラットフォームのメインのコードで定義する。


#if !defined(SUPRESS_DEBUG_MESSAGE) || !(SUPRESS_DEBUG_MESSAGE)

#include <stdint.h>

#include <commondef.h>

// NOTE: 各プラットフォームのコードで定義しなければならない

void debug_printf(const char* format, ...);

/**
 * @brief バイト列を16進数で出力する
 * @param data [IN]
 * @param len  [IN]
 */
void dump_bytes_hex(const uint8_t* data, uint8_t len);

/**
 * @brief バイト列を10進数で出力する
 * @param data [IN]
 * @param len  [IN]
 */
void dump_bytes_dec(const uint8_t* data, uint8_t len);


// デバッグ出力へのprintf()
#define DEBUG_PRINTF(...)  debug_printf(__VA_ARGS__)


// デバッグメッセージのヘッダーを出力する
#define DEBUG_HEADER() \
        DEBUG_PRINTF("%s %s:%d %s(): ", "[DEBUG]", __FILE__, __LINE__, __func__)

// デバッグメッセージをヘッダー付きで出力し、改行する
#define DEBUG(...) \
    do { \
        DEBUG_HEADER(); \
        DEBUG_PRINTF("" __VA_ARGS__); \
        DEBUG_PRINTF("\n"); \
    } while (false)



// 実行時間計測用のマクロ。ブロックを作るので、変数のスコープに注意。
// プラットフォーム側のコードで、millis()を定義する必要がある

extern "C" {
    unsigned long millis(void);
}

#define STOPWATCH_BLOCK_START(name) \
    do { \
        unsigned long __stopwatch_millis_ ## name = millis()

#define STOPWATCH_BLOCK_END_PRINTIFGREATER(name, time_msec) \
        unsigned long __elapsed_stopwatch_millis_ ## name = millis() - __stopwatch_millis_ ## name ; \
        if (__elapsed_stopwatch_millis_ ## name >= time_msec) { \
            DEBUG_HEADER(); \
            DEBUG_PRINTF(" Stopwatch \"%s\" %d[msec]\n", #name, __elapsed_stopwatch_millis_ ## name); \
        } \
    } while (false)

#define STOPWATCH_BLOCK_END(name) STOPWATCH_BLOCK_END_PRINTIFGREATER(name, 0)


#else

#define DEBUG_PRINTF(...) ((void)0)
#define DEBUG(...) ((void)0)

#define STOPWATCH_BLOCK_START(name)  {
#define STOPWATCH_BLOCK_END(name)    }

#endif
