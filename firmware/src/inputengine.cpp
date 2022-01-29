#include "inputengine.h"

#include <assert.h>
#include <alloca.h>

#include <candidatereader.h>

#include <debug.h>
#include <commondef.h>
#include "cstrlib.h"
#include "sjis.h"
#include "romajidef.h"


/** テキストを描画する
  @param line [IN] テキストを表示するテキスト行 (0 or 1)
  @param col [IN] 表示を開始する列番号 (0-121)
  @param sjis [IN] 表示するテキスト (NUL終端)
  @return 描画した列数
  */
uint8_t print_text(InputEngine* input, uint8_t line, uint8_t col, const char* sjis) {
// #if true
#if false
    uint8_t printed_cols = col;
    const char* end = sjis + strlen(sjis);

    // Clear lines

    STOPWATCH_BLOCK_START(inputengine_print_text_currentimpl);

    {
        uint8_t x1 = col;
        uint8_t y1 = line * 2 * 8;
        uint8_t x2 = input->screen->SCREEN_WIDTH;
        uint8_t y2 = y1 + input->font->FONT_HEIGHT;
        input->screen->clear_rect(x1, y1, x2, y2);
    }

    if (strlen(sjis) < 1) {
        return 0;
    }

    while (true) {
        uint8_t bytes_for_char = count_bytes_of_a_char_sjis(sjis);

        uint8_t ku, ten;
        convert_mb_to_kuten_sjis(sjis, &ku, &ten);

        uint8_t w, h;
        byte glyph[28];

        //DEBUG: 0b10101010 pattern
        memset(glyph, 0xAA, 28);

        bool glyph_found = input->font->get_glyph_from_kuten(ku, ten, glyph, &w, &h);

        if (!glyph_found) {
            if (bytes_for_char == 1) {
                input->font->get_glyph_from_kuten(0, 1, glyph, &w, &h);
            } else {
                input->font->get_glyph_from_kuten(0xFF, 1, glyph, &w, &h);
            }
        }
        input->screen->draw_glyph_obsolute(line, printed_cols, glyph, w, h);

        if (bytes_for_char == 1) {
            // 1バイト文字
            printed_cols += input->font->FONT_WIDTH_SINGLEBYTE;
            sjis += 1;
        } else {
            // 2バイト文字
            printed_cols += input->font->FONT_WIDTH_DOUBLEBYTE;
            sjis += 2;
        }

        if (sjis >= end || printed_cols >= 122) {
            return printed_cols - col;
        } else {
            continue;
        }
    }
    STOPWATCH_BLOCK_END(inputengine_print_text_currentimpl);

    return printed_cols - col;
#else

    size_t sjislen = strlen(sjis);
    const char* end = sjis + sjislen;

    if (sjislen < 1) {
        return 0;
    }

    // STOPWATCH_BLOCK_START(inputengine_print_text_newimpl);

    // Clear rect
    uint8_t x1 = col,
            y1 = line * 2 * 8,
            x2 = input->screen->SCREEN_WIDTH,
            y2 = y1 + input->font->FONT_HEIGHT;
    input->screen->clear_rect_pagealined(x1, y1, x2, y2);

    input->screen->print_at(x1, y1, sjis, sjislen);

    // STOPWATCH_BLOCK_END(inputengine_print_text_newimpl);

    return sjislen * input->font->FONT_WIDTH_SINGLEBYTE;
#endif
}


static
void draw_texts(InputEngine* input, bool update_romajibuffer, bool update_henkanbuffer) {

    // serial_printf("text  =\"%s\"\nhenkan=\"%s\"\nromaji=\"%s\"\n", input->henkanbuffer, input->romajibuffer);
    // DEBUG("Called--------");
    // DEBUG("  - henkanbuffer=\"%s\"", input->henkanbuffer);
    // DEBUG("  - romajibuffer=\"%s\"", input->romajibuffer);
    // DEBUG("--------------");

    uint8_t x1 = 0;
    uint8_t y1 = input->top_on_screen;
    uint8_t x2 = input->screen->SCREEN_WIDTH;
    uint8_t y2 = y1 + input->font->FONT_HEIGHT;
    // input->screen->clear_rect(x1, y1, x2, y2);
    input->screen->clear_rect_pagealined(x1, y1, x2, y2);

    uint8_t printed_col = 0;
    printed_col = print_text(input, 1, 0, input->henkanbuffer);
    print_text(input, 1, printed_col, input->romajibuffer);
}



/** ローマ字からひらがなへの変換を試みる
  @param romaji [IN]
  @param romaji_len [IN] 
  @param dst [OUT]
  @param dst_len [IN]
  @param consumed_bytes [OUT]
  @param written_bytes [OUT]
  @return 成功すれば変換後のひらがなの文字数（>0）、もしくは0（ひらがなに変換できず）
  */
uint8_t try_convert_romaji_to_hiragana(const char* romaji, uint16_t romaji_len, char* dst, uint16_t dst_len, uint16_t* consumed_bytes, uint16_t* written_bytes) {
    assert(romaji);
    assert(dst);
    assert(dst_len > 0);

    // Serial.println();
    // Serial.print("try_convert_romaji_to_hiragana(): Called with \"");
    // Serial.print(romaji);
    // Serial.println("\"");

    for (size_t i = 0; i < ROMAJITABLE_ENTRIES; ++i) {
        uint16_t entry_romaji_len = strlen(romajitable[i].romaji);
        if (entry_romaji_len != romaji_len) {
            continue;
        } else {
            if (memcmp(romaji, romajitable[i].romaji, romaji_len) == 0) {
                // 一致
                if (romaji_len > dst_len) {
                    // 書き出し先バッファの長さが足りない
                    return 0;
                }
                uint16_t totalhiraganabytelen = strlen(romajitable[i].hiragana);
                uint16_t hiraganabytelen = romajitable[i].hiraganabytelength;
                memcpy(dst, romajitable[i].hiragana, hiraganabytelen);
                if (totalhiraganabytelen == hiraganabytelen) {
                    // 余りのアルファベットがない場合
                    *consumed_bytes = romaji_len;
                    *written_bytes = hiraganabytelen;
                    // ShiftJIS決め打ちなので、ひらがなは2バイト1文字と推定して良い
                    return hiraganabytelen / 2;
                } else {
                    // 余りのアルファベットがある場合。末尾のアルファベットを書き戻す（消費しない）
                    *consumed_bytes = romaji_len - 1;
                    *written_bytes = hiraganabytelen;
                    return hiraganabytelen / 2;
                }
            }
        }
    }

    return 0;
}



/** 変換候補を一つ選んで確定する
 * @param candidates [IN]
 * @param dst [OUT] 選択された変換候補を書き出すバッファ。常にNUL終端された状態で戻る
 * @param dstlen [IN]
 * @return 正常に書き込めたらtrue、バッファ長が不足していたらfalse
 */
static
bool choose_one_candidates(InputEngine* input, SKK::CandidateReader* candidates, char* dst, size_t dstlen) {

    DEBUG("called.");

    uint8_t candidatecount = candidates->get_candidates_count();
    uint8_t next_start_index = 0;

    // 画面上に表示できる最大バイト数（この範囲内で、かつ変換候補が分断されない範囲で詰め込む）
    uint8_t max_display_bytes = 8 * 2;

    while (true) {
        // 変換候補選択用の表示バッファ（ShiftJIS）
        constexpr size_t DISPLAYTEXTBUFFERLEN = 16;
        uint8_t displaytextbuffer[DISPLAYTEXTBUFFERLEN + 1];
        memset(displaytextbuffer, '!', DISPLAYTEXTBUFFERLEN);
        displaytextbuffer[DISPLAYTEXTBUFFERLEN] = '\0';

        // どの選択キーがどの変換候補に結びついているかを保持する
        uint8_t associated_candidate_index[10];
        memset(associated_candidate_index, INVALID_UINT8, 10);
        // DEBUG("associated_candidate_index initialized.");
        // dump_bytes_hex(associated_candidate_index, 10);
        // DPUT("\n");
        
        // 変換候補を画面に詰め込む
        uint8_t cur_display_bytes = 0;
        uint8_t candidx = 0;
        // NOTE: 選択キーの都合で、一度に10個が上限（この上限に到達することはまずないけど）
        // for (candidx = 0; candidx < 10 && !candidates->is_reached_end(); ++candidx) {
        while (true) {
            // DEBUG("next_start_index=%d, candidx=%d", next_start_index, candidx);

            if (candidx >= 10) {
                DEBUG("Reached to limit.");
                break;
            }
            if (candidates->is_reached_end()) {
                DEBUG("Reached to end.");
                break;
            }

            uint8_t cur_len = candidates->get_current_candidate_length();
            DEBUG("cur_len=%d", cur_len);

            if (candidx == 0 && cur_len > max_display_bytes) {
                // 最初の変換候補が画面をはみ出す場合は、その一つだけを出す
                // next_start_index += 1;
                next_start_index += 1;
                // DEBUG("Too long candidate at first. next_start_index=%d", next_start_index);
                break;
            }
            if (cur_display_bytes + cur_len > max_display_bytes) {
                // 画面からはみ出すので抜ける
                // next_start_index += candidx;
                DEBUG("Break due to display overflow. next_start_index=%d, cur_display_bytes=%d, cur_len=%d,", next_start_index, (uint8_t)cur_display_bytes, (uint8_t)cur_len);
                break;
            }
            // バッファへ納める
            // for (uint8_t j = 0; j < cur_len; ++j) {
            while (true) {
                int ch = candidates->read();
                if (ch < 0) {
                    // DEBUG("CandidateReader.read() returned %d. next_start_index=%d", ch, next_start_index);
                    break;
                }
                // DEBUG("  ch=%d(0x%x)", (uint8_t)ch, (uint8_t)ch);
                // バッファへ文字コードを追加する
                // displaytextbuffer[cur_display_bytes+j] = (uint8_t)ch;
                displaytextbuffer[cur_display_bytes] = (uint8_t)ch;
                if (cur_display_bytes % 2 == 0) {
                    // 選択キー用のインデックス値を追加する
                    // DEBUG("Associated index: %d at %d", next_start_index + candidx, cur_display_bytes);
                    associated_candidate_index[cur_display_bytes / 2] = next_start_index + candidx;
                }
                cur_display_bytes += 1;
            }
            candidates->move_next();

            candidx += 1;
        }
        next_start_index += candidx;
        displaytextbuffer[cur_display_bytes] = '\0';
        // DEBUG("Dump displaytextbuffer:");
        // dump_bytes_hex(displaytextbuffer, DISPLAYTEXTBUFFERLEN);
        // DPUT("\n");
        DEBUG("Displayed candidates (next index=%ud).", next_start_index);
        print_text(input, 1, 0, (const char*)displaytextbuffer);
        // ここまでで、画面に変換候補が表示できた


        while (true) {
            constexpr char selectors[] = {'q','w','e','r','t','y','u','i','o','p'};
            constexpr uint8_t selectors_count = sizeof(selectors) / sizeof(selectors[0]);

            input->keyboard->flush();
            input->keyboard->update();
            Keyboard::keycode_t key = input->keyboard->get_key();

            if (key == Keyboard::KEYCODE_ESC || key == Keyboard::KEYCODE_BACKSPACE) {
                // 変換を中断して戻る
                DEBUG("Canceled by ESC key");
                return false;
            }

            if (key == 'n') {
                input->keyboard->wait_allkey_released();

                if (next_start_index >= candidatecount) {
                    // 先頭へ戻る
                    candidates->move_head();
                    next_start_index = 0;
                    DEBUG("Move back to head.");
                    break;
                } else {
                    // 次のページへ
                    DEBUG("Go next page.");
                    break;
                }
            }

            int8_t selectionbykey = -1;
            for (int i = 0; i < selectors_count; ++i) {
                if (key == selectors[i]) {
                    selectionbykey = i;
                    break;
                }
            }

            if (selectionbykey >= 0) {
                // 候補選択と思われるキー入力があった

                uint8_t selected_candidate_index = associated_candidate_index[selectionbykey];

                // if (0 <= selectionbykey && selectionbykey < candidatecount) {
                if (selected_candidate_index != INVALID_UINT8) {
                    // 有効な選択キーが押下された
                    DEBUG("selected_candidate_index=%d(0x%x)", selected_candidate_index, selected_candidate_index);
                    // DEBUG("associated_candidate_index = ");
                    // dump_bytes_hex(associated_candidate_index, 10);
                    // DPUT("\n");

                    candidates->move_head();
                    for (uint8_t i = 0; i < selected_candidate_index; ++i) {
                        DEBUG("Move next (current candidate len=%d)", candidates->get_current_candidate_length());
                        candidates->move_next();
                    }
                    // ここで、candidateは選択された変換候補の先頭を読み取る準備ができている
                    DEBUG("OK. key=%c, selected index=%d, candidate length=%d", key, selected_candidate_index, candidates->get_current_candidate_length());
                    return true;
                } else {
                    // 範囲外のキーが指定された
                    DEBUG_HEADER();
                    DEBUG_PRINTF("selected_candidate_index=%d(0x%x)\n", selected_candidate_index, selected_candidate_index);
                    DEBUG_PRINTF("  associated_candidate_inde = ");
                    dump_bytes_hex(associated_candidate_index, 10);
                    DEBUG_PRINTF("\n");
                }
            }

            // if (key == ' ') {
                // TODO: スペースキー押下による、選択候補の順次選択を実装したい
            // }

            // 有効なキー入力ではなかったので無視
            delay(1);
        }
    }

    DEBUG("Should not be reached.");
    return false;
}

/**
 * @return 漢字へ変換したらtrue、エラーや変換しなかったらfalse
 */
static
bool henkan(InputEngine* input) {
    // DEBUG("called. hiraganalen=%d", strlen(input->henkanbuffer));
    // DEBUG("  ");
    // dump_bytes_hex((const byte*)input->henkanbuffer, strlen(input->henkanbuffer));
    // DPUT("\n");
    // unsigned long henkan_timer = millis();
    SKK::CandidateReader reader;

    bool henkan_found = input->skk->henkan(input->henkanbuffer, &reader);
    // unsigned long elapsed_time_henkan = millis() - henkan_timer;
    // DEBUG("%lu[msec] elapsed in henkan() executing.", elapsed_time_henkan);


    bool multiple_candidate_found = false;

    // 選択された変換候補を格納するバッファ
    char buf[64 * 2];
    memset(buf, 0, 64 * 2);

    if (henkan_found) {
        uint8_t found_candidates = reader.get_candidates_count();
        multiple_candidate_found = found_candidates > 1;
        if (found_candidates == 1 || input->enabled_autodecide) {
            // 候補は1つだけなので、もしくは変換自動確定モードなので、先頭候補をそのまま使う
            // DEBUG("Only one candidate found. Use it.");
            // Fallthrough

        } else {
            // 複数の候補があるので、ユーザーに選択させる
            if (!choose_one_candidates(input, &reader, buf, 64 * 2)) {
                // 候補が選択されなかった
                DEBUG("Henkan canceled by user.");
                return false;

            } else {
                // 候補が選択された
                DEBUG("Henkan OK.");
                // Fallthrough
            }
        }
    } else {
        DEBUG("No candidate found.");
        // 入力欄をフラッシュさせる
        for (int i = 0; i < 2; i++) {
            uint8_t x1 = 0,
                    y1 = input->top_on_screen,
                    x2 = input->screen->SCREEN_WIDTH,
                    y2 = y1 + input->font->FONT_HEIGHT;
            input->screen->invert_rect(x1, y1, x2, y2);
            delay(50);
        }
        return false;
    }

    // 変換候補が確定しているので書き込む
    // int ch;
    // while ((ch = reader.read()) >= 0) {
    //     cstr_append_byte(textbuffer, TEXTBUFFER_LENGTH, (uint8_t)ch);
    // }
    if (input->currentInputMode == InputEngine::InputMode::Henkan_Hiragana) {
        size_t candlen = reader.get_current_candidate_length();
        char* candbuf = (char*)alloca(candlen + 1);
        candbuf[candlen] = '\0';
        int ch;
        for (int i = 0; i < candlen; i++) {
            candbuf[i] = reader.read();
        }
        // while (((ch = reader.read()) > 0) && !reader.is_reached_end()) {
            // cstr_append_byte(textbuffer, TEXTBUFFER_LENGTH, (uint8_t)ch);
        // }

        if (input->enabled_autodecide && multiple_candidate_found) {
            input->call_input_callback(InputEngine::AUTODECIDE_MULTIPLECANDIDATE_OPEN_BRACKET, 1);
        }

        input->call_input_callback(candbuf, strlen(candbuf));

        if (input->enabled_autodecide && multiple_candidate_found) {
            input->call_input_callback(InputEngine::AUTODECIDE_MULTIPLECANDIDATE_CLOSE_BRACKET, 1);
        }

    } else {
        // ひらがなをカタカナへ変換してから確定する
        DEBUG("Convert hiragana to katanaka...");
        // constexpr size_t KANABUF_LEN = 32 * 2;
        // char kanabuf[KANABUF_LEN];
        // int ch;
        // for (size_t i = 0; (i < KANABUF_LEN - 1) && ((ch = reader.read()) > 0) && !reader.is_reached_end(); ++i) {
        //     kanabuf[i] = (char)ch;
        // }
        // char *ptr = (char*)&kanabuf;
        // do {
        //     convert_hiragana_to_katakana_sjis(ptr, ptr);
        //     ptr = (char*)get_next_char_ptr_sjis((byte*)ptr);

        // } while (ptr);
        // cstr_append(textbuffer, TEXTBUFFER_LENGTH, kanabuf);

        if (input->enabled_autodecide && multiple_candidate_found) {
            input->call_input_callback(InputEngine::AUTODECIDE_MULTIPLECANDIDATE_OPEN_BRACKET, 1);
        }

        char kanabuf[3] = { 0, 0, 0 };
        int ch;
        while (!reader.is_reached_end()) {
            ch = reader.read();
            if (ch < 0) {
                break;
            }
            kanabuf[0] = (char)ch;
            if (count_bytes_of_a_char_sjis(kanabuf) == 1) {
                // 1バイト文字だったので、次のバイトから新たに処理する
                continue;
            }
            ch = reader.read();
            if (ch < 0) {
                // ここで読み込みが失敗したということは、直前に読み込んだ1バイト文字が終端なので、それを追加して終了する
                // cstr_append_byte(textbuffer, TEXTBUFFER_LENGTH, kanabuf[0]);
                input->call_input_callback(kanabuf, strlen(kanabuf));
                break;
            }
            kanabuf[1] = (char)ch;
            // ここに到達する時点でShiftJIS 2バイト文字であることが確定しているので、カタカナへ変えて追加する
            convert_hiragana_to_katakana_sjis(kanabuf, kanabuf);
            // cstr_append_byte(textbuffer, TEXTBUFFER_LENGTH, kanabuf[0]);
            // cstr_append_byte(textbuffer, TEXTBUFFER_LENGTH, kanabuf[1]);
            
            input->call_input_callback(kanabuf, strlen(kanabuf));
        }

        if (input->enabled_autodecide && multiple_candidate_found) {
            input->call_input_callback(InputEngine::AUTODECIDE_MULTIPLECANDIDATE_CLOSE_BRACKET, 1);
        }

        // DEBUG("Done");
    }    
    input->henkanbuffer[0] = '\0';

    DEBUG("Henkan done.");
    return true;
}






// bool InputEngine::init(Screen& screen, uint8_t top, FontManager& font, Keyboard& keyboard, SKK::SkkEngine* skk, InputMode defaultInputMode) {

bool InputEngine::init(ScreenEx& screen, uint8_t top, FontManager& font,
            Keyboard& keyboard, SKK::SkkEngine& skk, InputMode defaultInputMode,
            char* henkanbuffer, size_t henkanbuffer_length,
            char* romajibuffer, size_t romajibuffer_length) {

    assert(henkanbuffer_length > 16);
    assert(romajibuffer_length > 4);
    assert(henkanbuffer);
    assert(romajibuffer);

    this->screen = &screen;
    this->top_on_screen = top;
    this->font = &font;
    this->skk = &skk;
    this->keyboard = &keyboard;
    this->currentInputMode = defaultInputMode;
    this->henkanbuffer_length = henkanbuffer_length;
    this->henkanbuffer = henkanbuffer;
    memset(this->henkanbuffer, '\0', this->henkanbuffer_length);
    this->romajibuffer_length = romajibuffer_length;
    this->romajibuffer = romajibuffer;
    memset(this->romajibuffer, '\0', this->romajibuffer_length);
    // this->henkanbuffer = (char*)calloc(1, this->HENKANBUFFER_LENGTH);
    // assert(this->henkanbuffer);
    // this->romajibuffer = (char*)calloc(1, this->ROMAJIBUFFER_LENGTH);
    // assert(this->romajibuffer);

    return true;
}


void InputEngine::set_keydown_prehook_callback(keydown_callback_t cb) {
    this->callback_keydown_prehook = cb;
}

void InputEngine::set_keydown_uncaught_callback(keydown_callback_t cb) {
    this->callback_keydown_uncaught = cb;
}

void InputEngine::set_input_callback(input_callback_t cb) {
    this->callback_input = cb;
}


void InputEngine::set_autodecide_mode(bool enabled) {
    this->enabled_autodecide = enabled;
}

bool InputEngine::get_autodecide_mode(void) {
    return this->enabled_autodecide;
}

void InputEngine::run(void) {
    static unsigned long blink_interval_ms = 400;
    static unsigned long blink_timer_millis = 0;
    static bool blink_is_now_drawn = false;

    // スペースキーはSandS機能のために、別処理で判定する
    static bool spacekey_pressed_down = false;
    // SandS有効時、スペースキーを離した際にスペースキーの動作をするが、それをキャンセルするか？
    static bool suppress_spacekey_release_action = false;

    this->running = true;

    constexpr unsigned long loopinterval_msec = 5;
    unsigned long looptimer_millis = millis();

    while (this->running) {

        if (millis() - blink_timer_millis > blink_interval_ms) {
            constexpr int CURSOR_FULLWIDTH = 14;
            constexpr int CURSOR_HALFWIDTH = 7;
            constexpr int CURSOR_SLIM = 3;
            constexpr int CURSOR_LINE = 1;
            constexpr int CURSOR_YOFFSET_ON_KATAKANA = 8;
            uint8_t bytelength = strlen(henkanbuffer) + strlen(romajibuffer);
            uint8_t startcol = bytelength * 7;
            uint8_t x1, y1, x2, y2, blinkwidth;

            if (currentInputMode == InputMode::Direct || !this->is_henkan_waiting) {
                // 変換バッファを経由しない場合は（英字もしくは変換待ちでない）、
                // 細いカーソルにする
                blinkwidth = currentInputMode == InputMode::Direct ? CURSOR_LINE : CURSOR_SLIM;

            } else {
                // Shift押下時と変換待機時に全角幅のカーソルを出す

                bool is_shift_down = this->keyboard->is_key_down_rawkeycode(Keyboard::RAWKEYCODE_SHIFT_LEFT) || this->keyboard->is_key_down_rawkeycode(Keyboard::RAWKEYCODE_SHIFT_RIGHT);
                blinkwidth = (this->is_henkan_waiting || is_shift_down) ? CURSOR_FULLWIDTH : CURSOR_HALFWIDTH;
            }
            x1 = startcol;
            y1 = this->top_on_screen;
            // 前回のカーソル表示を確実に消すために、もっとも広い幅で消去する
            x2 = startcol + CURSOR_FULLWIDTH;
            y2 = y1 + this->font->FONT_HEIGHT;

            this->screen->clear_rect_pagealined(x1, y1, x2, y2);

            if (blink_is_now_drawn) {
                // カタカナ入力時は高さを半分にする
                y1 -= (currentInputMode == InputMode::Henkan_Katakana) ? CURSOR_YOFFSET_ON_KATAKANA : 0;
                // 本来のカーソル幅にする
                x2 = startcol + blinkwidth;
                this->screen->fill_rect_pagealined(x1, y1, x2, y2);
            }

            blink_is_now_drawn = !blink_is_now_drawn;
            blink_timer_millis = millis();
        }

        
        if (strlen(romajibuffer) == 0 && strlen(henkanbuffer) == 0) {
            // 入力待ちバッファが空なら、変換モードを抜ける
            this->is_henkan_waiting = false;
        }

        if (millis() - looptimer_millis < loopinterval_msec) {
            delayMicroseconds(250);
            continue;
        }

        looptimer_millis = millis();

        
        this->keyboard->update();

        
        // uint8_t ch = this->keyboard->get_key();

        uint8_t ch = Keyboard::KEYCODE_NONE;


        bool shift_by_sands = false;

        if (this->enabled_sands) {
            bool spacekey_is_down = keyboard->is_key_down_rawkeycode(Keyboard::RAWKEYCODE_SPACE);
            if (!spacekey_pressed_down) {
                if (spacekey_is_down) {
                    // スペースキーが押し込まれた
                    suppress_spacekey_release_action = false;
                    spacekey_pressed_down = true;
                    shift_by_sands = true;
                    // DEBUG("(SandS) Spacekey is now pressed down.");
                } else {
                    // スペースキーが離されたまま
                    // Nop
                }
            } else {
                if (spacekey_is_down) {
                    // スペースキーが継続して押されている
                    shift_by_sands = true;
                } else {
                    // スペースキーが離された
                    // DEBUG("(SandS) Spacekey is now released.");
                    spacekey_pressed_down = false;
                    if (!suppress_spacekey_release_action) {
                        ch = ' ';
                        // DEBUG("(SandS) Spacekey input.");
                    } else {
                        // Nop
                        // DEBUG("(SandS) Spacekey aciton canceled.");
                    }
                }
            }
        }

        if (ch == Keyboard::KEYCODE_NONE) {
            // SandSでスペースの入力がセットされなかった、もしくはSandS無効時は、
            // 普通にキー入力を取得する
            ch = keyboard->get_key();
        }

        if (ch == 0x00) {
            // 新たなキー押下なし
            continue;

        } else {

            // DEBUG("Key down : %d (0x%02x)", (uint8_t)ch, (uint8_t)ch);

            // まずはキーボードフックを処理する
            if (this->call_keydown_prehook_callback(ch)) {
                // コールバック側で処理したので、以下の処理はスキップする
                // DEBUG("Key down event is processed prehook.");
                continue;
            }

            if (ch != Keyboard::KEYCODE_SPACE) {
                suppress_spacekey_release_action = true;
            }

            if (ch == ' ' && this->enabled_sands && (spacekey_pressed_down || suppress_spacekey_release_action)) {
                // SandS有効時はいろんな条件でキー判定を飛ばす
                continue;
            }


            // Ctlr+L
            // if (ch == 0x0c) {
            //     Serial.println(F("Clear buffers."));
            //     this->screen->fill(0x00);
            //     // memset(textbuffer, '\0', TEXTBUFFER_LENGTH);
            //     // memset(henkanbuffer, '\0', HENKANBUFFER_LENGTH);
            //     // memset(romajibuffer, '\0', ROMAJIBUFFER_LENGTH);
            //     print_text(this, 0, 0, "(Kanji)");
            //     print_text(this, 1, 0, "(Input)");
            //     // continue;
            //     // return;
            //     continue;
            // }

            // if (ch == Keyboard::KEYCODE_ESC) {
            //     if (this->keyboard->is_key_down_rawkeycode(45)) {
            //         textbuffer[0] = '\0';
            //         flush_buffers();
            //         draw_texts(true, true, true);
            //         return;
            //     }
            // }

            // Backspace
            if (ch == Keyboard::KEYCODE_BACKSPACE) {
                if (strlen(romajibuffer) > 0) {
                    sjis_remove_tail_char(this->romajibuffer);
                    draw_texts(this, true, true);
                    continue;

                } else if (strlen(henkanbuffer) > 0) {
                    sjis_remove_tail_char(this->henkanbuffer);
                    draw_texts(this, true, true);
                    continue;

                } else {
                    this->call_keydown_uncaught_callback(ch);
                    continue;
                }
            }

            // Enter
            if (ch == Keyboard::KEYCODE_ENTER) {
                DEBUG("Enter key down catch. is_henkan_waiting=%s", is_henkan_waiting ? "true" : "false");
                if (this->is_henkan_waiting) {
                    // 変換待ちでEnterが押されたら、変換せずに確定する
                    this->is_henkan_waiting = false;
                    this->flush(true);
                    draw_texts(this, true, true);
                    DEBUG("Decide hiragana.");
                    continue;
                }
                /* else {
                    // 変換待ちではなくEnterが押されたら、改行する
                    // TODO: まず改行動作を実装する
                } */
            }


            if (ch == ' ' && is_henkan_waiting) {
                // スペース入力による変換の実行
                if (henkan(this)) {
                    this->is_henkan_waiting = false;
                }

                // スクリーン描画
                draw_texts(this, true, true);

                // return;
                continue;
            }

            // 変換モード切替
            if (ch == Keyboard::KEYCODE_MUHENKAN) {
                // 無変換へ
                DEBUG("Change mode to Direct input");
                currentInputMode = InputMode::Direct;
                is_henkan_waiting = false;
                // flush_buffers();
                this->flush(true);
                draw_texts(this, true, true);

                this->keyboard->wait_allkey_released();

                continue;
            }

            if (ch == Keyboard::KEYCODE_HENKAN_HIRA_KANA) {
                if (is_henkan_waiting) {
                    // 変換待機中に「変換」キーが押されたら、カタカナにして確定する。
                    // ひらがな・カタカナのモード切替は行わない。
                    convert_hiragana_to_katakana_sjis(henkanbuffer, henkanbuffer);
                    this->flush(true);
                    draw_texts(this, true, true);
                    continue;

                } else {
                    // 変換キー

                    this->flush(true);
                    if (currentInputMode == InputMode::Henkan_Hiragana) {
                        // カタ変換へ
                        DEBUG("Change mode to Henkan Katakana input");
                        currentInputMode = InputMode::Henkan_Katakana;

                    } else {  // == (currentInputMode == InputMode::Henkan_Katanaka || currentInputMode == InputMode::Direct)
                        // ひら変換へ
                        DEBUG("Change mode to Henkan Hiragana input");
                        currentInputMode = InputMode::Henkan_Hiragana;
                    }
                    this->keyboard->wait_allkey_released();

                    continue;
                }
            }

            if (shift_by_sands && ('a' <= ch && ch <= 'z')) {
                    // 小文字から 0x20 を引くと、大文字になる
                    ch -= 0x20;
            }

            if (('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z')) {
                // アルファベットの入力

                bool is_upper_char_input = ('A' <= ch && ch <= 'Z');

                if (currentInputMode == InputMode::Direct) {
                    // DEBUG("Direct input event with char %c", ch);
                    // 無変換モードなので、英字として自動で確定する
                    this->call_input_callback((char*)&ch, 1);
                    draw_texts(this, true, true);
                    // return;
                    continue;
                }

                // 変換処理の都合上、大文字は小文字に戻す
                if (is_upper_char_input) {
                    // 大文字に 0x20 を足すと、小文字になる
                    ch += 0x20;
                }

                // ローマ字バッファに押し込む
                cstr_append_char(romajibuffer, this->romajibuffer_length, ch);
                uint16_t romajibuffer_strlen = strlen(romajibuffer);
                
                if (is_henkan_waiting && is_upper_char_input) {
                    // 変換待ちで大文字入力なら、ひらがなへの変換をせずに漢字変換を実行する。
                    Serial.println("Henkan tirggered with Shift key.");
                    // 送り仮名（と思われる）ローマ字を変換バッファへ追加する
                    cstr_append(henkanbuffer, this->henkanbuffer_length, romajibuffer, strlen(romajibuffer));
                    henkan(this);
                    DEBUG("Henkan waiting chars gone.");
                    is_henkan_waiting = false;

                    is_upper_char_input = false;
                }

                // ひらがなへの変換を試みる
                uint16_t henkanbuffer_strlen = strlen(henkanbuffer);
                uint16_t consumed_bytes = 0;
                uint16_t written_len = 0;
                char* dst_ptr = &henkanbuffer[henkanbuffer_strlen];
                uint16_t hiragana_chlen = try_convert_romaji_to_hiragana(romajibuffer, strlen(romajibuffer),
                        dst_ptr, this->henkanbuffer_length, //HENKANBUFFER_LENGTH,
                        &consumed_bytes, &written_len);
                henkanbuffer[henkanbuffer_strlen+written_len] = '\0';

                // Serial.print(("try_convert_romaji_to_hiragana() returned "));
                // Serial.print(hiragana_chlen);
                // Serial.print((", consumed_bytes,written_len="));
                // Serial.print(consumed_bytes);
                // Serial.print(",");
                // Serial.print(written_len);
                // Serial.println();

                bool flush_include_romajibuffer_flush = true;

                if (hiragana_chlen > 0) {
                    if (romajibuffer_strlen == consumed_bytes) {
                        romajibuffer[0] = '\0';
                    } else {
                        // アルファベットを末尾に残す場合、ひらがなにできた分だけ前へ移動する
                        uint8_t unconsumed_bytes = romajibuffer_strlen - consumed_bytes;
                        for (int i = 0; i < unconsumed_bytes; ++i) {
                            romajibuffer[i] = romajibuffer[consumed_bytes + i];
                        }
                        romajibuffer[unconsumed_bytes] = '\0';
                        // romajibufferに残した分は確定させない
                        flush_include_romajibuffer_flush = false;
                    }
                } else {
                    // ひらがなに変換できなかった場合は、バッファの中身を確定せずに残す
                    flush_include_romajibuffer_flush = false;

                }

                // Serial.print(F("henkanbuffer="));
                // dump_bytes((const byte*)henkanbuffer, strlen(henkanbuffer));
                // Serial.println();
                // Serial.print(F("romajibuffer="));
                // dump_bytes((const byte*)romajibuffer, strlen(romajibuffer));
                // Serial.println();

                if (!is_henkan_waiting && is_upper_char_input) {
                    // 変換対象区間のはじまり
                    // Serial.println("Henkan section started with Shift key.");
                    is_henkan_waiting = true;

                } else if (!is_henkan_waiting && hiragana_chlen > 0) {
                    // ひらがなをそのまま確定させる
                    // Serial.println("Push hiragana to textbuffer.");
                    if (currentInputMode == InputMode::Henkan_Katakana) {
                        convert_hiragana_to_katakana_sjis(henkanbuffer, henkanbuffer);
                    }
                    this->flush(flush_include_romajibuffer_flush);

                } else if (is_upper_char_input) {
                    DEBUG("!!! Should no be reached main.ino L%d", __LINE__);
                    DEBUG("Start Henkan with Shift key.");
                    is_henkan_waiting = true;
                }

                draw_texts(this, true, true);

            } else if ((' ' <= ch && ch <= '~') ||    // ASCIIの記号（アルファベットは上でとらえているので、ここではざっくりと）
                    (0xa1 <= ch && ch <= 0xa5) ||  // 半角カナの記号
                    (0xb0 == ch) ||                // 半角カナの記号の伸ばし棒
                    (0xde <= ch && ch <= 0xdf)) {  // 半角カナの記号
                // アルファベット以外の記号と数字

                // 変換モードを抜ける
                is_henkan_waiting = false;
                // カタカナ処理
                if (currentInputMode == InputMode::Henkan_Katakana) {
                    char* ptr = henkanbuffer;
                    do {
                        convert_hiragana_to_katakana_sjis(ptr, ptr);
                        ptr = get_next_char_ptr_sjis(ptr);
                    } while (ptr);
                }

                // バッファを確定させる
                // flush_buffers();
                this->flush(true);

                if (currentInputMode == InputMode::Direct) {
                    // 直接入力時はキーコードそのまま
                    this->call_input_callback((char*)&ch, 1);

                } else {
                    // 変換モード時は、記号を全角に差し替える
                    // FIXME: いくつかだけ実装
                    char buf[2] = { 0, 0 };
                    switch (ch) {
                        case '-': // 伸ばし棒 「ー」
                            buf[0] = 0x81;
                            buf[1] = 0x5b;
                            break;
                        case ',':  // 句読点「、」
                            buf[0] = 0x81;
                            buf[1] = 0x41;
                            break;
                        case '.':  // 読点「、」
                            buf[0] = 0x81;
                            buf[1] = 0x42;
                            break;
                        case 0xa5:  // 半角の中黒を全角へ「・」
                            buf[0] = 0x81;
                            buf[1] = 0x45;
                            break;
                        default:
                            buf[0] = ch;
                            buf[1] = 0x00;
                    }
                    this->call_input_callback((char*)buf, strlen(buf));

                }

                draw_texts(this, true, true);
            }
        }
    }
}


bool InputEngine::call_keydown_prehook_callback(uint8_t ch) {
    // DEBUG("Called with ch=0x%02x(%d). ch=%p", ch, ch, this->callback_keydown_prehook);
    if (this->callback_keydown_prehook) {
        return this->callback_keydown_prehook(ch);
    }
    return false;
}


void InputEngine::call_keydown_uncaught_callback(uint8_t ch) {
    // DEBUG("Called with ch=0x%02x(%d). cb=%p", ch, ch, this->callback_keydown_uncaught);
    if (this->callback_keydown_uncaught) {
        this->callback_keydown_uncaught(ch);
    }
}


void InputEngine::call_input_callback(const char* s, size_t len) {
    // DEBUG("Call callback. len=%d. cb=%p", len, this->callback_input);
    if (len < 1) {
        return;
    }
    if (this->callback_input) {
        this->callback_input(s, len);
    }
}

void InputEngine::stop(void) {
    this->running = false;
}


void InputEngine::clear(void) {
    // DEBUG("Called.");
    this->henkanbuffer[0] = '\0';
    this->romajibuffer[0] = '\0';
}


void InputEngine::flush(bool include_alphabet) {
    // DEBUG("strlen(henkan)=%d, strlen(romaji)=%d", strlen(this->henkanbuffer), strlen(this->romajibuffer));
    this->call_input_callback(this->henkanbuffer, strlen(this->henkanbuffer));
    if (include_alphabet) {
        this->call_input_callback(this->romajibuffer, strlen(this->romajibuffer));
    }

    this->henkanbuffer[0] = '\0';
    if (include_alphabet) {
        this->romajibuffer[0] = '\0';
    }
}

void InputEngine::set_sands(bool enabled) {
    this->enabled_sands = enabled;
}

bool InputEngine::get_sands(void) {
    return this->enabled_sands;
}
