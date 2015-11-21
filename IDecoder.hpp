#ifndef __IDECODER_HPP
#define __IDECODER_HPP

#include "Types.hpp"
#include "AudioTranscoder.hpp"

struct IDecoder {
	using sample_t = Types::sample_t;
	using byte_t = Types::byte_t;
	using BUFFERTYPE = AudioTranscoder::BUFFERTYPE;

	enum ERRORCODE {
		ERRORCODE_NONE = 0,					// エラー無し
		ERRORCODE_UNKNOWN_ERROR,			// 不明なエラー
		ERRORCODE_INTERNAL_ERROR,			// 内部エラー
		ERRORCODE_INVALID_ARGUMENT,			// 引数が不正
		ERRORCODE_INVALID_FORMAT,			// データのフォーマットが不正
		ERRORCODE_CANNOT_LOAD_STREAM,		// データを読み込めなかった
		ERRORCODE_UNSUPPORTED_FORMAT,		// 対応していないフォーマット
	};

	// 継承クラス以外からはRead-onlyでお願い
	// getterを使用する実装にすると読みづらいしオーバーヘッドが気になるので
	int channelNum;					// チャンネル数
	int samplingRate;				// サンプリングレート
	sample_t bufferLength;			// 総サンプル数
	BUFFERTYPE bufferType;			// データの型

private:
	virtual sample_t _Tell() = 0;								// バッファの位置を取得
	virtual void _Seek(sample_t position) = 0;					// バッファの位置を設定
	virtual sample_t _Read(void *ptr, sample_t length) = 0;		// バッファ読み取り

public:
	IDecoder() {
		// 一応0クリアしておく
		channelNum = 0;
		samplingRate = 0;
		bufferLength = 0;
		bufferType = BUFFERTYPE::BUFFERTYPE_FIRST__;
	}

	virtual ~IDecoder() {};


	// サンプル→バイト
	byte_t SampleToByte(sample_t sample) {
		return (byte_t)(sample * (channelNum * SizeOf(bufferType)));
	}

	// バイト→サンプル
	sample_t ByteToSample(byte_t byte) {
		return (sample_t)(byte / (channelNum * SizeOf(bufferType)));
	}


	// バッファの位置を取得
	auto Tell() {
		return _Tell();
	}

	// バッファの位置を設定
	auto Seek(sample_t position) {
		if (position > bufferLength) {
			throw ERRORCODE_INVALID_ARGUMENT;
		}
		_Seek(position);
	}

	// バッファ読み取り
	auto Read(void *ptr, sample_t length) {
		return _Read(ptr, length);
	}
};

#endif