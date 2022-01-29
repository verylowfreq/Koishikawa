#include "panic.h"

#include <stdint.h>
#include <stdbool.h>

#include <Arduino.h>

#include "screenex.h"
#include "font.h"

// -- main.cppにて定義されている変数 --

extern ScreenEx screen;
extern FontManager font;


void panic(const char* modulename, const char* functionname, int lineno, const char* mes) {
    // シリアル出力
    //    ex) "PANIC! main.cpp#123 setup(): failed to initialize input module."
    Serial.print("PANIC!: ");
    Serial.print(modulename);
    Serial.print("#");
    Serial.print((int)lineno, DEC);
    Serial.print(functionname);
    Serial.print("(): ");
    Serial.println(mes);

    // 可能ならば画面出力
    screen.set_cursor(0, 0);
    screen.print("PANIC L");
    screen.print((int)lineno, DEC);
    screen.print(" ");
    screen.print(functionname);
    screen.set_cursor(0, font.FONT_HEIGHT);
    screen.print(mes);

    Serial.println("Halted by panic.");

    while (true) {
        delay(100);
    }
}
