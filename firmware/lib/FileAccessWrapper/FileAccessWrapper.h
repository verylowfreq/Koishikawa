#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/** ファイルアクセスを抽象化するクラス。実装はプラットフォーム依存
 */
class FileAccessWrapper {
public:
    // ---- プラットフォーム依存の実装が必要なメソッド ----

    enum class FileMode {
        READ,
        WRITE,
        READWRITE
    };

    /** ファイルを開く
     * @param path ファイルのパス（プラットフォーム依存）
     * @param mode ファイルを開くモード（プラットフォーム依存）
     * @return ファイルを開けたらtrue、開けなかったらfalse
     */
    virtual bool open(const char* path, FileMode mode) = 0;

    /** ファイルを既に開いているか否か
     * @return ファイルが開かれていればtrue、それ以外はfalse
     */
    virtual bool is_opened(void) = 0;

    /** ファイルを閉じる
     */
    virtual void close(void) = 0;

    /** 1バイトを読み込む
     * @return 読み込んだ1バイト、読み込めなかったら負の値
     */
    virtual int read(void) = 0;

    /** ファイルの現在の読み込み位置を取得する
     * @return 現在位置
     */
    virtual uint32_t position(void) = 0;

    /** ファイルの現在の読み込み位置を設定する
     * @param pos 
     * @return 新しいファイル位置
     */
    virtual uint32_t seek(uint32_t pos) = 0;

    /** ファイルの現在の読み込み位置を指定差分だけずらす
     * @param posdelta
     * @return 新しいファイル位置
     */
    uint32_t seek_delta(int32_t posdelta) {
        return this->seek(this->position() + posdelta);
    }


    // ---- デフォルト実装のあるメソッド ----

    virtual int read(uint8_t* buf, size_t buflen) {
        for (size_t i = 0; i < buflen; i++) {
            buf[i] = (uint8_t)this->read();
        }
        return buflen;
    }

    virtual uint8_t read_uint8(void) {
        return (uint8_t)this->read();
    }

    virtual uint16_t read_uint16(void) {
        uint16_t val = 0;
        val += (uint8_t)this->read();
        val += (uint16_t)((uint8_t)this->read()) << 8;
        return val;
    }
    virtual uint32_t read_uint24(void) {
        uint32_t val = 0;
        val += (uint8_t)this->read();
        val += (uint32_t)((uint8_t)this->read()) << 8;
        val += (uint32_t)((uint8_t)this->read()) << 16;
        return val;
    }
    
    virtual uint32_t read_uint32(void) {
        uint32_t val = 0;
        val += (uint8_t)this->read();
        val += (uint32_t)((uint8_t)this->read()) << 8;
        val += (uint32_t)((uint8_t)this->read()) << 16;
        val += (uint32_t)((uint8_t)this->read()) << 24;
        return val;
    }
};
