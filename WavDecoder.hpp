#ifndef __WAVDECODER_HPP
#define __WAVDECODER_HPP

#include "IDecoder.hpp"
#include <istream>
#include <memory>

class WavDecoder : public IDecoder {
	std::unique_ptr<std::istream> input;

	byte_t dataOffset;								// 先頭からPCMデータまでのオフセット
	byte_t dataSize;								// PCMデータのサイズ

	// implements
	sample_t _Tell();								// バッファの位置を取得
	void _Seek(sample_t position);					// バッファの位置を設定
	sample_t _Read(void *ptr, sample_t length);		// バッファ読み取り

public:
	WavDecoder(std::unique_ptr<std::istream> p_input);
};

#endif