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
		ERRORCODE_NONE = 0,					// エラー無し
		ERRORCODE_UNKNOWN_ERROR,			// 不明なエラー
		ERRORCODE_INTERNAL_ERROR,			// 内部エラー
		ERRORCODE_INVALID_ARGUMENT,			// 引数が不正
		ERRORCODE_CANNOT_OPEN_DEVICE,		// デバイスを開けなかった
	};

	const enum OUTPUTFORMAT {
		OUTPUTFORMAT_UINT8 = 0,				//  8bit unsigned int
		OUTPUTFORMAT_INT16,					// 16bit   signed int
		OUTPUTFORMAT_FLOAT,					// 32bit   signed float
	};

	static constexpr unsigned int OPENFLAG_AUTOSTART      = 0x00000001;		// 自動再生
	
	static constexpr unsigned int LOOPFLAG_FILE           = 0x00000001;		// ファイル全体をループ
	static constexpr unsigned int LOOPFLAG_SECTION        = 0x00000002;		// 指定した区間をループ
	
	static constexpr unsigned int BLOCKFLAG_ERROR         = 0x00000001;		// エラーが発生した
	static constexpr unsigned int BLOCKFLAG_LAST          = 0x00000002;		// 最後
	static constexpr unsigned int BLOCKFLAG_LOOP_FILE     = 0x00000004;		// ループ処理（ファイル）が発生した
	static constexpr unsigned int BLOCKFLAG_LOOP_SECTION  = 0x00000008;		// ループ処理（区間）が発生した
	static constexpr unsigned int BLOCKFLAG_FADE_BEGIN    = 0x00000010;		// フェード開始処理が発生した
	static constexpr unsigned int BLOCKFLAG_FADE_END      = 0x00000020;		// フェード終了処理が発生した

private:
	unsigned int bufNum;											// マルチバッファリング用出力バッファ数、1以上（2か3推奨）
	sample_t bufSize;												// 各バッファのサンプル数、1以上（サンプリングレート44100Hz、バッファ数2または3の場合は2048か4096あたりを推奨）

	std::unique_ptr<char[]> tempInputBuffer;						// _ReadBufferにデータを入れてもらう一時変数

	OUTPUTFORMAT outputFormat;										// 出力フォーマット
	AudioTranscoder::BUFFERTYPE outputBufferType;					// 出力フォーマットのデータの型

	HWAVEOUT hwo;
	std::unique_ptr<WAVEHDR[]> whdr;

	unsigned int currentIndex;										// 現在の出力バッファのインデックス
	std::unique_ptr<char[]> outputBuffer;							// 出力バッファ
	std::unique_ptr<sample_t[]> sectionPosition;					// 出力バッファの絶対位置
	msec_t lastPlayedTime;											// 最後に出力バッファを再生し終わった時刻
	sample_t lastSectionPosition;									// 最後の出力バッファの位置

	double volume;													// ボリューム

	int openFlag;													// 動作フラグ

	bool pauseFlag;													// 一時停止フラグ
	sample_t pausedTime;											// 一時停止時の再生位置
	msec_t pausedLastPlayedTime;									// 一時停止時のlastTimeとtimeGetTime()の差分

	int loopFlag;													// ループ再生フラグ
	sample_t beginLoopPoint;										// ループ開始地点
	sample_t endLoopPoint;											// ループ終了地点

	bool stopFlag;													// 停止中フラグ、主にGetPosition用

	bool finishWriteFlag;											// 再生バッファ書き込み終了フラグ
	bool finishPlayFlag;											// 再生終了フラグ

	std::function<void(AudioPlayer *)> finishCallback;				// 再生終了時に呼ばれるコールバック
	std::function<void(AudioPlayer *, bool)> loopCallback;			// ループ時に呼ばれるコールバック、最大でバッファサイズ分だけ呼ばれるタイミングがずれる
	std::function<void(AudioPlayer *)> fadeCallback;				// フェード終了時に呼ばれるコールバック、同じくずれる

	second_t SampleToSecond(sample_t sample);						// サンプル→秒
	sample_t SecondToSample(second_t second);						// 秒→サンプル
	sample_t MillisecondToSample(msec_t millisecond);				// ミリ秒→サンプル
	byte_t SampleToOutputByte(sample_t sample);						// サンプル→出力データのバイト
	sample_t OutputByteToSample(byte_t byte);						// 出力データのバイト→サンプル
	void WriteBuffer(WAVEHDR *curwhdr);								// データを読み取ってcurwhdrを準備する

	char* GetBuffer(int index);										// バッファ取得

	std::unique_ptr<IDecoder> decoder;								// デコーダー

public:
	AudioPlayer(std::unique_ptr<IDecoder> p_decoder, int p_openFlag, OUTPUTFORMAT p_outputFormat, sample_t p_bufSize, unsigned int p_bufNum);
	~AudioPlayer();

	void waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	sample_t GetPosition();																// 現在の再生位置を取得（サンプル）
	second_t GetPositionS();															// 現在の再生位置を取得（秒）
	void SetPosition(sample_t position);												// 再生位置を設定（サンプル）
	void SetPosition(second_t position);												// 再生位置を設定（秒）
	sample_t GetLength();																// 総再生時間を取得（サンプル）
	second_t GetLengthS();																// 総再生時間を取得（秒）
	void Play();																		// 再生する
	void Stop();																		// 停止する
	void Replay();																		// 停止して再生する
	bool GetFinishFlag();																// 終了フラグ取得
	bool GetPauseFlag();																// 一時停止フラグ取得
	void SetPauseFlag(bool p_pauseFlag);												// 一時停止または再開する
	int GetLoopFlag();																	// ループ再生フラグ取得
	void SetLoopFlag(int p_loopFlag);													// ループ再生フラグ設定
	void GetLoopPoint(sample_t *p_beginLoop, sample_t *p_loopLength);					// ループ区間取得（サンプル）
	void GetLoopPoint(second_t *p_beginLoop, second_t *p_loopLength);					// ループ区間取得（秒）
	void SetLoopPoint(sample_t p_beginLoop, sample_t p_loopLength);						// ループ区間設定（サンプル）
	void SetLoopPoint(second_t p_beginLoop, second_t p_loopLength);						// ループ区間設定（秒）
	double GetVolume();																	// ボリューム取得
	void SetVolume(double p_volume);													// ボリューム設定
	void Fade(sample_t duration, double to, bool relative);								// フェード
	void FadeIn(sample_t duration);														// フェードイン
	void FadeOut(sample_t duration);													// フェードアウト
	void StopFade(bool finish);															// フェードを解除
	void SetFinishCallback(const std::function<void(AudioPlayer *)> &callback);			// 再生終了時のコールバックを設定
	void ClearFinishCallback();															// 再生終了時のコールバックを解除
	void SetLoopCallback(const std::function<void(AudioPlayer *, bool)> &callback);		// ループ時のコールバックを設定
	void ClearLoopCallback();															// ループ時のコールバックを解除
	void SetFadeCallback(const std::function<void(AudioPlayer *)> &callback);			// フェード終了時のコールバックを設定
	void ClearFadeCallback();															// フェード終了時のコールバックを解除
};

#endif