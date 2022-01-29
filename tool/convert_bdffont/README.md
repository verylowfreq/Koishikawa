# BDFフォントを独自バイナリ形式へ変換するプログラム

## 使い方

以下を実行すると、BDF形式のフォント "shnm7x14a.bdf" と "shnmk14.bdf" をもとに独自バイナリ形式のファイル "fnt14jis.bin" を出力する。これをmicroSDのルートディレクトリへ "fnt14jis.fnt" として手動でコピーする。

`python convert_bdffont.py`


## 制約

東雲フォントファミリの14ドットゴシックの変換でのみ使用しており、これ以外での動作は不明。

[東雲 ビットマップフォントファミリー](http://openlab.ring.gr.jp/efont/shinonome/)
