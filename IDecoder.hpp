#ifndef __IDECODER_HPP
#define __IDECODER_HPP

#include "Types.hpp"
#include "AudioTranscoder.hpp"

struct IDecoder {
	using sample_t = Types::sample_t;
	using byte_t = Types::byte_t;
	using BUFFERTYPE = AudioTranscoder::BUFFERTYPE;

	enum ERRORCODE {
		ERRORCODE_NONE = 0,					// �G���[����
		ERRORCODE_UNKNOWN_ERROR,			// �s���ȃG���[
		ERRORCODE_INTERNAL_ERROR,			// �����G���[
		ERRORCODE_INVALID_ARGUMENT,			// �������s��
		ERRORCODE_INVALID_FORMAT,			// �f�[�^�̃t�H�[�}�b�g���s��
		ERRORCODE_CANNOT_LOAD_STREAM,		// �f�[�^��ǂݍ��߂Ȃ�����
		ERRORCODE_UNSUPPORTED_FORMAT,		// �Ή����Ă��Ȃ��t�H�[�}�b�g
	};

	// �p���N���X�ȊO�����Read-only�ł��肢
	// getter���g�p��������ɂ���Ɠǂ݂Â炢���I�[�o�[�w�b�h���C�ɂȂ�̂�
	int channelNum;					// �`�����l����
	int samplingRate;				// �T���v�����O���[�g
	sample_t bufferLength;			// ���T���v����
	BUFFERTYPE bufferType;			// �f�[�^�̌^

private:
	virtual sample_t _Tell() = 0;								// �o�b�t�@�̈ʒu���擾
	virtual void _Seek(sample_t position) = 0;					// �o�b�t�@�̈ʒu��ݒ�
	virtual sample_t _Read(void *ptr, sample_t length) = 0;		// �o�b�t�@�ǂݎ��

public:
	IDecoder() {
		// �ꉞ0�N���A���Ă���
		channelNum = 0;
		samplingRate = 0;
		bufferLength = 0;
		bufferType = BUFFERTYPE::BUFFERTYPE_FIRST__;
	}

	virtual ~IDecoder() {};


	// �T���v�����o�C�g
	byte_t SampleToByte(sample_t sample) {
		return (byte_t)(sample * (channelNum * SizeOf(bufferType)));
	}

	// �o�C�g���T���v��
	sample_t ByteToSample(byte_t byte) {
		return (sample_t)(byte / (channelNum * SizeOf(bufferType)));
	}


	// �o�b�t�@�̈ʒu���擾
	auto Tell() {
		return _Tell();
	}

	// �o�b�t�@�̈ʒu��ݒ�
	auto Seek(sample_t position) {
		if (position > bufferLength) {
			throw ERRORCODE_INVALID_ARGUMENT;
		}
		_Seek(position);
	}

	// �o�b�t�@�ǂݎ��
	auto Read(void *ptr, sample_t length) {
		return _Read(ptr, length);
	}
};

#endif