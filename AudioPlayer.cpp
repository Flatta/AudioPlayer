#define NOMINMAX

#include "AudioPlayer.hpp"
#include <algorithm>
#include <Mmreg.h>


using namespace std;
using namespace AudioTranscoder;


// �l�����͈̔͂Ɏ��߂�֐�
template <typename T>
inline T clamp(T value, T minValue, T maxValue) {
	return min(max(value, minValue), maxValue);
}


// �R�[���o�b�N�֐�
// �ǂ���烁���o�֐��̃|�C���^�͎擾�ł��Ȃ��悤�Ȃ̂ŁA������dwInstance��this�̃|�C���^��n���Ă��
inline void CALLBACK g_waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	auto player = (AudioPlayer*)dwInstance;
	player->waveOutProc(hwo, uMsg, dwInstance, dwParam1, dwParam2);
}


// �ϊ��֐��i�����p�j
// �~���b��o�C�g�Ƃ̕ϊ��֐��͂Ƃ�������2�͌��J�����ق����ǂ���������Ȃ�
inline AudioPlayer::second_t AudioPlayer::SampleToSecond(sample_t sample) {
	return (second_t)sample / decoder->samplingRate;
}

inline AudioPlayer::sample_t AudioPlayer::SecondToSample(second_t second) {
	if (second < 0) throw ERRORCODE_INVALID_ARGUMENT;
	return (sample_t)(second * decoder->samplingRate);
}

inline AudioPlayer::sample_t AudioPlayer::MillisecondToSample(msec_t millisecond) {
	return (sample_t)millisecond * decoder->samplingRate / 1000;
}

inline AudioPlayer::byte_t AudioPlayer::SampleToOutputByte(sample_t sample) {
	return (byte_t)sample * (decoder->channelNum * SizeOf(outputBufferType));
}

inline AudioPlayer::sample_t AudioPlayer::OutputByteToSample(byte_t byte) {
	return byte / (decoder->channelNum * SizeOf(outputBufferType));
}


// �o�b�t�@�擾
inline char* AudioPlayer::GetBuffer(int index) {
	return &outputBuffer[index * SampleToOutputByte(bufSize)];
}


// WAVEHDR����
void AudioPlayer::WriteBuffer(WAVEHDR *curwhdr) {
	// �I�����Ă�������������s��Ȃ�
	if (finishWriteFlag) {
		curwhdr->dwBufferLength = 0;
		curwhdr->dwUser = 0;
		return;
	}

	// �t���O������
	int flag = 0;

	// ��������
	sample_t position = 0;
	for (;;) {
		sample_t tempSize;
		
		// �o�b�t�@�c��T�C�Y�v�Z
		tempSize = bufSize - position;
		if (tempSize <= 0) {
			// �����f�R�[�h���Ȃ��Ă��ǂ�
			break;
		}

		// ���[�v�̖��[�����ׂ�
		bool isEndLoopBlock = false;
		sample_t curPos;
		if (loopFlag & LOOPFLAG_SECTION) {
			curPos = decoder->Tell();
			isEndLoopBlock = endLoopPoint - tempSize <= curPos && curPos < endLoopPoint;
		}

		// �Z�N�V�������f�R�[�h
		sample_t read;
		sample_t readSize = isEndLoopBlock ? endLoopPoint - curPos : tempSize;
		try {
			read = decoder->Read(tempInputBuffer.get() + decoder->SampleToByte(position), readSize);
			if (read == 0) {
				// End of File
				// �����f�R�[�h�ł��Ȃ�
				if (loopFlag & LOOPFLAG_FILE) {
					// ���[�v����
					decoder->Seek(0);
					flag |= BLOCKFLAG_LOOP_FILE;
				} else {
					// �I������
					finishWriteFlag = true;
					flag |= BLOCKFLAG_LAST;
					break;
				}
			}
		} catch (...) {
			// �G���[
			// �Ƃ肠����0�o�C�g�ǂݍ��񂾂Ƃ������Ƃɂ��Ă���
			read = 0;
			flag |= BLOCKFLAG_ERROR;
		}

		// �������݈ʒu�C��
		position += read;

		// ���[�v�p�V�[�N
		if (isEndLoopBlock && decoder->Tell() >= endLoopPoint) {
			decoder->Seek(beginLoopPoint);
			curwhdr->dwUser |= BLOCKFLAG_LOOP_SECTION;
		}
	};

	// �ϊ�
	// TODO: Fade
	Transcode(tempInputBuffer.get(), decoder->bufferType, curwhdr->lpData, outputBufferType, position * decoder->channelNum, volume);

	// ��񏑂�����
	curwhdr->dwBufferLength = SampleToOutputByte(position);
	curwhdr->dwUser = flag;

	return;
}


// �R�[���o�b�N����Ă΂��֐�
// �R�[���o�b�N�̎��̂̂悤�Ȃ���
void AudioPlayer::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	// �I����Ă�Ȃ牽�����Ȃ�
	// �������Ȃ��ƃf�X�g���N�^���Ă񂾎��Ƀt���[�Y����͗l
	if (finishPlayFlag) return;

	// �o�̓o�b�t�@�Đ��I��
	if (uMsg == MM_WOM_DONE) {
		// ���̏o�̓o�b�t�@�̃C���f�b�N�X�擾
		int nextIndex = currentIndex + 1;
		if (nextIndex >= bufNum) nextIndex = 0;

		// �t���O�擾
		int flag = whdr[currentIndex].dwUser;

		// �ŏI�u���b�N
		if (flag & BLOCKFLAG_LAST) {
			finishPlayFlag = true;
			if (finishCallback != nullptr) {
				finishCallback(this);
			}
		}

		// ���[�v���������ꂽ�u���b�N
		if (flag & (BLOCKFLAG_LOOP_FILE | BLOCKFLAG_LOOP_SECTION)) {
			if (loopCallback != nullptr) {
				loopCallback(this, (flag & BLOCKFLAG_LOOP_FILE) != 0);
			}
		}

		// �������C��
		lastPlayedTime = timeGetTime();
		lastSectionPosition = sectionPosition[nextIndex];

		sectionPosition[currentIndex] = decoder->Tell();

		// �o�̓o�b�t�@�ɏ�������
		WriteBuffer(&whdr[currentIndex]);
		waveOutWrite(hwo, &whdr[currentIndex], sizeof(WAVEHDR));

		// �o�̓o�b�t�@�̃C���f�b�N�X�X�V
		currentIndex = nextIndex;
	}
}


// �R���X�g���N�^
AudioPlayer::AudioPlayer(unique_ptr<IDecoder> p_decoder, int p_openFlag, OUTPUTFORMAT p_outputFormat, sample_t p_bufSize, unsigned int p_bufNum) : decoder(move(p_decoder)) {
	// �l�`�F�b�N
	if (p_bufSize <= 0 || p_bufNum <= 0) {
		throw ERRORCODE_INVALID_ARGUMENT;
	}

	// �l�R�s�[
	openFlag = p_openFlag;
	outputFormat = p_outputFormat;
	bufNum = max(p_bufNum, 1u);
	bufSize = p_bufSize;

	// �o�̓o�b�t�@�̌`���ݒ�
	switch (outputFormat) {
		case OUTPUTFORMAT_UINT8:
			outputBufferType = BUFFERTYPE_INT8U;
			break;

		case OUTPUTFORMAT_INT16:
			outputBufferType = BUFFERTYPE_INT16;
			break;

		case OUTPUTFORMAT_INT24:
			outputBufferType = BUFFERTYPE_INT24;
			break;

		case OUTPUTFORMAT_INT32:
			outputBufferType = BUFFERTYPE_INT32;
			break;

		case OUTPUTFORMAT_FLOAT:
			outputBufferType = BUFFERTYPE_FLOAT;
			break;

		default:
			// BUFFERTYPE_FLOAT���g���Ă��ǂ���������Ȃ�
			throw ERRORCODE_INVALID_ARGUMENT;
	}

	// ���낢�돉����
	hwo = nullptr;
	currentIndex = 0;
	volume = 1.0;

	// �t���O������
	stopFlag = true;
	finishWriteFlag = true;
	finishPlayFlag = true;
	pauseFlag = false;
	loopFlag = 0;

	// ���낢�돀���Ə�����
	sectionPosition = make_unique<sample_t[]>(bufNum);
	lastPlayedTime = timeGetTime();
	lastSectionPosition = 0;

	// �R�[���o�b�N������
	finishCallback = nullptr;
	loopCallback = nullptr;
	fadeCallback = nullptr;


	// �ȉ�decoder�����ɏ���

	// �l�`�F�b�N
	if (!IsValid(decoder->bufferType) || decoder->channelNum <= 0 || decoder->samplingRate <= 0) {
		throw ERRORCODE_INTERNAL_ERROR;
	}

	// _ReadBuffer�p�ꎞ�o�b�t�@�쐬
	tempInputBuffer = make_unique<char[]>(decoder->SampleToByte(bufSize));


	// �r�b�g���ƃu���b�N���E�v�Z
	unsigned int bitNum = BitSizeOf(outputBufferType);
	unsigned int blockAlign = decoder->channelNum * SizeOf(outputBufferType);

	// wfx����
	WAVEFORMATEX wfx = {
		(WORD)(outputFormat == OUTPUTFORMAT_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM),
		(WORD)decoder->channelNum,
		(DWORD)decoder->samplingRate,
		(DWORD)decoder->samplingRate * blockAlign,
		(WORD)blockAlign,
		(WORD)bitNum,
		(WORD)0
	};

	// MME���J��
	auto ret = waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD_PTR)&g_waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if (ret != MMSYSERR_NOERROR) {
		// �f�X�g���N�^�Ŗ�肪��������悤�Ȃ� hwo = nullptr �Ə����Ă݂悤
		throw ERRORCODE_CANNOT_OPEN_DEVICE;
	}

	// PCM�o�b�t�@����
	outputBuffer = make_unique<char[]>(SampleToOutputByte(bufSize) * bufNum);

	// �Đ��o�b�t�@����
	whdr = make_unique<WAVEHDR[]>(bufNum);
	for (unsigned int i = 0; i < bufNum; i++) {
		char *buffer = GetBuffer(i);

		whdr[i] = {
			buffer,
			(DWORD)SampleToOutputByte(bufSize),
			NULL,
			NULL,
			0,
			1,
			NULL,
			NULL
		};

		waveOutPrepareHeader(hwo, &whdr[i], sizeof(WAVEHDR));
	}

	// �����Đ�����
	if (openFlag & OPENFLAG_AUTOSTART) {
		Play();
	}
}

// �f�X�g���N�^
AudioPlayer::~AudioPlayer() {
	// �����I�ɏI���������Ƃɂ���
	stopFlag = true;
	finishPlayFlag = true;
	finishWriteFlag = true;

	// MME�̌�Еt��
	if (hwo != nullptr) {
		waveOutReset(hwo);
		for (unsigned int i = 0; i < bufNum; i++) {
			waveOutUnprepareHeader(hwo, &whdr[i], sizeof(WAVEHDR));
		}
		waveOutClose(hwo);
	}
}


// �ʒu�Ⓑ���̎擾�Ɛݒ�
void AudioPlayer::SetPosition(sample_t position) {
	decoder->Seek(position);
}

void AudioPlayer::SetPosition(second_t position) {
	SetPosition(SecondToSample(position));
}

AudioPlayer::sample_t AudioPlayer::GetPosition() {;
	if (stopFlag) return 0;
	if (finishPlayFlag) return GetLength();
	if (pauseFlag) return pausedTime;

	/*
	// �Ȃ񂩃t���[�Y����
	// ���m�ɂ̓����_����waveOutGetPosition����A���Ă��Ȃ��Ȃ�
	// �Ăԕp�x����������
	MMTIME mmt;
	mmt.wType = TIME_SAMPLES;
	waveOutGetPosition(hwo, &mmt, sizeof(MMTIME));

	switch (mmt.wType) {
		case TIME_SAMPLES:
			return mmt.u.sample;

		case TIME_MS:
			return MillisecondToSample(mmt.u.ms);

		case TIME_BYTES:
			return OutputByteToSample(mmt.u.cb);
	}
	//*/

	return lastSectionPosition + MillisecondToSample(timeGetTime() - lastPlayedTime);
}

AudioPlayer::second_t AudioPlayer::GetPositionS() {
	if (stopFlag) return 0.0;
	if (finishPlayFlag) return GetLengthS();
	// �蔲��
	return SampleToSecond(GetPosition());
}

AudioPlayer::sample_t AudioPlayer::GetLength() {
	return decoder->bufferLength;
}

AudioPlayer::second_t AudioPlayer::GetLengthS() {
	return SampleToSecond(GetLength());
}


// �Đ��ƒ�~
void AudioPlayer::Play() {
	if (!finishPlayFlag) return;

	// �e��t���O���Z�b�g
	stopFlag = false;
	finishWriteFlag = false;
	finishPlayFlag = false;

	// �o�b�t�@���ŏ��̏ꏊ�ɖ߂�
	decoder->Seek(0);
	currentIndex = 0;

	// �Đ����Ԍv���p�ϐ�������
	lastPlayedTime = timeGetTime();
	lastSectionPosition = 0;

	// �o�b�t�@�ǉ�
	for (unsigned int i = 0; i < bufNum; i++) {
		if (finishWriteFlag) break;

		// �I�����Ă��Ȃ���΍Đ��o�b�t�@�ɏ�������
		sectionPosition[i] = decoder->Tell();
		WriteBuffer(&whdr[i]);
		waveOutWrite(hwo, &whdr[i], sizeof(WAVEHDR));

		// ����̃o�b�t�@�̍Đ������𐳊m�ɂ��邽�߂ɂ����ł��X�V���Ă���
		if (i == 0) {
			lastPlayedTime = timeGetTime();
		}
	}
}

void AudioPlayer::Stop() {
	if (finishPlayFlag) return;

	// �e��t���O���Z�b�g
	stopFlag = true;
	finishWriteFlag = true;
	finishPlayFlag = true;
	pauseFlag = false;

	// �o�b�t�@�����Z�b�g
	waveOutReset(hwo);
}

void AudioPlayer::Replay() {
	Stop();
	Play();
}


// �t���O�֘A
bool AudioPlayer::GetFinishFlag() {
	return finishPlayFlag;
}

bool AudioPlayer::GetPauseFlag() {
	return pauseFlag;
}

void AudioPlayer::SetPauseFlag(bool p_pauseFlag) {
	if (finishPlayFlag || p_pauseFlag == pauseFlag) return;
	if (p_pauseFlag) {
		// �ꎞ��~
		pausedTime = GetPosition();
		pausedLastPlayedTime = lastPlayedTime - timeGetTime();
		waveOutPause(hwo);
	} else {
		// �ꎞ��~����
		waveOutRestart(hwo);
		lastPlayedTime = pausedLastPlayedTime + timeGetTime();
	}
	pauseFlag = p_pauseFlag;
}

int AudioPlayer::GetLoopFlag() {
	return loopFlag;
}

void AudioPlayer::SetLoopFlag(int p_loopFlag) {
	loopFlag = p_loopFlag;
}


// ���[�v�|�C���g
void AudioPlayer::GetLoopPoint(sample_t *p_beginLoop, sample_t *p_loopLength) {
	if (p_beginLoop != nullptr) *p_beginLoop = beginLoopPoint;
	if (p_loopLength != nullptr) *p_loopLength = endLoopPoint - beginLoopPoint;
}

void AudioPlayer::GetLoopPoint(second_t *p_beginLoop, second_t *p_loopLength) {
	if (p_beginLoop != nullptr) *p_beginLoop = SampleToSecond(beginLoopPoint);
	if (p_loopLength != nullptr) *p_loopLength = SampleToSecond(endLoopPoint - beginLoopPoint);
}

void AudioPlayer::SetLoopPoint(sample_t p_beginLoop, sample_t p_loopLength) {
	beginLoopPoint = p_beginLoop;
	endLoopPoint = beginLoopPoint + p_loopLength;
}

void AudioPlayer::SetLoopPoint(second_t p_beginLoop, second_t p_loopLength) {
	SetLoopPoint(SecondToSample(p_beginLoop), SecondToSample(p_loopLength));
}


// �{�����[��
double AudioPlayer::GetVolume() {
	return volume;
}

void AudioPlayer::SetVolume(double p_volume) {
	volume = p_volume;
}


// �R�[���o�b�N�̐ݒ�Ɖ���
void AudioPlayer::SetFinishCallback(const function<void(AudioPlayer *)> &callback) {
	finishCallback = callback;
}

void AudioPlayer::ClearFinishCallback() {
	finishCallback = nullptr;
}

void AudioPlayer::SetLoopCallback(const function<void(AudioPlayer *, bool)> &callback) {
	loopCallback = callback;
}

void AudioPlayer::ClearLoopCallback() {
	loopCallback = nullptr;
}

void AudioPlayer::SetFadeCallback(const function<void(AudioPlayer *)> &callback) {
	fadeCallback = callback;
}

void AudioPlayer::ClearFadeCallback() {
	fadeCallback = nullptr;
}