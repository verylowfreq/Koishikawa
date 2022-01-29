#include <debug.h>

#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#include <Arduino.h>


/** デバッグ出力用に、シリアルへ文字列を書き出す関数。
 * ArduinoのPrintクラスにvprintf系がないので、固定長バッファを経由して書き出す。
 * @param format 
 * @param ... 
 */
void debug_printf(const char* format, ...) {
    constexpr int SERIAL_PRINTF_BUFFER_LEN = 255;
    char serial_printf_buffer[SERIAL_PRINTF_BUFFER_LEN];
    va_list ap;
    va_start(ap, format);
    vsnprintf(serial_printf_buffer, SERIAL_PRINTF_BUFFER_LEN, (const char*)format, ap);
    Serial.print(serial_printf_buffer);
    va_end(ap);
}
