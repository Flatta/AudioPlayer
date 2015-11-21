#ifndef __AUDIOTRANSCODER_HPP
#define __AUDIOTRANSCODER_HPP

namespace AudioTranscoder {
	// バッファタイプ
	enum BUFFERTYPE {
		BUFFERTYPE_FIRST__ = 0,
		BUFFERTYPE_INT8,
		BUFFERTYPE_INT8U,
		BUFFERTYPE_INT16,
		BUFFERTYPE_INT16U,
		BUFFERTYPE_INT24,
		BUFFERTYPE_INT24U,
		BUFFERTYPE_INT32,
		BUFFERTYPE_INT32U,
		BUFFERTYPE_FLOAT,
		BUFFERTYPE_DOUBLE,
		BUFFERTYPE_LAST__,
	};

	// 関数
	bool IsValid(BUFFERTYPE bufferType);
	int SizeOf(BUFFERTYPE bufferType);
	int BitSizeOf(BUFFERTYPE bufferType);
	double BufferToDouble(const void *buffer, BUFFERTYPE bufferType);
	void DoubleToBuffer(double value, void *buffer, BUFFERTYPE bufferType);
	void Transcode(const void *src, BUFFERTYPE srcType, void *dst, BUFFERTYPE dstType, double coef);
	void Transcode(const char *src, BUFFERTYPE srcType, char *dst, BUFFERTYPE dstType, size_t size, double coef);
};

#endif