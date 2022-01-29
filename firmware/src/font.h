#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <Arduino.h>

#include <FileAccessWrapper.h>


class FontManager {
public:
    static constexpr uint32_t INVALID_UINT32_VALUE = 0xFFFFFFFF;
    static constexpr uint16_t INVALID_UINT16_VALUE = 0xFFFF;

    uint8_t FONT_HEIGHT = 0;
    uint8_t FONT_WIDTH_SINGLEBYTE = 0;
    uint8_t FONT_WIDTH_DOUBLEBYTE = 0;

    static constexpr uint8_t GLOBAL_HEADER_LENGTH = 2 + 3;  // 'FN' + 3byte LE length
    static constexpr uint8_t INDEX_HEADER_LENGTH = 2 + 3;   // 'TB' + 3byte LE length
    // static constexpr uint8_t GLYPH_LENGTH_DOUBLEBYTE = FONT_WIDTH_DOUBLEBYTE * ((FONT_HEIGHT + 7) / 8);
    // static constexpr uint8_t GLYPH_LENGTH_SINGLEBYTE = FONT_WIDTH_SINGLEBYTE * ((FONT_HEIGHT + 7) / 8);
    uint8_t GLYPH_LENGTH_DOUBLEBYTE = 0;
    uint8_t GLYPH_LENGTH_SINGLEBYTE = 0;

// private:
public:
    uint32_t get_ku_offset(uint8_t ku);
    uint32_t get_glyph_offset(uint8_t ku, uint8_t ten);

    byte* cache_buffer = nullptr;
    uint16_t cache_length = 0;
    byte* cache_tail = nullptr;

    /** 指定のグリフのキャッシュを探す
     * @param ku
     * @param ten
     * @return キャッシュ内に見つかればそのポインタ、見つからなければnullptr
     */
    byte* lookup_cache(uint8_t ku, uint8_t ten);

    void add_to_cache(uint8_t ku, uint8_t ten, byte* data, uint16_t data_length);

public:

    bool font_loaded = false;
    FileAccessWrapper* fontfile;

    bool init(FileAccessWrapper* file, uint8_t height, uint8_t halfwidth, uint8_t fullwidth);

    /** 区点番号をもとにグリフを取得する
      @param ku [IN] 
      @param ten [IN]
      @param dst [OUT] 
      @param width [OUT]
      @param height [OUT]
      @return グリフの取得に成功したらtrue
      */
    bool get_glyph_from_kuten(uint8_t ku, uint8_t ten, byte* dst, uint8_t* width, uint8_t* height);

    /** 指定サイズのキャッシュを有効にする
     * @param buffer
     * @param cachesize キャッシュで利用するメモリのバイト数
     */
    void enable_cache(byte* buffer, uint16_t cachesize);

    /** キャッシュを利用しない */
    void disable_cache(void);
};
