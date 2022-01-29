#include <FileAccessWrapper.h>

#include <stdio.h>
#include <assert.h>


class CstdioFileAccessor: public FileAccessWrapper {
public:

    FILE* file = nullptr;

    /** ファイルを開く
     * @param path ファイルのパス（プラットフォーム依存）
     * @param mode ファイルを開くモード（プラットフォーム依存）
     * @return ファイルを開けたらtrue、開けなかったらfalse
     */
    bool open(const char* path, FileMode mode) override {
        assert(mode == FileMode::READ);
        this->file = fopen(path, "rb");
        return this->file != nullptr;
    }

    /** ファイルを既に開いているか否か
     * @return ファイルが開かれていればtrue、それ以外はfalse
     */
    virtual bool is_opened(void) {
        return this->file != nullptr;
    }

    /** ファイルを閉じる
     */
    virtual void close(void) {
        if (this->is_opened()) {
            fclose(this->file);
            this->file = nullptr;
        }
    }

    /** 1バイトを読み込む
     * @return 読み込んだ1バイト、読み込めなかったら負の値
     */
    virtual int read(void) {
        uint8_t ch;
        int readlen = fread(&ch, 1, 1, this->file);
        assert(readlen == 1);
        return ch;
    }

    /** ファイルの現在の読み込み位置を取得する
     * @return 現在位置
     */
    virtual uint32_t position(void) {
        // Not C standard compliant but works.
        uint32_t pos = (uint32_t)ftell(this->file);
        return pos;
    }

    /** ファイルの現在の読み込み位置を設定する
     * @param pos 
     * @return 新しいファイル位置
     */
    virtual uint32_t seek(uint32_t pos) {
        fseek(this->file, pos, SEEK_SET);
        return this->position();
    }
};
