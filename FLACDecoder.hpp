#ifndef __FLACDECODER_HPP
#define __FLACDECODER_HPP

#include "IDecoder.hpp"
#include <memory>
#include <vector>
#include <istream>
#include <FLAC++/decoder.h>
#include <share/compat.h>

class FLACDecoder : public IDecoder, public FLAC::Decoder::Stream {
	ERRORCODE error;

	bool oggFLAC;
	bool checkMD5;

	bool initialized;

	std::unique_ptr<std::istream> input;
	FLAC__uint64 inputSize;							// inputのサイズ

	std::vector<FLAC__int32> stream;
	sample_t streamPosition;

	// implements
	sample_t _Tell();								// バッファの位置を取得
	void _Seek(sample_t position);					// バッファの位置を設定
	sample_t _Read(void *ptr, sample_t length);		// バッファ読み取り

	// callbacks
	//  read
	virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
	virtual ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	virtual ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	virtual ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
	virtual bool eof_callback();
	//  write
	virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[]);
	virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	//  other
	virtual void error_callback(::FLAC__StreamDecoderErrorStatus status);

public:
	FLACDecoder(std::unique_ptr<std::istream> p_input, bool p_checkMD5);
};

#endif