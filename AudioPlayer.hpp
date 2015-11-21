#ifndef __AUDIOPLAYER_HPP
#define __AUDIOPLAYER_HPP

#include "Types.hpp"
#include "IDecoder.hpp"
#include "AudioTranscoder.hpp"
#include <Windows.h>
#include <memory>
#include <functional>


struct AudioPlayer {
	using second_t = Types::second_t;
	using msec_t = Types::msec_t;
	using sample_t = Types::sample_t;
	using byte_t = Types::byte_t;

	const enum ERRORCODE {
		ERRORCODE_NONE = 0,					// �G���[����
		ERRORCODE_UNKNOWN_ERROR,			// �s���ȃG���[
		ERRORCODE_INTERNAL_ERROR,			// �����G���[
		ERRORCODE_INVALID_ARGUMENT,			// �������s��
		ERRORCODE_CANNOT_OPEN_DEVICE,		// �f�o�C�X���J���Ȃ�����
	};

	const enum OUTPUTFORMAT {
		OUTPUTFORMAT_UINT8 = 0,				//  8bit unsigned int
		OUTPUTFORMAT_INT16,					// 16bit   signed int
		OUTPUTFORMAT_FLOAT,					// 32bit   signed float
	};

	static constexpr unsigned int OPENFLAG_AUTOSTART      = 0x00000001;		// �����Đ�
	
	static constexpr unsigned int LOOPFLAG_FILE           = 0x00000001;		// �t�@�C���S�̂����[�v
	static constexpr unsigned int LOOPFLAG_SECTION        = 0x00000002;		// �w�肵����Ԃ����[�v
	
	static constexpr unsigned int BLOCKFLAG_ERROR         = 0x00000001;		// �G���[����������
	static constexpr unsigned int BLOCKFLAG_LAST          = 0x00000002;		// �Ō�
	static constexpr unsigned int BLOCKFLAG_LOOP_FILE     = 0x00000004;		// ���[�v�����i�t�@�C���j����������
	static constexpr unsigned int BLOCKFLAG_LOOP_SECTION  = 0x00000008;		// ���[�v�����i��ԁj����������
	static constexpr unsigned int BLOCKFLAG_FADE_BEGIN    = 0x00000010;		// �t�F�[�h�J�n��������������
	static constexpr unsigned int BLOCKFLAG_FADE_END      = 0x00000020;		// �t�F�[�h�I����������������

private:
	unsigned int bufNum;											// �}���`�o�b�t�@�����O�p�o�̓o�b�t�@���A1�ȏ�i2��3�����j
	sample_t bufSize;												// �e�o�b�t�@�̃T���v�����A1�ȏ�i�T���v�����O���[�g44100Hz�A�o�b�t�@��2�܂���3�̏ꍇ��2048��4096������𐄏��j

	std::unique_ptr<char[]> tempInputBuffer;						// _ReadBuffer�Ƀf�[�^�����Ă��炤�ꎞ�ϐ�

	OUTPUTFORMAT outputFormat;										// �o�̓t�H�[�}�b�g
	AudioTranscoder::BUFFERTYPE outputBufferType;					// �o�̓t�H�[�}�b�g�̃f�[�^�̌^

	HWAVEOUT hwo;
	std::unique_ptr<WAVEHDR[]> whdr;

	unsigned int currentIndex;										// ���݂̏o�̓o�b�t�@�̃C���f�b�N�X
	std::unique_ptr<char[]> outputBuffer;							// �o�̓o�b�t�@
	std::unique_ptr<sample_t[]> sectionPosition;					// �o�̓o�b�t�@�̐�Έʒu
	msec_t lastPlayedTime;											// �Ō�ɏo�̓o�b�t�@���Đ����I���������
	sample_t lastSectionPosition;									// �Ō�̏o�̓o�b�t�@�̈ʒu

	double volume;													// �{�����[��

	int openFlag;													// ����t���O

	bool pauseFlag;													// �ꎞ��~�t���O
	sample_t pausedTime;											// �ꎞ��~���̍Đ��ʒu
	msec_t pausedLastPlayedTime;									// �ꎞ��~����lastTime��timeGetTime()�̍���

	int loopFlag;													// ���[�v�Đ��t���O
	sample_t beginLoopPoint;										// ���[�v�J�n�n�_
	sample_t endLoopPoint;											// ���[�v�I���n�_

	bool stopFlag;													// ��~���t���O�A���GetPosition�p

	bool finishWriteFlag;											// �Đ��o�b�t�@�������ݏI���t���O
	bool finishPlayFlag;											// �Đ��I���t���O

	std::function<void(AudioPlayer *)> finishCallback;				// �Đ��I�����ɌĂ΂��R�[���o�b�N
	std::function<void(AudioPlayer *, bool)> loopCallback;			// ���[�v���ɌĂ΂��R�[���o�b�N�A�ő�Ńo�b�t�@�T�C�Y�������Ă΂��^�C�~���O�������
	std::function<void(AudioPlayer *)> fadeCallback;				// �t�F�[�h�I�����ɌĂ΂��R�[���o�b�N�A�����������

	second_t SampleToSecond(sample_t sample);						// �T���v�����b
	sample_t SecondToSample(second_t second);						// �b���T���v��
	sample_t MillisecondToSample(msec_t millisecond);				// �~���b���T���v��
	byte_t SampleToOutputByte(sample_t sample);						// �T���v�����o�̓f�[�^�̃o�C�g
	sample_t OutputByteToSample(byte_t byte);						// �o�̓f�[�^�̃o�C�g���T���v��
	void WriteBuffer(WAVEHDR *curwhdr);								// �f�[�^��ǂݎ����curwhdr����������

	char* GetBuffer(int index);										// �o�b�t�@�擾

	std::unique_ptr<IDecoder> decoder;								// �f�R�[�_�[

public:
	AudioPlayer(std::unique_ptr<IDecoder> p_decoder, int p_openFlag, OUTPUTFORMAT p_outputFormat, sample_t p_bufSize, unsigned int p_bufNum);
	~AudioPlayer();

	void waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	sample_t GetPosition();																// ���݂̍Đ��ʒu���擾�i�T���v���j
	second_t GetPositionS();															// ���݂̍Đ��ʒu���擾�i�b�j
	void SetPosition(sample_t position);												// �Đ��ʒu��ݒ�i�T���v���j
	void SetPosition(second_t position);												// �Đ��ʒu��ݒ�i�b�j
	sample_t GetLength();																// ���Đ����Ԃ��擾�i�T���v���j
	second_t GetLengthS();																// ���Đ����Ԃ��擾�i�b�j
	void Play();																		// �Đ�����
	void Stop();																		// ��~����
	void Replay();																		// ��~���čĐ�����
	bool GetFinishFlag();																// �I���t���O�擾
	bool GetPauseFlag();																// �ꎞ��~�t���O�擾
	void SetPauseFlag(bool p_pauseFlag);												// �ꎞ��~�܂��͍ĊJ����
	int GetLoopFlag();																	// ���[�v�Đ��t���O�擾
	void SetLoopFlag(int p_loopFlag);													// ���[�v�Đ��t���O�ݒ�
	void GetLoopPoint(sample_t *p_beginLoop, sample_t *p_loopLength);					// ���[�v��Ԏ擾�i�T���v���j
	void GetLoopPoint(second_t *p_beginLoop, second_t *p_loopLength);					// ���[�v��Ԏ擾�i�b�j
	void SetLoopPoint(sample_t p_beginLoop, sample_t p_loopLength);						// ���[�v��Ԑݒ�i�T���v���j
	void SetLoopPoint(second_t p_beginLoop, second_t p_loopLength);						// ���[�v��Ԑݒ�i�b�j
	double GetVolume();																	// �{�����[���擾
	void SetVolume(double p_volume);													// �{�����[���ݒ�
	void Fade(sample_t duration, double to, bool relative);								// �t�F�[�h
	void FadeIn(sample_t duration);														// �t�F�[�h�C��
	void FadeOut(sample_t duration);													// �t�F�[�h�A�E�g
	void StopFade(bool finish);															// �t�F�[�h������
	void SetFinishCallback(const std::function<void(AudioPlayer *)> &callback);			// �Đ��I�����̃R�[���o�b�N��ݒ�
	void ClearFinishCallback();															// �Đ��I�����̃R�[���o�b�N������
	void SetLoopCallback(const std::function<void(AudioPlayer *, bool)> &callback);		// ���[�v���̃R�[���o�b�N��ݒ�
	void ClearLoopCallback();															// ���[�v���̃R�[���o�b�N������
	void SetFadeCallback(const std::function<void(AudioPlayer *)> &callback);			// �t�F�[�h�I�����̃R�[���o�b�N��ݒ�
	void ClearFadeCallback();															// �t�F�[�h�I�����̃R�[���o�b�N������
};

#endif