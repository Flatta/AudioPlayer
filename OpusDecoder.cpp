#include "Config.hpp"
#ifdef ENABLE_OPUS

#include "OpusDecoder.hpp"
#include <opus.h>

// Opusの仕様
constexpr int channelNum   = 2;
constexpr int samplingRate = 48000;


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t = IDecoder::byte_t;


// コンストラクタ
OpusDecoder::OpusDecoder(unique_ptr<istream> p_input, bool p_useFloat, bool p_dither) : input(move(p_input)) {
	// メンバ設定
	useFloat = p_useFloat;

	// バッファタイプ決定
	bufferType = useFloat ? BUFFERTYPE::BUFFERTYPE_FLOAT : BUFFERTYPE::BUFFERTYPE_INT16;

	// 例外設定
	input->exceptions(istream::failbit | istream::badbit);

	// Opusを開く
	OpusFileCallbacks cb = {read, seek, tell, close};
	of = op_open_callbacks(this, &cb, nullptr, 0, nullptr);
	if (of == NULL) {
		// 読み込めなかった
		throw ERRORCODE_INVALID_FORMAT;
	}

	// 情報取得・設定
	bufferLength = op_pcm_total(of, -1);
	channelNum = ::channelNum;
	samplingRate = ::samplingRate;

	// デコード設定
	op_set_dither_enabled(of, p_dither ? 1 : 0);
}

// デストラクタ
OpusDecoder::~OpusDecoder() {
	// Opusの後片付け
	op_free(of);
}


// 以下インターフェイス
sample_t OpusDecoder::_Tell() {
	ogg_int64_t position = op_pcm_tell(of);
	if (position < 0) throw position;
	return position;
}

void OpusDecoder::_Seek(sample_t position) {
	int ret = op_pcm_seek(of, position);
	if (ret < 0) throw ret;
}

sample_t OpusDecoder::_Read(void *ptr, sample_t length) {
	if (useFloat) {
		// Float型
		int read = op_read_float_stereo(of, (float *)ptr, length * channelNum);
		if (read < 0) throw read;
		return read;
	} else {
		// 整数型
		int read = op_read_stereo(of, (opus_int16 *)ptr, length * channelNum);
		if (read < 0) throw read;
		return read;
	}
}


// callbacks
// 成功: 読み取ったバイト数, 失敗: 負数
int OpusDecoder::read(void *stream, unsigned char *ptr, int nbytes) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -2;
	try {
		decoder->input->read((char *)ptr, nbytes);
		return decoder->input->gcount();
	} catch (...) {
		return -3;
	}
}

// 成功: 0, 失敗: -1
int OpusDecoder::seek(void *stream, opus_int64 offset, int whence) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -1;
	try {
		ios_base::seekdir way;
		switch (whence) {
			case SEEK_SET: way = ios_base::beg; break;
			case SEEK_CUR: way = ios_base::cur; break;
			case SEEK_END: way = ios_base::end; break;
			default: throw 1;
		}
		if (decoder->input->eof() && decoder->input->fail()) decoder->input->clear();
		decoder->input->seekg(offset, way);
		return 0;
	} catch (...) {
		return -1;
	}
}

// 成功: 位置（バイト単位）, 失敗: 未定義
// （規格には失敗時の返り値は定義されていない）
opus_int64 OpusDecoder::tell(void *stream) {
	if (stream == nullptr) return -1;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return -1;
	try {
		return decoder->input->tellg();
	} catch (...) {
		return -1;
	}
}

// 成功: 0, 失敗: EOF
int OpusDecoder::close(void *stream) {
	if (stream == nullptr) return EOF;
	auto decoder = (OpusDecoder *)stream;
	if (!decoder->input) return EOF;
	try {
		decoder->input.reset();
		return 0;
	} catch (...) {
		return EOF;
	}
}

#endif