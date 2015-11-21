#ifndef __VORBISDECODER_HPP
#define __VORBISDECODER_HPP

#include "IDecoder.hpp"
#include <memory>
#include <istream>
#include <vorbis/vorbisfile.h>

class VorbisDecoder : public IDecoder {
	std::unique_ptr<std::istream> input;
	OggVorbis_File vf;

	// implements
	sample_t _Tell();								// バッファの位置を取得
	void _Seek(sample_t position);					// バッファの位置を設定
	sample_t _Read(void *ptr, sample_t length);		// バッファ読み取り

	// callbacks
	static int close(void *datasource);
	static size_t read(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int seek(void *datasource, ogg_int64_t offset, int whence);
	static long tell(void *datasource);

public:
	VorbisDecoder(std::unique_ptr<std::istream> p_input, int p_bitNum);
	~VorbisDecoder();
};

#endif