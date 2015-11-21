#ifndef __MAIN_HPP
#define __MAIN_HPP

#include "Config.hpp"
#include "AudioPlayer.hpp"
#include <Windows.h>

constexpr auto ERRORCODE_DECODER_DISABLED = -256;
constexpr auto OPENFLAG_AUTOCLOSE = 0x00000010;

#ifdef ENABLE_FLAC
# define FLAC_DUMMY_IMPL
#else
# define FLAC_DUMMY_IMPL { return ERRORCODE_DECODER_DISABLED; }
#endif

#ifdef ENABLE_OPUS
# define OPUS_DUMMY_IMPL
#else
# define OPUS_DUMMY_IMPL { return ERRORCODE_DECODER_DISABLED; }
#endif

#ifdef ENABLE_VORBIS
# define VORBIS_DUMMY_IMPL
#else
# define VORBIS_DUMMY_IMPL { return ERRORCODE_DECODER_DISABLED; }
#endif

#ifdef ENABLE_WAV
# define WAV_DUMMY_IMPL
#else
# define WAV_DUMMY_IMPL { return ERRORCODE_DECODER_DISABLED; }
#endif


using index_t = unsigned int;


bool _APExists(index_t index);
index_t _APOpen(AudioPlayer *player, int openFlag);
void _APForceClose(index_t index);


index_t WINAPI APOpenFLACFileA(const char *p_audioFile, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) FLAC_DUMMY_IMPL;
index_t WINAPI APOpenFLACFileW(const wchar_t *p_audioFile, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) FLAC_DUMMY_IMPL;
index_t WINAPI APOpenFLACMemory(const char *p_audioBuf, size_t p_audioSize, int p_checkMD5, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) FLAC_DUMMY_IMPL;

index_t WINAPI APOpenOpusFileA(const char *p_audioFile, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) OPUS_DUMMY_IMPL;
index_t WINAPI APOpenOpusFileW(const wchar_t *p_audioFile, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) OPUS_DUMMY_IMPL;
index_t WINAPI APOpenOpusMemory(const char *p_audioBuf, size_t p_audioSize, int p_useFloat, int p_dither, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) OPUS_DUMMY_IMPL;

index_t WINAPI APOpenVorbisFileA(const char *p_audioFile, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) VORBIS_DUMMY_IMPL;
index_t WINAPI APOpenVorbisFileW(const wchar_t *p_audioFile, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) VORBIS_DUMMY_IMPL;
index_t WINAPI APOpenVorbisMemory(const char *p_audioBuf, size_t p_audioSize, int p_bitNum, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) VORBIS_DUMMY_IMPL;

index_t WINAPI APOpenWavFileA(const char *p_audioFile, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) WAV_DUMMY_IMPL;
index_t WINAPI APOpenWavFileW(const wchar_t *p_audioFile, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) WAV_DUMMY_IMPL;
index_t WINAPI APOpenWavMemory(const char *p_audioBuf, size_t p_audioSize, int p_openFlag, int p_outputFormat, int p_bufSize, int p_bufNum) WAV_DUMMY_IMPL;

int WINAPI APExists(index_t index);
void WINAPI APClose(index_t index);
void WINAPI APFree();
void WINAPI APClear();

int WINAPI APGetFinishFlag(index_t index);

int WINAPI APGetPosition(index_t index);
void WINAPI APGetPositionS(index_t index, double *p_ret);
void WINAPI APSetPosition(index_t index, int p_position);
void WINAPI APSetPositionS(index_t index, double p_position);

int WINAPI APGetLength(index_t index);
void WINAPI APGetLengthS(index_t index, double *p_ret);

void WINAPI APPlay(index_t index);
void WINAPI APStop(index_t index);
void WINAPI APStopClose(index_t index);
void WINAPI APReplay(index_t index);

void WINAPI APGetVolume(index_t index, double *p_ret);
void WINAPI APSetVolume(index_t index, double p_volume);

int WINAPI APGetPauseFlag(index_t index);
void WINAPI APSetPauseFlag(index_t index, int p_pauseFlag);

int WINAPI APGetLoopFlag(index_t index);
void WINAPI APSetLoopFlag(index_t index, int p_loopFlag);

void WINAPI APGetLoopPoint(index_t index, int *p_beginLoop, int *p_loopLength);
void WINAPI APGetLoopPointS(index_t index, double *p_beginLoop, double *p_loopLength);
void WINAPI APSetLoopPoint(index_t index, int p_beginLoop, int p_loopLength);
void WINAPI APSetLoopPointS(index_t index, double p_beginLoop, double p_loopLength);

int WINAPI APGetAutoCloseFlag(index_t index);
void WINAPI APSetAutoCloseFlag(index_t index, bool flag);

#endif