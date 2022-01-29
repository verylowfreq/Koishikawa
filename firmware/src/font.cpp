#include "font.h"

#include <assert.h>

#include <debug.h>


bool FontManager::init(FileAccessWrapper* file, uint8_t height, uint8_t halfwidth, uint8_t fullwidth) {

    if (!file) {
        DEBUG("file is NULL");
        return false;
    }
    if (!file->is_opened()) {
        DEBUG("file not opened.");
        return false;
    }
    this->fontfile = file;
    this->FONT_HEIGHT = height;
    this->FONT_WIDTH_SINGLEBYTE = halfwidth;
    this->FONT_WIDTH_DOUBLEBYTE = fullwidth;
    this->GLYPH_LENGTH_SINGLEBYTE = this->FONT_WIDTH_SINGLEBYTE * ((this->FONT_HEIGHT + 7) / 8);
    this->GLYPH_LENGTH_DOUBLEBYTE = this->FONT_WIDTH_DOUBLEBYTE * ((this->FONT_HEIGHT + 7) / 8);
    this->font_loaded = true;

    return true;
}


uint32_t FontManager::get_ku_offset(uint8_t ku) {
    if (!this->font_loaded) {
        Serial.println("get_ku_offset(): font not loaded.");
        return INVALID_UINT32_VALUE;
    }
    this->fontfile->seek(0);
    constexpr size_t buflen = 128;
    byte buf[buflen];
    uint32_t offset = 0;
    this->fontfile->read(buf, buflen);
    uint32_t headertail = GLOBAL_HEADER_LENGTH + (INDEX_HEADER_LENGTH - 3) + (((uint32_t)buf[4] << 16) | ((uint32_t)buf[3] << 8) | (uint32_t)buf[2]);
    offset = GLOBAL_HEADER_LENGTH + INDEX_HEADER_LENGTH;
    do {
        // Serial.print("get_ku_offset(): read offset=");
        // Serial.println(offset);
        this->fontfile->seek(offset);
        this->fontfile->read(buf, buflen);
        uint32_t endpos = (buflen / 4) * 4;
        for (uint32_t i = 0; i < endpos; ) {
            uint8_t cur_ku = buf[i];
            if (cur_ku == ku) {
                uint32_t cur_offset = (((uint32_t)buf[i+3] << 16) | ((uint32_t)buf[i+2] << 8) | (uint32_t)buf[i+1]);
                // Serial.print("get_ku_offset(): found ku offset=");
                // Serial.println(cur_offset);
                return cur_offset;
            }
            i += 4;
        }
        offset += buflen;
    } while (offset < headertail);

    Serial.print("get_ku_offset(): Ku not found. Ku=");
    Serial.println(ku);

    return INVALID_UINT32_VALUE;
}


uint32_t FontManager::get_glyph_offset(uint8_t ku, uint8_t ten) {
    uint32_t ku_offset = this->get_ku_offset(ku);
    if (ku_offset == INVALID_UINT32_VALUE) {
        return INVALID_UINT32_VALUE;
    }
    uint8_t length_per_glyph;
    if (ku == 0) {
        length_per_glyph = GLYPH_LENGTH_SINGLEBYTE;
    } else {
        length_per_glyph = GLYPH_LENGTH_DOUBLEBYTE;
        ten -= 1;
    }
    uint32_t glyph_offset = ku_offset + (uint32_t)length_per_glyph * ten;
    //DEBUG:
    // Serial.print("get_glyph_offset(): ku_offset=");
    // Serial.print(ku_offset);
    // Serial.print(", glyph_offset=");
    // Serial.println(glyph_offset);
    return glyph_offset;
}


bool FontManager::get_glyph_from_kuten(uint8_t ku, uint8_t ten, byte* dst, uint8_t* width, uint8_t* height) {
    uint32_t glyph_offset = this->get_glyph_offset(ku, ten);
    if (glyph_offset == INVALID_UINT32_VALUE) {
        return false;
    }
    *height = FONT_HEIGHT;
    uint8_t length_per_glyph;
    if (ku == 0) {
        length_per_glyph = GLYPH_LENGTH_SINGLEBYTE;
        *width = FONT_WIDTH_SINGLEBYTE;
    } else {
        length_per_glyph = GLYPH_LENGTH_DOUBLEBYTE;
        *width = FONT_WIDTH_DOUBLEBYTE;
    }

    if ((ku > 94) || (ku != 0 && ten > 94)) {
        Serial.print("FontManager::get_glyph_from_kuten():Invalid KuTen. Ku,Ten=");
        Serial.print(ku); Serial.print(", "); Serial.println(ten);
        ku = 1;
        ten = 94;
    }

    byte* ptr = this->lookup_cache(ku, ten);
    if (ptr) {
        // Serial.println("Font glyph is on cache.");
        memcpy(dst, ptr, length_per_glyph);
        return true;
    }

    // uint8_t glyphbytes = *width * 2;
    int readlen = 0;
    this->fontfile->seek(glyph_offset);
    bool failed_read_glyph = false;
    for (int readcnt = 0; readcnt < length_per_glyph; ++readcnt) {
        int readbyte = this->fontfile->read();
        if (readbyte < 0) {
            //DEBUG:
            Serial.print("File.read() returned invalid value at ");
            Serial.print(glyph_offset + readcnt);
            Serial.print(", KuTen=");
            Serial.print(ku); Serial.print(","); Serial.print(ten);
            Serial.println();
            // Serial.println(". Re-open font file.");
            readbyte = 0xF0;
            failed_read_glyph = true;
        }

        dst[readcnt] = (byte)readbyte;
        ++readlen;
    }

    // Add to cache if read
    if (!failed_read_glyph) {
        this->add_to_cache(ku, ten, dst, length_per_glyph);
    }

    // for (int i = 0; i < 3; ++i) {
    //     this->fontfile.seek(glyph_offset);
    //     readlen = this->fontfile.read(dst, length_per_glyph);

    //     if (readlen == glyphbytes) {
    //         return true;

    //     } else if (readlen <= 0) {
    //         //DEBUG:
    //         Serial.print("File.read() returned ");
    //         Serial.print(readlen);
    //         Serial.print(", KuTen=");
    //         Serial.print(ku); Serial.print(","); Serial.print(ten);
    //         Serial.println(". Re-open font file.");

    //         this->fontfile.close();
    //         this->fontfile = SD.open(this->fontfilepath, FILE_READ);
    //         delay(100);
    //     }
    // }

    //DEBUG:
    if (failed_read_glyph) {
        Serial.print("FontManager::get_glyph_from_kuten(): Error in reading glyph. readlen=");
        Serial.print(readlen); Serial.print(", glyph_offset="); Serial.println(glyph_offset);
    }

    return true;
}


/*
キャッシュのメモリ構造
Glyphs:
    (1byte) Ku
    (1byte) Ten
    (nbyte) Glyph data
    (nbyte) padding 1バイト文字の場合、2バイト文字と同じバイト数になるまでパディングする

リングバッファとし、もっとも追加されたものを追い出す。
*/


void FontManager::add_to_cache(uint8_t ku, uint8_t ten, byte* data, uint16_t data_length) {
    if (!this->cache_buffer || this->cache_length < 1) {
        return;
    }
    
    uint16_t glyphobjectbytes_in_cache = 1 + 1 + this->GLYPH_LENGTH_DOUBLEBYTE;
    byte* dstptr = this->cache_tail;

    *dstptr++ = ku;
    *dstptr++ = ten;
    memcpy(dstptr, data, data_length);
    dstptr += data_length;
    // 2バイト文字に満たない文をパディングする
    for (int i = data_length; i < this->GLYPH_LENGTH_DOUBLEBYTE; i++) {
        *dstptr++ = 0xFF;
    }


    if (dstptr + glyphobjectbytes_in_cache > this->cache_buffer + this->cache_length) {
        // 次を書き込む余裕はないので、先頭へ戻す
        this->cache_tail = this->cache_buffer;
    } else {
        this->cache_tail = dstptr;
    }

    // D("buf=%p, buflen=%d, tail=%p\n", this->cache_buffer, this->cache_length, this->cache_tail);
}


byte* FontManager::lookup_cache(uint8_t ku, uint8_t ten) {
    if (!this->cache_buffer || (this->cache_length < 1)) {
        return nullptr;
    }
    byte* ptr = this->cache_buffer;
    byte* cache_end = this->cache_buffer + this->cache_length;
    while (ptr < cache_end) {
        uint8_t cur_ku = *ptr++;
        uint8_t cur_ten = *ptr++;
        if (cur_ku == ku && cur_ten == ten) {
            return ptr;
        } else {
            ptr += this->GLYPH_LENGTH_DOUBLEBYTE;
            continue;
        }
    }
    return nullptr;
}


void FontManager::enable_cache(byte* buffer, uint16_t cachesize) {
    assert(buffer);
    assert(cachesize > 0);

    this->cache_buffer = buffer;
    this->cache_length = cachesize;
    this->cache_tail = buffer;
}


void FontManager::disable_cache(void) {
    this->cache_buffer = nullptr;
    this->cache_length = 0;
    this->cache_tail = nullptr;
}
