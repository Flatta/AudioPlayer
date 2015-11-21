#ifndef __AUDIOPLAYER_AS
#define __AUDIOPLAYER_AS

#uselib "AudioPlayer.dll"
#cfunc APExists           "APExists"           int
#cfunc APOpenFLACFileA    "APOpenFLACFileA"    sptr, int, int, int, int, int
#cfunc APOpenFLACFileW    "APOpenFLACFileW"    wptr, int, int, int, int, int
#cfunc APOpenFLACMemory   "APOpenFLACMemory"   var, int, int, int, int, int, int
#cfunc APOpenOpusFileA    "APOpenOpusFileA"    sptr, int, int, int, int, int, int
#cfunc APOpenOpusFileW    "APOpenOpusFileW"    wptr, int, int, int, int, int, int
#cfunc APOpenOpusMemory   "APOpenOpusMemory"   var, int, int, int, int, int, int, int
#cfunc APOpenVorbisFileA  "APOpenVorbisFileA"  sptr, int, int, int, int, int, int
#cfunc APOpenVorbisFileW  "APOpenVorbisFileW"  wptr, int, int, int, int, int
#cfunc APOpenVorbisMemory "APOpenVorbisMemory" var, int, int, int, int, int, int
#cfunc APOpenWavFileA     "APOpenWavFileA"     sptr, int, int, int, int
#cfunc APOpenWavFileW     "APOpenWavFileW"     wptr, int, int, int, int
#cfunc APOpenWavMemory    "APOpenWavMemory"    var, int, int, int, int, int
#func  APClose            "APClose"            int
#cfunc APGetFinishFlag    "APGetFinishFlag"    int
#cfunc APGetPosition      "APGetPosition"      int
#func  APGetPositionS     "APGetPositionS"     int, var
#func  APSetPosition      "APSetPosition"      int, int
#func  APSetPositionS     "APSetPositionS"     int, double
#cfunc APGetLength        "APGetLength"        int
#func  APGetLengthS       "APGetLengthS"       int, var
#func  APPlay             "APPlay"             int
#func  APStop             "APStop"             int
#func  APReplay           "APReplay"           int
#func  APGetVolume        "APGetVolume"        int, var
#func  APSetVolume        "APSetVolume"        int, double
#cfunc APGetPauseFlag     "APGetPauseFlag"     int
#func  APSetPauseFlag     "APSetPauseFlag"     int, int
#cfunc APGetLoopFlag      "APGetLoopFlag"      int
#func  APSetLoopFlag      "APSetLoopFlag"      int, int
#func  APGetLoopPoint     "APGetLoopPoint"     int, var, var
#func  APGetLoopPointS    "APGetLoopPointS"    int, var, var
#func  APSetLoopPoint     "APSetLoopPoint"     int, int, int
#func  APSetLoopPointS    "APSetLoopPointS"    int, double, double
#cfunc APGetAutoCloseFlag "APGetAutoCloseFlag" int
#func  APSetAutoCloseFlag "APSetAutoCloseFlag" int, int
#func  APClear            "APClear"
#func  APFree      onexit "APFree"

#define APOpenOpusFile APOpenOpusFileA
#define APOpenVorbisFile APOpenVorbisFileA
#define APOpenWavFile APOpenWavFileA


#enum OUTPUTFORMAT_UINT8 = 0
#enum OUTPUTFORMAT_INT16
#enum OUTPUTFORMAT_INT24
#enum OUTPUTFORMAT_INT32
#enum OUTPUTFORMAT_FLOAT

#define OPENFLAG_AUTOSTART 0x00000001
#define OPENFLAG_AUTOCLOSE 0x00000010

#define LOOPFLAG_FILE    0x00000001
#define LOOPFLAG_SECTION 0x00000002

#endif