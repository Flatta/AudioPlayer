#include "Config.hpp"
#ifdef ENABLE_WAV

#define NOMINMAX

#include "WavDecoder.hpp"
#include <algorithm>
#include <Windows.h>
#include <Mmreg.h>


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t = IDecoder::byte_t;


// コンストラクタ
WavDecoder::WavDecoder(unique_ptr<istream> p_input) : input(move(p_input)) {
	// 例外設定
	input->exceptions(istream::failbit | istream::badbit);

	// inputのサイズ取得
	input->seekg(0, ios::end);
	byte_t size = input->tellg();
	input->seekg(0);

	// サイズチェック
	if (size < 12) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// RIFFチャンク、WAVEチャンク検査
	uint32_t audioBufInt[3];
	input->read((char *)audioBufInt, sizeof(audioBufInt));
	if (audioBufInt[0] != (uint32_t)0x46464952/*'RIFF'*/ || audioBufInt[2] != (uint32_t)0x45564157/*'WAVE'*/) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// RIFFチャンクのサイズチェックは面倒なので省略
	
	// fmt チャンク、dataチャンク探索
	dataOffset = 0;
	dataSize = 0;
	byte_t fmtOffset = 0;
	while (true) {
		uint32_t chunkSize;
		uint32_t chunkCode;

		// ヘッダサイズ分あるか確認
		if ((byte_t)input->tellg() + 8 > size) {
			break;
		}

		// チャンクの識別子とサイズを取得
		input->read((char *)&chunkCode, sizeof(chunkCode));
		input->read((char *)&chunkSize, sizeof(chunkSize));

		// チャンクサイズ分あるか確認
		if ((byte_t)input->tellg() + chunkSize > size) {
			break;
		}

		// 識別子で処理振り分け
		switch (chunkCode) {
			// 'fmt '
			case 0x20746D66:
				// チャンクがPCMWAVEFORMATを格納しているか確認
				// WAVEFORMATEXにも対応するためチャンクがPCMWAVEFORMATより大きくても許容する
				if (chunkSize >= sizeof(PCMWAVEFORMAT)) {
					fmtOffset = input->tellg();
				}
				break;

			// 'data'
			case 0x61746164:
				dataOffset = input->tellg();
				dataSize = chunkSize;
				break;
		}

		// 必要なチャンクが全て見つかったなら抜ける
		if (fmtOffset > 0 && dataOffset > 0) {
			break;
		}

		// データを読み飛ばす
		input->seekg(chunkSize, ios::cur);
	}

	// 'fmt 'チャンクか'data'チャンク、またはその両方が見つからなかった
	if (fmtOffset == 0 || dataOffset == 0) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// フォーマット情報コピー
	PCMWAVEFORMAT pcmwf;
	input->seekg(fmtOffset);
	input->read((char *)&pcmwf, sizeof(PCMWAVEFORMAT));

	// バッファタイプ決定（＆対応フォーマットか確認）
	if (pcmwf.wf.wFormatTag == WAVE_FORMAT_PCM) {
		// 整数型のリニアPCM
		if (pcmwf.wBitsPerSample == 32) {
			// 32ビットリニアPCM（非標準？）
			bufferType = BUFFERTYPE::BUFFERTYPE_INT32;
		} else if (pcmwf.wBitsPerSample == 24) {
			// 24ビットリニアPCM（非標準？）
			bufferType = BUFFERTYPE::BUFFERTYPE_INT24;
		} else if (pcmwf.wBitsPerSample == 16) {
			// 16ビットリニアPCM
			bufferType = BUFFERTYPE::BUFFERTYPE_INT16;
		} else if (pcmwf.wBitsPerSample == 8) {
			// 8ビットリニアPCM
			bufferType = BUFFERTYPE::BUFFERTYPE_INT8U;
		} else {
			// 不正なビット数
			throw ERRORCODE_INVALID_FORMAT;
		}
	} else if (pcmwf.wf.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
		// Float型のリニアPCM
		bufferType = BUFFERTYPE::BUFFERTYPE_FLOAT;
	} else {
		// 不明なフォーマット
		throw ERRORCODE_UNSUPPORTED_FORMAT;
	}

	// フォーマット情報チェック
	// 自前で計算したものをセットしてしまっても良いかもしれない
	if (pcmwf.wf.nAvgBytesPerSec != pcmwf.wf.nBlockAlign * pcmwf.wf.nSamplesPerSec || pcmwf.wf.nBlockAlign != pcmwf.wf.nChannels * (pcmwf.wBitsPerSample / 8)) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// データサイズがブロック境界に揃っているかチェック
	// 無理やりブロック境界に揃えてしまっても良いかもしれない
	if (dataSize % pcmwf.wf.nBlockAlign != 0) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// 情報設定
	channelNum = pcmwf.wf.nChannels;
	samplingRate = pcmwf.wf.nSamplesPerSec;
	bufferLength = dataSize / pcmwf.wf.nBlockAlign;

	// 位置初期化
	input->seekg(dataOffset);
}


// 以下インターフェイス
sample_t WavDecoder::_Tell() {
	return ByteToSample((byte_t)input->tellg() - dataOffset);
}

void WavDecoder::_Seek(sample_t position) {
	if (position > bufferLength) throw ERRORCODE_INVALID_ARGUMENT;
	if (input->eof() && input->fail()) input->clear();
	input->seekg(SampleToByte(position) + dataOffset);
}

sample_t WavDecoder::_Read(void *ptr, sample_t length) {
	sample_t readSize = std::min(length, bufferLength - _Tell());
	input->read((char *)ptr, SampleToByte(readSize));
	return readSize;
}

#endif