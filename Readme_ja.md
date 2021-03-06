# AudioPlayer
シンプルなオレオレ音声再生ライブラリ
Windows専用

## 機能
- 4GB以上の音声データの読み込み
- 複数音声の同時再生
	- 実装上は最低でも32767の音声が同時再生できる
	- ビルド環境によってはもっと多くなる（2147483647とか）
- ファイルからの音声の読み込み
- メモリからの音声の読み込み
- シーク
- ボリューム設定
- ギャップレスループ再生
	- 全体ループ
	- 区間ループ（ABループ）
- [HSP3](http://hsp.tv/)向けのAPI

## 使用可能な入力フォーマット
- FLAC / Ogg FLAC
	- 8ビット、16ビット、24ビット、32ビットの整数型
- Ogg Opus
- Ogg Vorbis
- WAV (Linear PCM)
	- 8ビットまたは16ビットの整数型
	- 24ビットまたは32ビットの整数型（非標準かもしれない）
	- 32ビットの浮動小数点数型

## 使用可能な出力フォーマット
（リニアPCMのみ）
- 8ビットまたは16ビットの整数型
- 24ビットまたは32ビットの整数型（非標準かもしれない）
- 32ビットの浮動小数点数型