#pragma once

#include <string.h>

#include <commondef.h>


// 続行不能エラーをできるだけ出力するメソッドを宣言するだけのヘッダー
// 関数の実態は各プラットフォームのメインのコードで定義する。

// #define __PANIC_DIR_SEPARATOR '/'
// #define __PANIC_FILENAME  (strrchr(__FILE__, __PANIC_DIR_SEPARATOR) ? strrchr(__FILE__, __PANIC_DIR_SEPARATOR) + 1 : __FILE__)


/** 続行不能なエラーが発生したときに、できるだけそれを出力し、実行を中止する。
 * @param modulename
 * @param functionname
 * @param lineno
 * @param mes エラーの説明
 */
void panic(const char* modulename, const char* functionname, int lineno, const char* mes);


#define PANIC(mes) \
    do { \
        panic(__FILENAME__, __func__, __LINE__, mes); \
    } while (false)
