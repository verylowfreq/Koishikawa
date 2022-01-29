#include <Arduino.h>

#include <assert.h>

#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#include <debug.h>
#include <panic.h>
#include "keyboard.h"
#include "screen.h"
#include "font.h"
#include <candidatereader.h>
#include <ArduinoSDFileAccessor.h>>
#include <skkdict.h>
#include <skkengine.h>

#include <cstrlib.h>
#include <sjis.h>

#include "inputengine.h"
#include "screenex.h"


#define PIN_SD_CS 7
// #define SPI_SD_CLOCK (1000000UL)
#define SPI_SD_CLOCK (400000UL)

const char* FILEPATH_FONT8 = "FONT8JIS.FNT";
const char* FILEPATH_FONT14 = "FNT14JIS.FNT";
const char* FILEPATH_SYSDICT = "SYSDICT.SKD";
const char* FILEPATH_USERDICT = "USERDICT.SKD";

const char* FILEPATH_SJGB18TABLE = "CNVSJGB.TBL";

ArduinoSDFileAccessor font14file;

ScreenEx screen;
FontManager font;

constexpr uint16_t FONTCACHEBUFFER_LENGTH = (1 + 1 + 28) * 16;
byte fontcachebuffer[FONTCACHEBUFFER_LENGTH];

ArduinoSDFileAccessor sysDictFile;
SKK::SkkDict sysDict;
SKK::SkkEngine skk;

ArduinoSDFileAccessor convert_sjis_gb18030_table_file;
SKK::SkkDict convert_sjis_gb18030_table;

InputEngine inputLine;

/* 編集中ドキュメントの文字列を記憶するバッファ
   FIXME:簡易化のために、固定配列のバッファをそのまま用いる
*/
#define TEXTBUFFER_LENGTH 256
char textbuffer[TEXTBUFFER_LENGTH];

/* 入力系バッファは以下の2つ。スクリーン上では単純に連結して表示する。
 */

constexpr int HENKANBUFFER_LENGTH = 32;
// ひらがなから漢字への変換待ちの文字を記憶するためのバッファ
char henkanbuffer[HENKANBUFFER_LENGTH];

constexpr int ROMAJIBUFFER_LENGTH = 6;
// アルファベットからひらがなへ変換待ちの文字を記憶するためのバッファ
char romajibuffer[ROMAJIBUFFER_LENGTH];



void draw_texts(bool update_textbuffer);


/* メモ：
  漢字変換は、
   1. ひらがな（送り仮名英字含む）で単純一致を探す
   2. 見つかった候補を列挙する
   3. 候補から一つを選ぶ
   4. 文字列置き換えする
     a. 送り仮名なしなら、変換待ちバッファ全てを変換候補に置き換えて、テキストバッファへ送る
     b. 送り仮名ありなら、変換バッファのうちのひらがなは変換候補に置き換えて、末尾1文字のアルファバットは変換バッファに残す。
        末尾1文字だけを見てアルファベット判定はできないので、先頭から文字数カウントとバイト数カウントをする。
 */


/**
 * @brief デバッグ用。SDカードのディレクトリツリーを出力する
 * @param dir [IN] `SD.open("/")` をそのまま与える
 * @param numTabs インデント時のTAB文字の個数
 */
void printDirectory(File dir, int numTabs);


/** 現在のテキストバッファの内容をシリアルで出力し、バッファを空にする
 */
void send_text_via_uart(void);

/** テキストを描画する
  @param line [IN] テキストを表示するテキスト行 (0 or 1)
  @param col [IN] 表示を開始する列番号 (0-121)
  @param sjis [IN] 表示するテキスト (NUL終端)
  @return 描画した列数
  */
uint8_t print_text(uint8_t line, uint8_t col, const char* sjis);


/** ローマ字からひらがなへの変換を試みる
  @param romaji [IN]
  @param romaji_len [IN] 
  @param dst [OUT]
  @param dst_len [IN]
  @param consumed_bytes [OUT]
  @param written_bytes [OUT]
  @return 成功すれば変換後のひらがなの文字数（>0）、もしくは0（ひらがなに変換できず）
  */
uint8_t try_convert_romaji_to_hiragana(const char* romaji, uint16_t romaji_len, char* dst, uint16_t dst_len, uint16_t* consumed_bytes, uint16_t* written_bytes);


#include <WString.h>

/** 
 * @brief Serialへ出力するprintfの簡易実装のcstr版。固定バッファで実装されている。
 * @param format [IN]
 * @param ... [IN]
 */
void serial_printf(const char* format, ...) {
    constexpr int SERIAL_PRINTF_BUFFER_LEN = 255;
    char serial_printf_buffer[SERIAL_PRINTF_BUFFER_LEN];
    va_list ap;
    va_start(ap, format);
    vsnprintf(serial_printf_buffer, SERIAL_PRINTF_BUFFER_LEN, (const char*)format, ap);
    Serial.print(serial_printf_buffer);
    va_end(ap);
}


Keyboard keyboard;


// void panic(const char* message) {
//     D(message);

//     screen.print_at(0, 0, message);

//     while (true) {
//         delay(100);
//     }
// }


bool input_keydown_prehook_callback(uint8_t ch) {

    bool func1_pressed = keyboard.is_key_down_rawkeycode(Keyboard::RAWKEYCODE_FUNCTION_1);
    bool func2_pressed = keyboard.is_key_down_rawkeycode(Keyboard::RAWKEYCODE_FUNCTION_2);
    bool func3_pressed = keyboard.is_key_down_rawkeycode(Keyboard::RAWKEYCODE_FUNCTION_3);

    // 制御コードの送出は漢字変換よりも優先する

    if (func2_pressed && ch == 'c') {
        // Send Ctrl+C
        DEBUG("Send Ctrl+C");
        Serial2.write((uint8_t)'\c');
        return true;
    }

    if (func2_pressed && ch == 'z') {
        // Send Ctrl+Z
        DEBUG("Send Ctrl+Z");
        Serial2.write((uint8_t)'\z');
        return true;
    }

    if (ch == Keyboard::KEYCODE_FUNCTION_2) {
        inputLine.set_autodecide_mode(!inputLine.get_autodecide_mode());
        DEBUG("henkanAutoDecide=%s", inputLine.get_autodecide_mode() ? "true" : "false");
        return true;
    }

    if (inputLine.is_henkan_waiting) {
        // 漢字変換待機中なので、これ以上はハンドルしない
        // DEBUG("Cancel keyhook due to is_henkan_waiting == true");
        return false;
    }

    // シリアル出力

    bool req_send_text_via_uart = false;
    {
        if (func1_pressed && func3_pressed) {
            req_send_text_via_uart = true;
        }

        bool enter_pressed = keyboard.is_key_down_rawkeycode(Keyboard::RAWKEYCODE_ENTER);
        // if (enter_pressed) {
        //     DEBUG("Enter key is pressed down now.");
        // }
        if (enter_pressed && (strlen(inputLine.henkanbuffer) == 0) && (strlen(inputLine.romajibuffer)) == 0) {
            req_send_text_via_uart = true;
        }
    }

    if (req_send_text_via_uart) {
        send_text_via_uart();
        keyboard.wait_allkey_released();
        return true;
    }



    return false;
}


bool input_keydown_uncaught_callback(uint8_t ch) {
    if (ch == Keyboard::KEYCODE_BACKSPACE) {
        sjis_remove_tail_char(textbuffer);
        draw_texts(true);
    } else if (ch == Keyboard::KEYCODE_ENTER) {
        //
    }
    return true;
}


void input_callback(const char* str, size_t len) {
    cstr_append(textbuffer, TEXTBUFFER_LENGTH, str, len);
    draw_texts(true);
}


void setup(void) {

    unsigned long start_millis = millis();

    Serial.begin(115200);
    Serial.println("Initializing...");

    memset(textbuffer, '\0', TEXTBUFFER_LENGTH);


    DEBUG("Init screen... ");
    screen.init();
    screen.clear();
    Serial.println("Screen ready.");

    DEBUG("Init Keyboard...");
    Wire.begin();
    keyboard.init();
    Serial.println("Keyboard ready.");

    DEBUG("Init SD... ");
    // if (!SD.begin(SPI_SD_CLOCK, PIN_SD_CS)) {
    if (!SD.begin(PIN_SD_CS)) {
        Serial.println("Failed to SD.begin()");
        PANIC("ERR:SDCARD");
    }
    Serial.println("SDCard ready.");

    DEBUG("Init font... ");
    if (!font14file.open(FILEPATH_FONT14, ArduinoSDFileAccessor::FileMode::READ)) {
        PANIC("Failed to open font14");
    }
    if (!font.init(&font14file, 14, 7, 14)) {
        Serial.println("Failed to font.load_font()");
        PANIC("Failed to font.load_font()");
    }
    font.enable_cache(fontcachebuffer, FONTCACHEBUFFER_LENGTH);
    screen.set_font(font);
    Serial.println("Font ready.");

    DEBUG("Init skk... ");
    skk.init();
    if (!sysDictFile.open(FILEPATH_SYSDICT, ArduinoSDFileAccessor::FileMode::READ)) {
        DEBUG("Failed to load skk sys dict file.");
        assert(false);
    }
    if (!sysDict.init(&sysDictFile)) {
        DEBUG("Failed to set SkkDictFile for sysDict.");
        assert(false);
    }
    if (!skk.set_sysdict(&sysDict)) {
        DEBUG("Failed to set sysDict to SkkEngine.");
        assert(false);
    }
    Serial.println("SKK ready.");


    DEBUG("Init InputEngine module... ");
    if (!inputLine.init(screen, 16, font, keyboard, skk, InputEngine::InputMode::Henkan_Hiragana,
                        henkanbuffer, HENKANBUFFER_LENGTH, romajibuffer, ROMAJIBUFFER_LENGTH)) {
        DEBUG("Failed to initialize InputEngine.");
        PANIC("Failed to initialize InputEngine.");
    }
    inputLine.set_sands(true);
    inputLine.set_keydown_prehook_callback(input_keydown_prehook_callback);
    inputLine.set_keydown_uncaught_callback(input_keydown_uncaught_callback);
    inputLine.set_input_callback(input_callback);

    Serial.println("InputEngine component ready.");

    screen.clear();
    screen.set_cursor(0, 0);
    screen.println_at(0, 0, "\x83\x8F\x81\x5B\x83\x76\x83\x8D " __TIME__);
    screen.println_at(0, 16, "KOISHIKAWA " __DATE__);

    screen.invert_rect(0, 0, 14 * 4 - 1, 14);
    screen.invert_rect(0, 16, 7 * 10 - 1, 31);
    screen.draw_line(121, 0, 121, 31);
    screen.draw_line(121 - 7 * 8 - 3, 14, 121, 14);

    DEBUG("Init Serial2 with 57600bps... ");
    Serial2.begin(57600);
    Serial.println("Serial2 ready. (57600bps 8N1)");

    DEBUG("Init text encoding utils... ");
    if (!convert_sjis_gb18030_table_file.open(FILEPATH_SJGB18TABLE, ArduinoSDFileAccessor::FileMode::READ)) {
        PANIC("FAIL: load convertion table for SJIS and GB18030.");
    }

    if (!convert_sjis_gb18030_table.init(&convert_sjis_gb18030_table_file)) {
        PANIC("FAIL: init table object");
    }
    
    Serial.println("Text encoding conversion (ShiftJIS to GB18030) ready.");

    Serial.printf("Initalized in %lu[msec]\n", millis() - start_millis);


    //DEBUG:
    {
        Serial.println("DEBUG: SDCard filetree:");
        File rootdir = SD.open("/");
        Serial.println("------------");
        printDirectory(rootdir, 0);
        Serial.println("------------");
    }

    Serial.println("Press any key to continue...");
    keyboard.wait_any_key();
    keyboard.wait_allkey_released();

    screen.clear();
    draw_texts(true);

    DEBUG("Leave setup()");
}


void draw_texts(bool update_textbuffer) {
    // 画面の1行に表示する最大の文字の数（バイト数）
    constexpr int DISPLAY_BYTES_IN_A_LINE = 7 * 2;
    static uint16_t displaystartindex = 0;

    uint8_t top = 0;

    // textbufferの文字数が多い場合、末尾が収まるようにする

    if (strlen(textbuffer) < DISPLAY_BYTES_IN_A_LINE) {
        // DEBUG("All text could be displayed in a line.");
        displaystartindex = 0;

    } else if (strlen(&textbuffer[displaystartindex]) < DISPLAY_BYTES_IN_A_LINE) {
        // DEBUG("decrement displaystartindex");
        uint16_t current_chars_in_a_line = count_chars_sjis(&textbuffer[displaystartindex]);
        uint16_t current_total_length_chars = count_chars_sjis(textbuffer);
        uint16_t new_start_chars = current_total_length_chars - current_chars_in_a_line - 1;
        char* ptr1 = (char*)get_n_char_ptr_sjis(textbuffer, new_start_chars);
        if (count_chars_sjis(ptr1) > DISPLAY_BYTES_IN_A_LINE) {
            new_start_chars -= 1;
            ptr1 = (char*)get_n_char_ptr_sjis(textbuffer, new_start_chars);
        }
        displaystartindex = ptr1 - textbuffer;
        
    } else {
        // DEBUG("increment displaystartindex");
        while (strlen(&textbuffer[displaystartindex]) > DISPLAY_BYTES_IN_A_LINE) {
            uint8_t cur_char_bytes = count_bytes_of_a_char_sjis(&textbuffer[displaystartindex]);
            displaystartindex += cur_char_bytes;
        }
    }
    const char* textbuffer_display_head = &textbuffer[displaystartindex];
    uint8_t display_text_length = strlen(textbuffer_display_head);
    uint8_t x1 = display_text_length * font.FONT_WIDTH_SINGLEBYTE,
            y1 = top,
            x2 = screen.SCREEN_WIDTH,
            y2 = y1 + font.FONT_HEIGHT;
    if (x1 > 2) {
        // カーソルを確実に消すために、少しずらす
        x1 -= 2;
    }
    screen.clear_rect_pagealined(x1, y1, x2, y2);

    screen.print_at(0, 0, textbuffer_display_head);

    { // textbufferのカーソルを表示する（常時点灯）
        uint8_t bytelen = strlen(textbuffer_display_head);
        uint8_t startcol = bytelen * 7;
        uint8_t cursorwidth = 1;
        uint8_t x1 = startcol,
                y1 = 0,
                x2 = x1 + cursorwidth,
                y2 = y1 + font.FONT_HEIGHT;
        screen.draw_hline(x1, y1, y2);
    }
}


/** 現在のテキストバッファの内容をシリアルで出力し、バッファを空にする
 */
void send_text_via_uart(void) {
    DEBUG("called.");
    if (strlen(textbuffer) < 1) {
        Serial2.write((uint8_t)'\n');
        return;
    }

    STOPWATCH_BLOCK_START(textconversion);

    // SJISからGB18030へ変換したうえで出力する実装
    size_t len = strlen(textbuffer);
    const char* end = textbuffer + len;
    const char* ptr = textbuffer;
    bool waiting_sjis_second_byte = false;
    const char* sjischar_start = textbuffer;

    while (ptr < end) {
        // DEBUG("ch=0x%02x(%d)", (uint8_t)*ptr, (uint8_t)*ptr);
        if (!waiting_sjis_second_byte && sjis_is_first_byte(*ptr)) {
            // DEBUG("SJIS 1st byte.");
            // SJISの第1バイトなので、第2バイトを待つ
            sjischar_start = ptr;
            waiting_sjis_second_byte = true;
            ++ptr;
            continue;

        } else if (!waiting_sjis_second_byte) {
            if (0xa0 <= *ptr && *ptr <= 0xdf) {
                // 半角カナはGB18030と互換性が（たぶん）ないので、とりあえず無視
                // FIXME: 適切な対応法を考える
                DEBUG("Warn: encount Hankaku-Kana char: 0x%02x(%d)", (uint8_t)*ptr, (uint8_t)*ptr);
                ++ptr;
                continue;

            } else {
                // DEBUG("ASCII byte.");
                // 1バイトのASCII文字（もしくは制御バイト）なので、そのまま出力する
                Serial2.write((uint8_t)*ptr);
                ++ptr;
                continue;
            }

        } else {
            // DEBUG("SJIS 2nd byte.");
            // SJISの第2バイトなので、第1バイトと合わせてエンコードを変換し、出力する
            char sjisbuf[2] = { *sjischar_start, *ptr };
            waiting_sjis_second_byte = false;
            ++ptr;
            uint8_t matched_length;
            uint32_t startaddr = convert_sjis_gb18030_table.search_startaddr_from_index_for(sjisbuf, 2, &matched_length);
            // DEBUG("startaddr=0x%lx(%0ld), matched_length=%d", startaddr, startaddr, matched_length);
            if (startaddr == INVALID_UINT32) {
                DEBUG("Error: cannot find startaddr for 0x%02x, 0x%02x", (uint8_t)sjisbuf[0], (uint8_t)sjisbuf[1]);
                continue;
            }
            SKK::CandidateReader reader;
            if (!convert_sjis_gb18030_table.search_henkanentry_for(startaddr, true, matched_length, sjisbuf, 2, &reader) ) {
                DEBUG("Error: cannot find Henkan entry for 0x%02x, 0x%02x", (uint8_t)sjisbuf[0], (uint8_t)sjisbuf[1]);
                continue;
            }
            uint8_t gb18030_length = reader.get_current_candidate_length();
            uint8_t gb18030_bytes[4] = { 0, 0, 0, 0 };
            for (uint8_t i = 0; i < gb18030_length; i++) {
                gb18030_bytes[i] = reader.read();
            }

            // DEBUG("Converted from (SJIS) 0x%02x, 0x%02x to (GB18030) 0x%02x, 0x%02x, 0x%02x, 0x%02x",
            //         (uint8_t)sjisbuf[0], (uint8_t)sjisbuf[1],
            //         (uint8_t)gb18030_bytes[0], (uint8_t)gb18030_bytes[1], (uint8_t)gb18030_bytes[2], (uint8_t)gb18030_bytes[3]);

            Serial2.write((uint8_t*)gb18030_bytes, gb18030_length);
        }

    }

    STOPWATCH_BLOCK_END(textconversion);

    Serial2.write((uint8_t)'\n');

    textbuffer[0] = '\0';
    draw_texts(true);

    DEBUG("Finished sending textbuffer content.");
}

bool is_first_loop = true;


void loop(void) {

    if (is_first_loop) {
        Serial.println("Ready.");
        is_first_loop = false;
    }

    inputLine.run();
}


void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
