#ifndef __WAVDECODER_HPP
#define __WAVDECODER_HPP

#include "IDecoder.hpp"
#include <istream>
#include <memory>

class WavDecoder : public IDecoder {
	std::unique_ptr<std::istream> input;

	byte_t dataOffset;								// �擪����PCM�f�[�^�܂ł̃I�t�Z�b�g
	byte_t dataSize;								// PCM�f�[�^�̃T�C�Y

	// implements
	sample_t _Tell();								// �o�b�t�@�̈ʒu���擾
	void _Seek(sample_t position);					// �o�b�t�@�̈ʒu��ݒ�
	sample_t _Read(void *ptr, sample_t length);		// �o�b�t�@�ǂݎ��

public:
	WavDecoder(std::unique_ptr<std::istream> p_input);
};

#endif