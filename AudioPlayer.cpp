#define NOMINMAX

#include "AudioPlayer.hpp"
#include <algorithm>
#include <Mmreg.h>


using namespace std;
using namespace AudioTranscoder;


// 値を一定の範囲に収める関数
template <typename T>
inline T clamp(T value, T minValue, T maxValue) {
	return min(max(value, minValue), maxValue);
}


// コールバック関数
// どうやらメンバ関数のポインタは取得できないようなので、こいつのdwInstanceにthisのポインタを渡してやる
inline void CALLBACK g_waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	auto player = (AudioPlayer*)dwInstance;
	player->waveOutProc(hwo, uMsg, dwInstance, dwParam1, dwParam2);
}


// 変換関数（内部用）
// ミリ秒やバイトとの変換関数はともかく上2つは公開したほうが良いかもしれない
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


// バッファ取得
inline char* AudioPlayer::GetBuffer(int index) {
	return &outputBuffer[index * SampleToOutputByte(bufSize)];
}


// WAVEHDR準備
void AudioPlayer::WriteBuffer(WAVEHDR *curwhdr) {
	// 終了していたらもう何も行わない
	if (finishWriteFlag) {
		curwhdr->dwBufferLength = 0;
		curwhdr->dwUser = 0;
		return;
	}

	// フラグ初期化
	int flag = 0;

	// 書き込み
	sample_t position = 0;
	for (;;) {
		sample_t tempSize;
		
		// バッファ残りサイズ計算
		tempSize = bufSize - position;
		if (tempSize <= 0) {
			// もうデコードしなくても良い
			break;
		}

		// ループの末端か調べる
		bool isEndLoopBlock = false;
		sample_t curPos;
		if (loopFlag & LOOPFLAG_SECTION) {
			curPos = decoder->Tell();
			isEndLoopBlock = endLoopPoint - tempSize <= curPos && curPos < endLoopPoint;
		}

		// セクションをデコード
		sample_t read;
		sample_t readSize = isEndLoopBlock ? endLoopPoint - curPos : tempSize;
		try {
			read = decoder->Read(tempInputBuffer.get() + decoder->SampleToByte(position), readSize);
			if (read == 0) {
				// End of File
				// もうデコードできない
				if (loopFlag & LOOPFLAG_FILE) {
					// ループ処理
					decoder->Seek(0);
					flag |= BLOCKFLAG_LOOP_FILE;
				} else {
					// 終了処理
					finishWriteFlag = true;
					flag |= BLOCKFLAG_LAST;
					break;
				}
			}
		} catch (...) {
			// エラー
			// とりあえず0バイト読み込んだということにしておく
			read = 0;
			flag |= BLOCKFLAG_ERROR;
		}

		// 書き込み位置修正
		position += read;

		// ループ用シーク
		if (isEndLoopBlock && decoder->Tell() >= endLoopPoint) {
			decoder->Seek(beginLoopPoint);
			curwhdr->dwUser |= BLOCKFLAG_LOOP_SECTION;
		}
	};

	// 変換
	// TODO: Fade
	Transcode(tempInputBuffer.get(), decoder->bufferType, curwhdr->lpData, outputBufferType, position * decoder->channelNum, volume);

	// 情報書き込み
	curwhdr->dwBufferLength = SampleToOutputByte(position);
	curwhdr->dwUser = flag;

	return;
}


// コールバックから呼ばれる関数
// コールバックの実体のようなもの
void AudioPlayer::waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	// 終わってるなら何もしない
	// こうしないとデストラクタを呼んだ時にフリーズする模様
	if (finishPlayFlag) return;

	// 出力バッファ再生終了
	if (uMsg == MM_WOM_DONE) {
		// 次の出力バッファのインデックス取得
		int nextIndex = currentIndex + 1;
		if (nextIndex >= bufNum) nextIndex = 0;

		// フラグ取得
		int flag = whdr[currentIndex].dwUser;

		// 最終ブロック
		if (flag & BLOCKFLAG_LAST) {
			finishPlayFlag = true;
			if (finishCallback != nullptr) {
				finishCallback(this);
			}
		}

		// ループ処理がされたブロック
		if (flag & (BLOCKFLAG_LOOP_FILE | BLOCKFLAG_LOOP_SECTION)) {
			if (loopCallback != nullptr) {
				loopCallback(this, (flag & BLOCKFLAG_LOOP_FILE) != 0);
			}
		}

		// 時刻を修正
		lastPlayedTime = timeGetTime();
		lastSectionPosition = sectionPosition[nextIndex];

		sectionPosition[currentIndex] = decoder->Tell();

		// 出力バッファに書き込み
		WriteBuffer(&whdr[currentIndex]);
		waveOutWrite(hwo, &whdr[currentIndex], sizeof(WAVEHDR));

		// 出力バッファのインデックス更新
		currentIndex = nextIndex;
	}
}


// コンストラクタ
AudioPlayer::AudioPlayer(unique_ptr<IDecoder> p_decoder, int p_openFlag, OUTPUTFORMAT p_outputFormat, sample_t p_bufSize, unsigned int p_bufNum) : decoder(move(p_decoder)) {
	// 値チェック
	if (p_bufSize <= 0 || p_bufNum <= 0) {
		throw ERRORCODE_INVALID_ARGUMENT;
	}

	// 値コピー
	openFlag = p_openFlag;
	outputFormat = p_outputFormat;
	bufNum = max(p_bufNum, 1u);
	bufSize = p_bufSize;

	// 出力バッファの形式設定
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
			// BUFFERTYPE_FLOATを使っても良いかもしれない
			throw ERRORCODE_INVALID_ARGUMENT;
	}

	// いろいろ初期化
	hwo = nullptr;
	currentIndex = 0;
	volume = 1.0;

	// フラグ初期化
	stopFlag = true;
	finishWriteFlag = true;
	finishPlayFlag = true;
	pauseFlag = false;
	loopFlag = 0;

	// いろいろ準備と初期化
	sectionPosition = make_unique<sample_t[]>(bufNum);
	lastPlayedTime = timeGetTime();
	lastSectionPosition = 0;

	// コールバック初期化
	finishCallback = nullptr;
	loopCallback = nullptr;
	fadeCallback = nullptr;


	// 以下decoderを元に準備

	// 値チェック
	if (!IsValid(decoder->bufferType) || decoder->channelNum <= 0 || decoder->samplingRate <= 0) {
		throw ERRORCODE_INTERNAL_ERROR;
	}

	// _ReadBuffer用一時バッファ作成
	tempInputBuffer = make_unique<char[]>(decoder->SampleToByte(bufSize));


	// ビット数とブロック境界計算
	unsigned int bitNum = BitSizeOf(outputBufferType);
	unsigned int blockAlign = decoder->channelNum * SizeOf(outputBufferType);

	// wfx準備
	WAVEFORMATEX wfx = {
		(WORD)(outputFormat == OUTPUTFORMAT_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM),
		(WORD)decoder->channelNum,
		(DWORD)decoder->samplingRate,
		(DWORD)decoder->samplingRate * blockAlign,
		(WORD)blockAlign,
		(WORD)bitNum,
		(WORD)0
	};

	// MMEを開く
	auto ret = waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD_PTR)&g_waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if (ret != MMSYSERR_NOERROR) {
		// デストラクタで問題が発生するようなら hwo = nullptr と書いてみよう
		throw ERRORCODE_CANNOT_OPEN_DEVICE;
	}

	// PCMバッファ準備
	outputBuffer = make_unique<char[]>(SampleToOutputByte(bufSize) * bufNum);

	// 再生バッファ準備
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

	// 自動再生処理
	if (openFlag & OPENFLAG_AUTOSTART) {
		Play();
	}
}

// デストラクタ
AudioPlayer::~AudioPlayer() {
	// 強制的に終了したことにする
	stopFlag = true;
	finishPlayFlag = true;
	finishWriteFlag = true;

	// MMEの後片付け
	if (hwo != nullptr) {
		waveOutReset(hwo);
		for (unsigned int i = 0; i < bufNum; i++) {
			waveOutUnprepareHeader(hwo, &whdr[i], sizeof(WAVEHDR));
		}
		waveOutClose(hwo);
	}
}


// 位置や長さの取得と設定
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
	// なんかフリーズする
	// 正確にはランダムにwaveOutGetPositionから帰ってこなくなる
	// 呼ぶ頻度が原因かも
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
	// 手抜き
	return SampleToSecond(GetPosition());
}

AudioPlayer::sample_t AudioPlayer::GetLength() {
	return decoder->bufferLength;
}

AudioPlayer::second_t AudioPlayer::GetLengthS() {
	return SampleToSecond(GetLength());
}


// 再生と停止
void AudioPlayer::Play() {
	if (!finishPlayFlag) return;

	// 各種フラグをセット
	stopFlag = false;
	finishWriteFlag = false;
	finishPlayFlag = false;

	// バッファを最初の場所に戻す
	decoder->Seek(0);
	currentIndex = 0;

	// 再生時間計測用変数を準備
	lastPlayedTime = timeGetTime();
	lastSectionPosition = 0;

	// バッファ追加
	for (unsigned int i = 0; i < bufNum; i++) {
		if (finishWriteFlag) break;

		// 終了していなければ再生バッファに書き込み
		sectionPosition[i] = decoder->Tell();
		WriteBuffer(&whdr[i]);
		waveOutWrite(hwo, &whdr[i], sizeof(WAVEHDR));

		// 初回のバッファの再生時刻を正確にするためにここでも更新しておく
		if (i == 0) {
			lastPlayedTime = timeGetTime();
		}
	}
}

void AudioPlayer::Stop() {
	if (finishPlayFlag) return;

	// 各種フラグをセット
	stopFlag = true;
	finishWriteFlag = true;
	finishPlayFlag = true;
	pauseFlag = false;

	// バッファをリセット
	waveOutReset(hwo);
}

void AudioPlayer::Replay() {
	Stop();
	Play();
}


// フラグ関連
bool AudioPlayer::GetFinishFlag() {
	return finishPlayFlag;
}

bool AudioPlayer::GetPauseFlag() {
	return pauseFlag;
}

void AudioPlayer::SetPauseFlag(bool p_pauseFlag) {
	if (finishPlayFlag || p_pauseFlag == pauseFlag) return;
	if (p_pauseFlag) {
		// 一時停止
		pausedTime = GetPosition();
		pausedLastPlayedTime = lastPlayedTime - timeGetTime();
		waveOutPause(hwo);
	} else {
		// 一時停止解除
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


// ループポイント
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


// ボリューム
double AudioPlayer::GetVolume() {
	return volume;
}

void AudioPlayer::SetVolume(double p_volume) {
	volume = p_volume;
}


// コールバックの設定と解除
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