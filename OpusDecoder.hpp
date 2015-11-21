#ifndef __OPUSDECODER_HPP
#define __OPUSDECODER_HPP

#include "IDecoder.hpp"
#include <memory>
#include <istream>
#include <opusfile.h>

class OpusDecoder : public IDecoder {
	std::unique_ptr<std::istream> input;
	OggOpusFile *of;

	bool useFloat;									// Float���g�p���邩

	// implements
	sample_t _Tell();								// �o�b�t�@�̈ʒu���擾
	void _Seek(sample_t position);					// �o�b�t�@�̈ʒu��ݒ�
	sample_t _Read(void *ptr, sample_t length);		// �o�b�t�@�ǂݎ��

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