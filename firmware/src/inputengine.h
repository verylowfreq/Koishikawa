#pragma once

/* バッファされた文字入力を受け持つモジュール
   SKKが利用できる。モードの一つとしてアルファベットの直接入力も可能。
   画面のうち1行を占有する。
 */


#include <stdint.h>
#include <stdbool.h>

#include "screen.h"
#include "font.h"
#include <skkengine.h>
#include "keyboard.h"
#include "screenex.h"


class InputEngine {
public:

    /* キー押下イベントのコールバック
       trueを返すと、InputEngine側でのハンドリングを抑制できる */
    typedef bool (*keydown_callback_t)(uint8_t keycode);
    /* 文字列確定時のコールバック */
    typedef void (*input_callback_t)(const char* input, size_t len);

    enum class InputMode {
        Direct,
        Henkan_Hiragana,
        Henkan_Katakana
    };

// private:
    // 表示デバイス
    ScreenEx* screen = nullptr;

    // スクリーン上の、InpuLineが使用する領域の上側のY座標
    uint8_t top_on_screen;

    // 表示に利用するフォント
    FontManager* font = nullptr;

    Keyboard* keyboard = nullptr;

    SKK::SkkEngine* skk = nullptr;

    // メイン処理を継続するか否か
    bool running = false;

    // 漢字変換時の変換候補の自動確定モードが有効か否か
    bool enabled_autodecide = false;

    // "Space and Shift" が有効か否か
    // (スペースキーの単押しでスペース、同時押しでShiftの動作)
    bool enabled_sands = false;

    // InputEngineが処理する前に、キー入力をフックする
    keydown_callback_t callback_keydown_prehook = nullptr;
    // InputEngineが処理しなかったキー入力や、確定済みテキストへの操作のコールバック
    keydown_callback_t callback_keydown_uncaught = nullptr;
    // 文字列が確定されるたびに呼び出されるコールバック
    input_callback_t callback_input = nullptr;

    InputMode currentInputMode;

    size_t henkanbuffer_length;
    char* henkanbuffer = nullptr;
    size_t romajibuffer_length;
    char* romajibuffer = nullptr;

    bool is_henkan_waiting = false;

    bool call_keydown_prehook_callback(uint8_t ch);
    void call_keydown_uncaught_callback(uint8_t ch);
    void call_input_callback(const char* s, size_t len);


public:

    bool init(ScreenEx& screen, uint8_t top, FontManager& font,
              Keyboard& keyboard, SKK::SkkEngine& skk, InputMode defaultInputMode,
              char* henkanbuffer, size_t henkanbuffer_length,
              char* romajibuffer, size_t romajibuffer_length
              );

    void set_keydown_prehook_callback(keydown_callback_t cb);
    void set_keydown_uncaught_callback(keydown_callback_t cb);
    void set_input_callback(input_callback_t cb);

    // 変換自動確定モードの有効無効を設定する
    void set_autodecide_mode(bool enabled);
    bool get_autodecide_mode(void);

    // SandSの有効無効を設定する
    void set_sands(bool enabled);
    bool get_sands(void);

    // InputEngineの処理に制御を移す
    void run(void);

    // メインループを終了し制御を戻す
    void stop(void);

    // 確定前の入力文字をすべて消去する
    void clear(void);

    // // 確定前の入力をそのまま確定させる
    // void flush(void);

    /** 確定前の入力をそのまま確定させる。
     * @param include_alphabet ひらがなになっていないアルファベットを含めるか
     */
    void flush(bool include_alphabet);

    // 画面に描画をすべて行なう
    // void draw_all(void);


    static constexpr const char* AUTODECIDE_MULTIPLECANDIDATE_OPEN_BRACKET = "{";
    static constexpr const char* AUTODECIDE_MULTIPLECANDIDATE_CLOSE_BRACKET = "}";
};
