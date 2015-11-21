#include "Config.hpp"
#ifdef ENABLE_FLAC

#include "FLACDecoder.hpp"
#include "Int24.hpp"

#undef min				// fuck
#include <algorithm>


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t = IDecoder::byte_t;


// コンストラクタ
FLACDecoder::FLACDecoder(unique_ptr<istream> p_input, bool p_checkMD5) : input(move(p_input)), FLAC::Decoder::Stream() {
	// メンバ変数初期化
	error = ERRORCODE_NONE;
	streamPosition = 0;
	initialized = false;

	// メンバ変数設定
	checkMD5 = p_checkMD5;

	// inputのサイズ取得
	input->seekg(0, ios::end);
	inputSize = input->tellg();
	input->seekg(0);

	// マジックコード判別
	uint32_t magic = 0x00000000;
	input->read((char *)&magic, sizeof(magic));
	input->seekg(0);

	switch (magic) {
		case 0x43614C66:	// fLaC
			oggFLAC = false;
			break;

		case 0x5367674F:	// OggS
			oggFLAC = true;
			break;

		default:
			throw ERRORCODE_INVALID_FORMAT;
	}

	// 初期化
	FLAC__StreamDecoderInitStatus initStatus = oggFLAC ? init_ogg() : init();
	if (initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// 設定
	set_md5_checking(checkMD5);

	// メタデータ取得
	if (!process_until_end_of_metadata()) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	initialized = true;
}


// 以下インターフェイス
sample_t FLACDecoder::_Tell() {
	if (error != ERRORCODE_NONE) throw error;
	return streamPosition;
}

void FLACDecoder::_Seek(sample_t position) {
	if (error != ERRORCODE_NONE) throw error;

	// Finished?
	if (get_state() == FLAC__STREAM_DECODER_END_OF_STREAM) {
		reset();
		set_md5_checking(checkMD5);
		process_until_end_of_metadata();
	}

	if (!seek_absolute(position)) {
		//reset();
		error = error != ERRORCODE_NONE ? error : ERRORCODE_UNKNOWN_ERROR;
		throw error;
	}
	streamPosition = position;
	stream.clear();
}

sample_t FLACDecoder::_Read(void *ptr, sample_t length) {
	if (error != ERRORCODE_NONE) throw error;

	// Read
	while (stream.size() < length * channelNum && error == ERRORCODE_NONE && (
			get_state() == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC ||
			get_state() == FLAC__STREAM_DECODER_READ_FRAME
		)) {
		process_single();
	}
	// エラー処理
	if (error != ERRORCODE_NONE || (
		get_state() != FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC &&
		get_state() != FLAC__STREAM_DECODER_READ_FRAME &&
		get_state() != FLAC__STREAM_DECODER_END_OF_STREAM
	)) {
		throw error != ERRORCODE_NONE ? error : ERRORCODE_UNKNOWN_ERROR;
	}
	// 読み取りサイズ計算
	sample_t readSize = min((sample_t)stream.size() / channelNum, length);
	// 格納
	// とりあえずパフォーマンス悪そう
	for (sample_t i = 0; i < readSize * channelNum; i++) {
		FLAC__int32 value = stream[i];
		// バッファタイプにより処理振り分け
		switch (bufferType) {
			case BUFFERTYPE::BUFFERTYPE_INT8:  ((int8_t  *)ptr)[i] = value; break;
			case BUFFERTYPE::BUFFERTYPE_INT16: ((int16_t *)ptr)[i] = value; break;
			case BUFFERTYPE::BUFFERTYPE_INT24: ((int24_t *)ptr)[i] = value; break;
			case BUFFERTYPE::BUFFERTYPE_INT32: ((int32_t *)ptr)[i] = value; break;
			// Unreachable code
			default:
				throw ERRORCODE_INTERNAL_ERROR;
		}
	}
	// 格納した分を削除
	stream.erase(stream.begin(), stream.begin() + readSize * channelNum);
	// 位置更新
	streamPosition += readSize;
	return readSize;
}


// callbacks
::FLAC__StreamDecoderReadStatus FLACDecoder::read_callback(FLAC__byte buffer[], size_t *bytes) {
	if (error != ERRORCODE_NONE) return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	try {
		input->read((char *)buffer, *bytes);
		*bytes = input->gcount();
		if (eof_callback()) {
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}
	} catch (...) {
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

::FLAC__StreamDecoderSeekStatus FLACDecoder::seek_callback(FLAC__uint64 absolute_byte_offset) {
	if (error != ERRORCODE_NONE) return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	try {
		if (input->eof() && input->fail()) input->clear();
		input->seekg(absolute_byte_offset, ios_base::beg);
	} catch (...) {
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

::FLAC__StreamDecoderTellStatus FLACDecoder::tell_callback(FLAC__uint64 *absolute_byte_offset) {
	if (error != ERRORCODE_NONE) return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	try {
		*absolute_byte_offset = input->tellg();
	} catch (...) {
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

::FLAC__StreamDecoderLengthStatus FLACDecoder::length_callback(FLAC__uint64 *stream_length) {
	if (error != ERRORCODE_NONE) return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	*stream_length = inputSize;
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool FLACDecoder::eof_callback() {
	return input->tellg() >= inputSize;
}

::FLAC__StreamDecoderWriteStatus FLACDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[]) {
	if (bufferLength == 0) {
		// メタデータにストリーム情報が存在しないフォーマット
		error = ERRORCODE_UNSUPPORTED_FORMAT;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	// コピー
	// 追記できるようにインターリーブで格納する
	// パフォーマンス悪そう
	size_t offset = stream.size();
	stream.resize(offset + frame->header.blocksize * channelNum);
	for (unsigned int i = 0; i < frame->header.blocksize; i++) {
		for (unsigned int c = 0; c < channelNum; c++) {
			stream[offset] = buffer[c][i];
			offset++;
		}
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata) {
	if (initialized) return;

	switch (metadata->type) {
		// ストリーム情報
		case FLAC__METADATA_TYPE_STREAMINFO:
			if (bufferLength != 0) {
				// 既にストリーム情報を入手した後
				// may be needless?
				error = ERRORCODE_INVALID_FORMAT;
				return;
			}
			// 情報格納
			bufferLength = metadata->data.stream_info.total_samples;
			samplingRate = metadata->data.stream_info.sample_rate;
			channelNum = metadata->data.stream_info.channels;
			// バッファタイプ決定
			switch (metadata->data.stream_info.bits_per_sample) {
				case 8:  bufferType = BUFFERTYPE::BUFFERTYPE_INT8;  break;
				case 16: bufferType = BUFFERTYPE::BUFFERTYPE_INT16; break;
				case 24: bufferType = BUFFERTYPE::BUFFERTYPE_INT24; break;
				case 32: bufferType = BUFFERTYPE::BUFFERTYPE_INT32; break;
				// 未サポートのビット数
				// 4bitとか
				default:
					error = ERRORCODE_UNSUPPORTED_FORMAT;
					break;
			}
			break;
	}
}

void FLACDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status) {
	// エラー状態セット
	error = ERRORCODE_UNKNOWN_ERROR;
}

#endif