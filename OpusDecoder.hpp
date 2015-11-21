#ifndef __OPUSDECODER_HPP
#define __OPUSDECODER_HPP

#include "IDecoder.hpp"
#include <memory>
#include <istream>
#include <opusfile.h>

class OpusDecoder : public IDecoder {
	std::unique_ptr<std::istream> input;
	OggOpusFile *of;

	bool useFloat;									// Floatを使用するか

	// implements
	sample_t _Tell();								// バッファの位置を取得
	void _Seek(sample_t position);					// バッファの位置を設定
	sample_t _Read(void *ptr, sample_t length);		// バッファ読み取り

	// callbacks
	static int read(void *stream, unsigned char *ptr, int nbytes);
	static int seek(void *stream, opus_int64 offset, int whence);
	static opus_int64 tell(void *stream);
	static int close(void *stream);

public:
	OpusDecoder(std::unique_ptr<std::istream> p_input, bool p_useFloat, bool p_dither);
	~OpusDecoder();
};

#endif