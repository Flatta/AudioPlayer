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


// For internal use / 内部使用の非公開関数
///////////////////////////////////////////////////////////////////////////////

// 指定したAudioPlayerが存在するか調べる
// 特別な理由がない限りAudioPlayerが存在するかどうかは_APExistsで確認すること（pendingCloseの処理のため）
bool _APExists(index_t index) {
	if (index < (index_t)0 || index >= players.size() || !players[index].player) return false;
	// pendingCloseならこの場で消す
	if (players[index].pendingClose) {
		_APForceClose(index);
		return false;
	}
	return true;
}

// AudioPlayerを作成してplayersの空きに保存
// 成功時は0以上の整数を、失敗時は負数を返す
index_t _APOpen(unique_ptr<IDecoder> p_decoder, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) {
	unique_ptr<IDecoder> decoder(move(p_decoder));

	// とりあえずAudioPlayer作成、失敗したらなんでも良いので負数を返すようにする
	AudioPlayer *player;
	try {
		player = new AudioPlayer(move(decoder), p_openFlag, (OUTPUTFORMAT)p_outputFormat, p_bufSize, p_bufNum);
	} catch (...) {
		return -2;
	}

	// playersの空きを探す（なければ末尾を空きとみなす）
	index_t index = players.size();
	for (index_t i = 0; i < players.size(); i++) {
		if (!_APExists(i)) {
			index = i;
			break;
		}
	}

	// PLAYERCONTAINER準備
	PLAYERCONTAINER newPlayer = {
		player,
		false,
		false,
	};

	// 空きに新しく作ったPLAYERCONTAINERを入れる
	if (index == players.size()) {
		players.push_back(newPlayer);
	} else {
		players[index] = newPlayer;
	}

	// 自動再生処理
	if (p_openFlag & OPENFLAG_AUTOCLOSE) {
		APSetAutoCloseFlag(index, true);
	}

	return index;
}

// 指定したAudioPlayerを強制的に削除する
// APCloseを使うまでもないときとか
void _APForceClose(index_t index) {
	// 削除済みなのに呼ばれた時は例外を投げてみる
	if (!players[index].player) throw 1;
	// 以下削除処理
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


// Misc. / 雑多な関数（主にplayers管理用）
///////////////////////////////////////////////////////////////////////////////

// 指定したAudioPlayerが存在するか調べる
int WINAPI APExists(index_t index) {
	return _APExists(index) ? 1 : 0;
}

// 指定したAudioPlayerを閉じる
void WINAPI APClose(index_t index) {
	if (!_APExists(index)) return;
	_APForceClose(index);
}

// 全てのAudioPlayerを閉じる
void WINAPI APFree() {
	for (index_t i = 0; i < players.size(); i++) {
		if (!players[i].player) {
			continue;
		}
		_APForceClose(i);
	}
}

// pendingCloseなAudioPlayerを全て閉じる
// 通常呼ぶ必要はない
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


// Operater / 操作関数
///////////////////////////////////////////////////////////////////////////////

// 再生終了フラグ取得
int WINAPI APGetFinishFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetFinishFlag() ? 1: 0;
}

// 再生位置取得（サンプル単位）
int WINAPI APGetPosition(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetPosition();
}

// 再生位置取得（秒単位）
void WINAPI APGetPositionS(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetPositionS();
}

// 再生位置設定（サンプル単位）
void WINAPI APSetPosition(index_t index, int p_position) {
	if (!_APExists(index)) return;
	try {
		players[index].player->SetPosition((sample_t)p_position);
	} catch (...) {
		// 握りつぶすよりほかない
	}
}

// 再生位置設定（秒単位）
void WINAPI APSetPositionS(index_t index, double p_position) {
	if (!_APExists(index)) return;
	try {
		players[index].player->SetPosition((second_t)p_position);
	} catch (...) {
		// 握りつぶすよりほかない
	}
}

// 長さ取得（サンプル単位）
int WINAPI APGetLength(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetLength();
}

// 長さ取得（秒単位）
void WINAPI APGetLengthS(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetLengthS();
}

// 再生
void WINAPI APPlay(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Play();
}

// 停止
void WINAPI APStop(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Stop();
}

// 停止して閉じる
void WINAPI APStopClose(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Stop();
	APClose(index);
}

// 再再生
void WINAPI APReplay(index_t index) {
	if (!_APExists(index)) return;
	players[index].player->Replay();
}

// ボリューム取得
void WINAPI APGetVolume(index_t index, double *p_ret) {
	if (!_APExists(index)) return;
	if (p_ret != NULL) *p_ret = players[index].player->GetVolume();
}

// ボリューム設定
void WINAPI APSetVolume(index_t index, double p_volume) {
	if (!_APExists(index)) return;
	players[index].player->SetVolume(p_volume);
}

// ポーズフラグ取得
int WINAPI APGetPauseFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetPauseFlag() ? 1 : 0;
}

// ポーズフラグ設定
void WINAPI APSetPauseFlag(index_t index, int p_pauseFlag) {
	if (!_APExists(index)) return;
	players[index].player->SetPauseFlag(p_pauseFlag != 0 ? true : false);
}

// ループフラグ取得
int WINAPI APGetLoopFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].player->GetLoopFlag();
}

// ループフラグ設定
void WINAPI APSetLoopFlag(index_t index, int p_loopFlag) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopFlag(p_loopFlag);
}

// ループ位置取得（サンプル単位）
void WINAPI APGetLoopPoint(index_t index, int *p_beginLoop, int *p_loopLength) {
	if (!_APExists(index)) return;
	AudioPlayer::sample_t beginLoop, loopLength;
	players[index].player->GetLoopPoint(&beginLoop, &loopLength);
	if (p_beginLoop != nullptr) *p_beginLoop = beginLoop;
	if (p_loopLength != nullptr) *p_loopLength = loopLength;
}

// ループ位置取得（秒単位）
void WINAPI APGetLoopPointS(index_t index, double *p_beginLoop, double *p_loopLength) {
	if (!_APExists(index)) return;
	AudioPlayer::second_t beginLoop, loopLength;
	players[index].player->GetLoopPoint(&beginLoop, &loopLength);
	if (p_beginLoop != nullptr) *p_beginLoop = beginLoop;
	if (p_loopLength != nullptr) *p_loopLength = loopLength;
}

// ループ位置設定（サンプル単位）
void WINAPI APSetLoopPoint(index_t index, int p_beginLoop, int p_loopLength) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopPoint((sample_t)p_beginLoop, (sample_t)p_loopLength);
}

// ループ位置設定（秒単位）
void WINAPI APSetLoopPointS(index_t index, double p_beginLoop, double p_loopLength) {
	if (!_APExists(index)) return;
	players[index].player->SetLoopPoint((second_t)p_beginLoop, (second_t)p_loopLength);
}

// 自動解放フラグ取得
int WINAPI APGetAutoCloseFlag(index_t index) {
	if (!_APExists(index)) return 0;
	return players[index].autoClose ? 1 : 0;
}

// 自動解放フラグ設定
void WINAPI APSetAutoCloseFlag(index_t index, bool flag) {
	if (!_APExists(index)) return;
	if (flag == players[index].autoClose) return;

	players[index].autoClose = flag;

	if (flag) {
		players[index].player->SetFinishCallback([index](AudioPlayer *player) {
			// 削除予約を入れておく
			// ここで削除すると死ぬためここでは削除できない（この関数はplayerのメンバから呼び出されている）
			// 実際に削除されるのは
			//     ・次にこのplayerが操作されそうになった時
			//     ・今後の_APOpenでこのplayer以前に空きが存在しなかった時
			//     ・次にAPClearが呼ばれた時
			// のいずれかになる
			players[index].pendingClose = true;
			return 1;
		});
	} else {
		players[index].player->ClearFinishCallback();
	}
}