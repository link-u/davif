# コマンドラインオプションの説明

## 入出力

### 入力

 - `-i input.avif`（必須）

### 出力

 - `-o output.png`（必須）

### Alpha/Depth画像の展開

 - `--extract-alpha <alpha.png>`
 - `--extract-depth <depth.png>`

付与されているalpha/depthの画像を別ファイルにデコードして保存する。

このオプションでalphaを展開しても、`-o <output.png>`で指定したファイルには引き続きalphaは適用される。

## マルチスレッド

### 利用スレッド数

- `--threads <num-threads>`
  - 初期値：論理コア数（`nproc`コマンドで確認可能）

注意点：

 - `--threads 0` の場合、メインスレッドのみでデコードする。
 - `--threads 1` の場合、メインスレッドとは別にもう1つスレッドが建ち、一応多少早くなる。
