#pragma once

#include <debug.h>

#include <unity.h>

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

// --- Required by debug.h

void debug_printf(const char* format, ...) {
    constexpr size_t buflen = 255;
    char buf[buflen];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, buflen, format, args);
    printf("%s", buf);

    va_end(args);
}

unsigned long millis(void) {
    return 0;
}

void panic(char const* filename, char const* funcname, int lineno, char const* mes) {
    printf("panic: %s, %s() %d, %s\n", filename, funcname, lineno, mes);
    TEST_ASSERT(false);
}
