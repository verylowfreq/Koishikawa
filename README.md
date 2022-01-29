# Koishikawa

Building a Japanese Kanji wordprocessor.

日本語の漢字変換ができる端末デバイスの製作。

まだまだ開発中です。

なお、本レポジトリは公開用レポジトリです。非公開の開発用レポジトリから更新をまとめて取り込んでいます。


## 製作物の概要

 - PlatformIOとArduino環境での開発。
 - AVR ATmega4809-PFをコアとした構成。
 - SKKによる漢字変換。
 - 内部の文字コードはShiftJIS。
 - ストレージをmicroSD/SDHCとすることによる、ファイルサイズ制約の緩和。
 - 表示デバイスは122x32ドットのグラフィック液晶。
 - 文字入力を主眼とした、コンパクトな独自配列のキーボード。
 - SKK辞書とフォントは独自のバイナリ形式へ変換したうえでmicroSDへ配置。
 - 外部へのシリアル通信での送出が可能。
 - 安価なレシートプリンタでの印刷を想定した、GB18030へのテキストエンコーディング変換。


## 構成コンポーネント

 - 基板 : 独自設計。おおむねB5サイズ。
 - AVR ATmega4809-PF : システムのコアとなるチップ。
 - AVR ATmega328P-PU : 補助用途のチップ。キーボードのスキャンをする。ATmga4809-PFとはI2Cで通信。
 - SG12232C : 122x32ドットのグラフィック液晶モジュール。
 - M5Stamp C3 (ESP32-C3) : WiFi経由でSSH接続を提供する。


 ## ファイル構成
 
  - README.md
  - firmware/ : ATmega4809-PFのファームウェア
  - firmware-m328p/ : ATmega328P-PUのファームウェア
  - firmware-esp32c3/ : M5Stamp C3 (ESP32-C3)のファームウェア
  - tool/ : ホストで実行するツール
    - convert_skkdict/ : SKK辞書を変換するプログラム
    - convert_bdffont/ : BDF形式のフォントを変換するプログラム
    - generate_romajitable/ : ローマ字変換テーブルを生成するプログラム
    - generate_sjisgb18030dict/ : ShiftJISからGB18030への変換テーブルを生成するプログラム
  - doc/ : ドキュメント
  - schematic/ : 回路および基板


## ライセンス

ライセンスは個別のファイル、もしくはディレクトリ内のLICENSEファイルに記載してあります。記載がない場合は以下の通りMIT Licenseとします。

```
Copyright 2022 verylowfreq ( https://github.com/verylowfreq/ )

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
