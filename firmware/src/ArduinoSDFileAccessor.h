#pragma once

#include <FileAccessWrapper.h>

#include <SD.h>


class ArduinoSDFileAccessor : public FileAccessWrapper {
private:
    bool _is_opened = false;
    File file;
    FileMode mode;

public:

    /** ファイルを開く
     * @param path ファイルのパス（プラットフォーム依存）
     * @param mode ファイルを開くモード（プラットフォーム依存）
     * @return ファイルを開けたらtrue、開けなかったらfalse
     */
    virtual bool open(const char* path, FileMode mode) override;

    /** ファイルを既に開いているか否か
     * @return ファイルが開かれていればtrue、それ以外はfalse
     */
    virtual bool is_opened(void) override;

    /** ファイルを閉じる
     */
    virtual void close(void) override;

    /** 1バイトを読み込む
     * @return 読み込んだ1バイト、読み込めなかったら負の値
     */
    virtual int read(void) override;

    /** ファイルの現在の読み込み位置を取得する
     * @return 現在位置
     */
    virtual uint32_t position(void) override;

    /** ファイルの現在の読み込み位置を設定する
     * @param pos 
     * @return 新しいファイル位置
     */
    virtual uint32_t seek(uint32_t pos) override;
};
