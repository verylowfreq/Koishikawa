#pragma once


#include <stdint.h>
#include <stdbool.h>

// For "byte" typedef
#include <Arduino.h>

/*
物理配置とキーコードの対応表:
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  11, [23],
   13,  14, 15, 16, 17, 18, 19, 20, 21, 22, [ 35 ],
   [25],  26, 27, 28, 29, 30, 31, 32, 33, 34?, [46],
   37,  38,  39,  40, [ 41 ], 42,  43,  44,  45

   [23] : Backspace
   [35] : Return/Enter
   [25], [46] : Shift
   [41] : Space
*/


class Keyboard {
public:
    // システムで使用するキーコード（文字入力を想定した付番）
    typedef uint8_t keycode_t;


// private:
public:
    // キーボードコントローラのI2Cバス上でのアドレスは 0x21（7ビットアドレス。RWビットを含まない）
    static constexpr uint8_t target_i2c_addr = 0x21;

    // キーボードコントローラから報告されるレポートのデータ長は7バイト（キーコード6バイトと終端0xFF）
    static constexpr uint8_t READDATACOUNT = 7;

    // キーボードコントローラから報告されるキーコードは6個（各1バイト）
    static constexpr uint8_t RAWKEYCODECOUNT = 6;

    // キーボードコントローラから報告されるキーコード（物理レイアウトに近い番号）
    typedef uint8_t rawkeycode_t;

    byte prev_report[7];
    byte latest_report[7];

    // 記憶しておく最大キー数（この数のキーが同時に押下された場合までは正常に動作する）
    // TODO: これを超えた場合に、安全にエラーを吐くようにする
    static constexpr uint8_t MAXKEYS = 6;

    // keycode_t keycodes_pressing[6];
    keycode_t buffered_keydown[6];

    unsigned long key_pressing_millis[MAXKEYS] = { 0 };

    /**
      @brief サブプロセッサからキーボードの入力状態を読み取り、指定のバッファへ格納する
      */
    bool read_report_from_keyboardcontroller(rawkeycode_t *dst);

    /**
      @brief サブプロセッサからキーボードの入力状態を読み取り、現在状態として更新する
      */
    bool read_report(void);

public:
    /**
     @brief 使用前の初期化処理。
    */
    void init(void);

    /* ---- バッファ付き入力 ----
       入力をバッファし、ひとつずつ処理する場合。長押し時のキーリピートも処理する（未実装）。
       おもに文字入力を想定
     */

    /** キー状態を更新する
     */
    void update(void);

    /** 新たに押下されたキーを取得する
      */
    uint8_t get_key(void);

    /** 未処理のキーを破棄する
      */
    void flush(void);


    /* ---- 押下状態の取得 ----
       現在のキー押下状況を返す ( update() の呼び出し時点での判定に基づく)
     */

    /** 指定されてキーがいま押下されているか 
     *  @param keycode [IN] 調べたいキーのキーコード
     *  @return 押下中ならtrue
     */
    // bool is_key_down(keycode_t key);


    /** 指定されてキーがいま押下されているか
     *  @param keycode [IN] 調べたいキーのキーコード
     *  @return 押下中ならtrue
     */
    bool is_key_down_rawkeycode(rawkeycode_t key);

    /** 最新の押下状況のレポートを書き出す
     * @param dst [OUT]
     * @param dstlen [IN]
     * @return レポートの個数
     */
    uint8_t get_report(byte* dst, size_t dstlen);

    /* ---- ユーティリティ系 ----
       よく使われそうな機能をあらかじめ実装しておいたもの
     */

    /** なにかキーが押されるまで待機する
     */
    void wait_any_key(void);

    /** 指定されたキーが押されるまで待機する
     *  @param keycode [IN] 押されるのを待機するキーのキーコード
     */
    void wait_key(keycode_t key);

    /** すべてのキーが押されなくなるまで待機する
     */
    void wait_allkey_released(void);



    /** ---- キーコード一覧 ---- */

    static constexpr uint8_t KEYCODE_NONE = 0x00;

    // Assigned from ASCII Control code
    static constexpr uint8_t KEYCODE_ESC = 0x1b;
    static constexpr uint8_t KEYCODE_BACKSPACE = 0x08;
    static constexpr uint8_t KEYCODE_ENTER = 0x0D;  // Carriage return
    static constexpr uint8_t KEYCODE___ENTER = 0x0A;  // (Hidden assign) Left feed
    static constexpr uint8_t KEYCODE_DELETE = 0x7F;
    static constexpr uint8_t KEYCODE_SPACE = ' ';

    static constexpr rawkeycode_t RAWKEYCODE_ESC = 1;
    static constexpr rawkeycode_t RAWKEYCODE_BACKSPACE = 23;
    static constexpr rawkeycode_t RAWKEYCODE_ENTER = 35;
    static constexpr rawkeycode_t RAWKEYCODE_SPACE = 41;

    // Original code

    static constexpr uint8_t KEYCODE_HOME = 0x81;
    static constexpr uint8_t KEYCODE_END = 0x82;
    static constexpr uint8_t KEYCODE_PAGEUP = 0x83;
    static constexpr uint8_t KEYCODE_PAGEDOWN = 0x84;

    static constexpr uint8_t KEYCODE_ARROWLEFT = 0x85;
    static constexpr uint8_t KEYCODE_ARROWDOWN = 0x86;
    static constexpr uint8_t KEYCODE_ARROWUP = 0x87;
    static constexpr uint8_t KEYCODE_ARROWRIGHT = 0x88;

    static constexpr uint8_t KEYCODE_MUHENKAN = 0x89;
    static constexpr uint8_t KEYCODE_HENKAN_HIRA_KANA = 0x8A;

    static constexpr uint8_t KEYCODE_FUNCTION_1 = 0x8B;
    static constexpr uint8_t KEYCODE_FUNCTION_2 = 0x8C;
    static constexpr uint8_t KEYCODE_FUNCTION_3 = 0x8D;
    
    static constexpr rawkeycode_t RAWKEYCODE_FUNCTION_1 = 38;
    static constexpr rawkeycode_t RAWKEYCODE_FUNCTION_2 = 39;
    static constexpr rawkeycode_t RAWKEYCODE_FUNCTION_3 = 43;

    static constexpr rawkeycode_t RAWKEYCODE_SHIFT_LEFT = 25;
    static constexpr rawkeycode_t RAWKEYCODE_SHIFT_RIGHT = 46;
};
