#include "Config.hpp"
#ifdef ENABLE_WAV

#define NOMINMAX

#include "WavDecoder.hpp"
#include <algorithm>
#include <Windows.h>
#include <Mmreg.h>


using namespace std;

using sample_t = IDecoder::sample_t;
using byte_t = IDecoder::byte_t;


// �R���X�g���N�^
WavDecoder::WavDecoder(unique_ptr<istream> p_input) : input(move(p_input)) {
	// ��O�ݒ�
	input->exceptions(istream::failbit | istream::badbit);

	// input�̃T�C�Y�擾
	input->seekg(0, ios::end);
	byte_t size = input->tellg();
	input->seekg(0);

	// �T�C�Y�`�F�b�N
	if (size < 12) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// RIFF�`�����N�AWAVE�`�����N����
	uint32_t audioBufInt[3];
	input->read((char *)audioBufInt, sizeof(audioBufInt));
	if (audioBufInt[0] != (uint32_t)0x46464952/*'RIFF'*/ || audioBufInt[2] != (uint32_t)0x45564157/*'WAVE'*/) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// RIFF�`�����N�̃T�C�Y�`�F�b�N�͖ʓ|�Ȃ̂ŏȗ�
	
	// fmt �`�����N�Adata�`�����N�T��
	dataOffset = 0;
	dataSize = 0;
	byte_t fmtOffset = 0;
	while (true) {
		uint32_t chunkSize;
		uint32_t chunkCode;

		// �w�b�_�T�C�Y�����邩�m�F
		if ((byte_t)input->tellg() + 8 > size) {
			break;
		}

		// �`�����N�̎��ʎq�ƃT�C�Y���擾
		input->read((char *)&chunkCode, sizeof(chunkCode));
		input->read((char *)&chunkSize, sizeof(chunkSize));

		// �`�����N�T�C�Y�����邩�m�F
		if ((byte_t)input->tellg() + chunkSize > size) {
			break;
		}

		// ���ʎq�ŏ����U�蕪��
		switch (chunkCode) {
			// 'fmt '
			case 0x20746D66:
				// �`�����N��PCMWAVEFORMAT���i�[���Ă��邩�m�F
				// WAVEFORMATEX�ɂ��Ή����邽�߃`�����N��PCMWAVEFORMAT���傫���Ă����e����
				if (chunkSize >= sizeof(PCMWAVEFORMAT)) {
					fmtOffset = input->tellg();
				}
				break;

			// 'data'
			case 0x61746164:
				dataOffset = input->tellg();
				dataSize = chunkSize;
				break;
		}

		// �K�v�ȃ`�����N���S�Č��������Ȃ甲����
		if (fmtOffset > 0 && dataOffset > 0) {
			break;
		}

		// �f�[�^��ǂݔ�΂�
		input->seekg(chunkSize, ios::cur);
	}

	// 'fmt '�`�����N��'data'�`�����N�A�܂��͂��̗�����������Ȃ�����
	if (fmtOffset == 0 || dataOffset == 0) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// �t�H�[�}�b�g���R�s�[
	PCMWAVEFORMAT pcmwf;
	input->seekg(fmtOffset);
	input->read((char *)&pcmwf, sizeof(PCMWAVEFORMAT));

	// �o�b�t�@�^�C�v����i���Ή��t�H�[�}�b�g���m�F�j
	if (pcmwf.wf.wFormatTag == WAVE_FORMAT_PCM) {
		// �����^�̃��j�APCM
		if (pcmwf.wBitsPerSample == 32) {
			// 32�r�b�g���j�APCM�i��W���H�j
			bufferType = BUFFERTYPE::BUFFERTYPE_INT32;
		} else if (pcmwf.wBitsPerSample == 24) {
			// 24�r�b�g���j�APCM�i��W���H�j
			bufferType = BUFFERTYPE::BUFFERTYPE_INT24;
		} else if (pcmwf.wBitsPerSample == 16) {
			// 16�r�b�g���j�APCM
			bufferType = BUFFERTYPE::BUFFERTYPE_INT16;
		} else if (pcmwf.wBitsPerSample == 8) {
			// 8�r�b�g���j�APCM
			bufferType = BUFFERTYPE::BUFFERTYPE_INT8U;
		} else {
			// �s���ȃr�b�g��
			throw ERRORCODE_INVALID_FORMAT;
		}
	} else if (pcmwf.wf.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
		// Float�^�̃��j�APCM
		bufferType = BUFFERTYPE::BUFFERTYPE_FLOAT;
	} else {
		// �s���ȃt�H�[�}�b�g
		throw ERRORCODE_UNSUPPORTED_FORMAT;
	}

	// �t�H�[�}�b�g���`�F�b�N
	// ���O�Ōv�Z�������̂��Z�b�g���Ă��܂��Ă��ǂ���������Ȃ�
	if (pcmwf.wf.nAvgBytesPerSec != pcmwf.wf.nBlockAlign * pcmwf.wf.nSamplesPerSec || pcmwf.wf.nBlockAlign != pcmwf.wf.nChannels * (pcmwf.wBitsPerSample / 8)) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// �f�[�^�T�C�Y���u���b�N���E�ɑ����Ă��邩�`�F�b�N
	// �������u���b�N���E�ɑ����Ă��܂��Ă��ǂ���������Ȃ�
	if (dataSize % pcmwf.wf.nBlockAlign != 0) {
		throw ERRORCODE_INVALID_FORMAT;
	}

	// ���ݒ�
	channelNum = pcmwf.wf.nChannels;
	samplingRate = pcmwf.wf.nSamplesPerSec;
	bufferLength = dataSize / pcmwf.wf.nBlockAlign;

	// �ʒu������
	input->seekg(dataOffset);
}


// �ȉ��C���^�[�t�F�C�X
sample_t WavDecoder::_Tell() {
	return ByteToSample((byte_t)input->tellg() - dataOffset);
}

void WavDecoder::_Seek(sample_t position) {
	if (position > bufferLength) throw ERRORCODE_INVALID_ARGUMENT;
	if (input->eof() && input->fail()) input->clear();
	input->seekg(SampleToByte(position) + dataOffset);
}

sample_t WavDecoder::_Read(void *ptr, sample_t length) {
	sample_t readSize = std::min(length, bufferLength - _Tell());
	input->read((char *)ptr, SampleToByte(readSize));
	return readSize;
}

#endif