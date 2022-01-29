#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include <unity.h>

#include <debug.h>



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


void test_debug_printf_1(void) {

    printf("This test cannot auto assertion. Please look with your eyes.\n");

    const char* test_str_1 = "HELLO, WORLD %d %ld";

    printf("Reference: \"");
    printf(test_str_1, 123, 123L);
    printf("\"\n");
    printf("Target   : \"");
    debug_printf(test_str_1, 123, 123L);
    printf("\"\n");

    TEST_ASSERT(true);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_debug_printf_1);

    return UNITY_END();    
}
