#include "Config.hpp"
#include "Main.hpp"
#include "AudioPlayer.hpp"
#include "IDecoder.hpp"
#include "FInput.hpp"

#ifdef ENABLE_FLAC
# include "FLACDecoder.hpp"
#endif

#ifdef ENABLE_OPUS
# include "OpusDecoder.hpp"
#endif

#ifdef ENABLE_VORBIS
# include "VorbisDecoder.hpp"
#endif

#ifdef ENABLE_WAV
# include "WavDecoder.hpp"
#endif

#include <vector>
#include <memory>


using namespace std;

using OUTPUTFORMAT = AudioPlayer::OUTPUTFORMAT;
using sample_t = AudioPlayer::sample_t;
using second_t = AudioPlayer::second_t;

using PLAYERCONTAINER = struct PlayerContainer {
	AudioPlayer *player;
	bool autoClose;
	bool pendingClose;
};


vector<PLAYERCONTAINER> players;


// For internal use / �����g�p�̔���J�֐�
///////////////////////////////////////////////////////////////////////////////

// �w�肵��AudioPlayer�����݂��邩���ׂ�
// ���ʂȗ��R���Ȃ�����AudioPlayer�����݂��邩�ǂ�����_APExists�Ŋm�F���邱�ƁipendingClose�̏����̂��߁j
bool _APExists(index_t index) {
	if (index < (index_t)0 || index >= players.size() || !players[index].player) return false;
	// pendingClose�Ȃ炱�̏�ŏ���
	if (players[index].pendingClose) {
		_APForceClose(index);
		return false;
	}
	return true;
}

// AudioPlayer���쐬����players�̋󂫂ɕۑ�
// ��������0�ȏ�̐������A���s���͕�����Ԃ�
index_t _APOpen(unique_ptr<IDecoder> p_decoder, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	unique_ptr<IDecoder> decoder(move(p_decoder));

	// �Ƃ肠����AudioPlayer�쐬�A���s������Ȃ�ł��ǂ��̂ŕ�����Ԃ��悤�ɂ���
	AudioPlayer *player;
	try {
		player = new AudioPlayer(move(decoder), p_openFlag, (OUTPUTFORMAT)p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -2;
	}

	// players�̋󂫂�T���i�Ȃ���Ζ������󂫂Ƃ݂Ȃ��j
	index_t index = players.size();
	for (index_t i = 0; i < players.size(); i++) {
		if (!_APExists(i)) {
			index = i;
			break;
		}
	}

	// PLAYERCONTAINER����
	PLAYERCONTAINER newPlayer = {
		player,
		false,
		false,
	};

	// �󂫂ɐV���������PLAYERCONTAINER������
	if (index == players.size()) {
		players.push_back(newPlayer);
	} else {
		players[index] = newPlayer;
	}

	// �����Đ�����
	if (p_openFlag & OPENFLAG_AUTOCLOSE) {
		APSetAutoCloseFlag(index, true);
	}

	return index;
}

// �w�肵��AudioPlayer�������I�ɍ폜����
// APClose���g���܂ł��Ȃ��Ƃ��Ƃ�
void _APForceClose(index_t index) {
	// �폜�ς݂Ȃ̂ɌĂ΂ꂽ���͗�O�𓊂��Ă݂�
	if (!players[index].player) throw 1;
	// �ȉ��폜����
	delete(players[index].player);
	players[index].player = nullptr;
}


// FLAC / Ogg FLAC
///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_FLAC

// From file (ascii / multibyte filename)
index_t WINAPI APOpenFLACFileA(const char *p_audioFile, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new FLACDecoder(move(stream), p_checkMD5 != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From file (unicode filename)
index_t WINAPI APOpenFLACFileW(const wchar_t *p_audioFile, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new FLACDecoder(move(stream), p_checkMD5 != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From memory
index_t WINAPI APOpenFLACMemory(const char *p_audioBuf, size_t p_audioSize, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::Memory(p_audioBuf, p_audioSize));
		unique_ptr<IDecoder> decoder(new FLACDecoder(move(stream), p_checkMD5 != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

#endif


// Ogg Opus
///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_OPUS

// From file (ascii / multibyte filename)
index_t WINAPI APOpenOpusFileA(const char *p_audioFile, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new OpusDecoder(move(stream), p_useFloat != 0, p_dither != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From file (unicode filename)
index_t WINAPI APOpenOpusFileW(const wchar_t *p_audioFile, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new OpusDecoder(move(stream), p_useFloat != 0, p_dither != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From memory
index_t WINAPI APOpenOpusMemory(const char *p_audioBuf, size_t p_audioSize, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::Memory(p_audioBuf, p_audioSize));
		unique_ptr<IDecoder> decoder(new OpusDecoder(move(stream), p_useFloat != 0, p_dither != 0));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

#endif


// Ogg Vorbis
///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_VORBIS

// From file (ascii / multibyte filename)
index_t WINAPI APOpenVorbisFileA(const char *p_audioFile, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new VorbisDecoder(move(stream), p_bitNum));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From file (unicode filename)
index_t WINAPI APOpenVorbisFileW(const wchar_t *p_audioFile, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new VorbisDecoder(move(stream), p_bitNum));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From memory
index_t WINAPI APOpenVorbisMemory(const char *p_audioBuf, size_t p_audioSize, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::Memory(p_audioBuf, p_audioSize));
		unique_ptr<IDecoder> decoder(new VorbisDecoder(move(stream), p_bitNum));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

#endif


// WAV (Linear PCM)
///////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_WAV

// From file (ascii / multibyte filename)
index_t WINAPI APOpenWavFileA(const char *p_audioFile, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new WavDecoder(move(stream)));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From file (unicode filename)
index_t WINAPI APOpenWavFileW(const wchar_t *p_audioFile, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::File(p_audioFile));
		unique_ptr<IDecoder> decoder(new WavDecoder(move(stream)));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

// From memory
index_t WINAPI APOpenWavMemory(const char *p_audioBuf, size_t p_audioSize, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	try {
		unique_ptr<istream> stream(FInput::Memory(p_audioBuf, p_audioSize));
		unique_ptr<IDecoder> decoder(new WavDecoder(move(stream)));
		return _APOpen(move(decoder), p_openFlag, p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -1;
	}
}

#endif


// Misc. / �G���Ȋ֐��i���players�Ǘ��p�j
///////////////////////////////////////////////////////////////////////////////

// �w�肵��AudioPlayer�����݂��邩���ׂ�
int WINAPI APExists(index_t index) {
	return _APExists(index) ? 1 : 0;
}

// �w�肵��AudioPlayer�����
void WINAPI APClose(index_t index) {
	if (!_APExists(index)) return;
	_APForceClose(index);
}

// �S�Ă�AudioPlayer�����
void WINAPI APFree() {
	for (index_t i = 0; i < players.size(); i++) {
		if (!players[i].player) {
			continue;
		}
		_APForceClose(i);
	}
}

// pendingClose��AudioPlayer��S�ĕ���
// �ʏ�ĂԕK�v�͂Ȃ�
void WINAPI APClear() {
	for (index_t i = 0; i < players.size(); i++) {
		if (!players[i].player) {
			continue;
		}
		if (players[i].pendingClose) {
			_APForceClose(i);
		}
	}
}


// Operater / ����֐�
///////////////////////////////////////////////////////////////////////////////

// �Đ��I���t���O�擾
int WINAPI APGetFinishFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetFinishFlag() ? 1: 0;
}

// �Đ��ʒu�擾�i�T���v���P�ʁj
int WINAPI APGetPosition(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetPosition();
}

// �Đ��ʒu�擾�i�b�P�ʁj
void WINAPI APGetPositionS(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetPositionS();
}

// �Đ��ʒu�ݒ�i�T���v���P�ʁj
void WINAPI APSetPosition(index_t index, int p_position) {
	if (!_APExists(index)) return;
	try {
		players[index].player->SetPosition((sample_t)p_position);
	} catch (...) {
		// ����Ԃ����ق��Ȃ�
	}
}

// �Đ��ʒu�ݒ�i�b�P�ʁj
void WINAPI APSetPositionS(index_t index, double p_position) {
	if (!_APExists(index)) return;
	try {
		players[index].player->SetPosition((second_t)p_position);
	} catch (...) {
		// ����Ԃ����ق��Ȃ�
	}
}

// �����擾�i�T���v���P�ʁj
int WINAPI APGetLength(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetLength();
}

// �����擾�i�b�P�ʁj
void WINAPI APGetLengthS(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetLengthS();
}

// �Đ�
void WINAPI APPlay(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Play();
}

// ��~
void WINAPI APStop(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Stop();
}

// ��~���ĕ���
void WINAPI APStopClose(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Stop();
	APClose(index);
}

// �čĐ�
void WINAPI APReplay(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Replay();
}

// �{�����[���擾
void WINAPI APGetVolume(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetVolume();
}

// �{�����[���ݒ�
void WINAPI APSetVolume(index_t index, double p_volume) {
	if (!_APExists(index)) return;
	players[index].player->SetVolume(p_volume);
}

// �|�[�Y�t���O�擾
int WINAPI APGetPauseFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetPauseFlag() ? 1 : 0;
}

// �|�[�Y�t���O�ݒ�
void WINAPI APSetPauseFlag(index_t index, int p_pauseFlag) {
	if (!_APExists(index)) return;
	players[index].player->SetPauseFlag(p_pauseFlag != 0 ? true : false);
}

// ���[�v�t���O�擾
int WINAPI APGetLoopFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetLoopFlag();
}

// ���[�v�t���O�ݒ�
void WINAPI APSetLoopFlag(index_t index, int p_loopFlag) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopFlag(p_loopFlag);
}

// ���[�v�ʒu�擾�i�T���v���P�ʁj
void WINAPI APGetLoopPoint(index_t index, int *p_beginLoop, int *p_loopLength) {
	if (!_APExists(index)) return;
	AudioPlayer::sample_t beginLoop, loopLength;
	players[index].player->GetLoopPoint(&beginLoop, &loopLength);
	if (p_beginLoop != nullptr) *p_beginLoop = beginLoop;
	if (p_loopLength != nullptr) *p_loopLength = loopLength;
}

// ���[�v�ʒu�擾�i�b�P�ʁj
void WINAPI APGetLoopPointS(index_t index, double *p_beginLoop, double *p_loopLength) {
	if (!_APExists(index)) return;
	AudioPlayer::second_t beginLoop, loopLength;
	players[index].player->GetLoopPoint(&beginLoop, &loopLength);
	if (p_beginLoop != nullptr) *p_beginLoop = beginLoop;
	if (p_loopLength != nullptr) *p_loopLength = loopLength;
}

// ���[�v�ʒu�ݒ�i�T���v���P�ʁj
void WINAPI APSetLoopPoint(index_t index, int p_beginLoop, int p_loopLength) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopPoint((sample_t)p_beginLoop, (sample_t)p_loopLength);
}

// ���[�v�ʒu�ݒ�i�b�P�ʁj
void WINAPI APSetLoopPointS(index_t index, double p_beginLoop, double p_loopLength) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopPoint((second_t)p_beginLoop, (second_t)p_loopLength);
}

// ��������t���O�擾
int WINAPI APGetAutoCloseFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].autoClose ? 1 : 0;
}

// ��������t���O�ݒ�
void WINAPI APSetAutoCloseFlag(index_t index, bool flag) {
	if (!_APExists(index)) return;
	if (flag == players[index].autoClose) return;

	players[index].autoClose = flag;

	if (flag) {
		players[index].player->SetFinishCallback([index](AudioPlayer *player) {
			// �폜�\������Ă���
			// �����ō폜����Ǝ��ʂ��߂����ł͍폜�ł��Ȃ��i���̊֐���player�̃����o����Ăяo����Ă���j
			// ���ۂɍ폜�����̂�
			//     �E���ɂ���player�����삳�ꂻ���ɂȂ�����
			//     �E�����_APOpen�ł���player�ȑO�ɋ󂫂����݂��Ȃ�������
			//     �E����APClear���Ă΂ꂽ��
			// �̂����ꂩ�ɂȂ�
			players[index].pendingClose = true;
			return 1;
		});
	} else {
		players[index].player->ClearFinishCallback();
	}
}