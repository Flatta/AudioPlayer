#include "AudioTranscoder.hpp"
#include "Int24.hpp"
#include <algorithm>
#include <climits>


using namespace std;


// �W��
const double k8  = (double)_I8_MIN  * -1.0;
const double k16 = (double)_I16_MIN * -1.0;
const double k24 = (double)_I24_MIN * -1.0;
const double k32 = (double)_I32_MIN * -1.0;

// �W���̋t��
const double d8  = 1.0 / k8;
const double d16 = 1.0 / k16;
const double d24 = 1.0 / k24;
const double d32 = 1.0 / k32;

// �l�����͈̔͂Ɏ��߂�֐�
template <typename T>
inline T clamp(T value, T minValue, T maxValue) {
	return min(max(value, minValue), maxValue);
}


namespace AudioTranscoder {
	// �L���Ȍ^���`�F�b�N
	bool IsValid(BUFFERTYPE bufferType) {
		return bufferType > BUFFERTYPE_FIRST__ && bufferType < BUFFERTYPE_LAST__;
	}

	// �o�b�t�@�^�C�v���T�C�Y�i�o�C�g�P�ʁj
	int SizeOf(BUFFERTYPE bufferType) {
		switch (bufferType) {
			case BUFFERTYPE_INT8:
			case BUFFERTYPE_INT8U:
				return sizeof(int8_t);

			case BUFFERTYPE_INT16:
			case BUFFERTYPE_INT16U:
				return sizeof(int16_t);

			case BUFFERTYPE_INT24:
			case BUFFERTYPE_INT24U:
				return sizeof(int24_t);

			case BUFFERTYPE_INT32:
			case BUFFERTYPE_INT32U:
				return sizeof(int32_t);

			case BUFFERTYPE_FLOAT:
				return sizeof(float);

			case BUFFERTYPE_DOUBLE:
				return sizeof(double);
		}
		throw 1;
	}

	// �o�b�t�@�^�C�v���T�C�Y�i�r�b�g�P�ʁj
	int BitSizeOf(BUFFERTYPE bufferType) {
		return SizeOf(bufferType) * 8;
	}

	// �o�b�t�@��double
	double BufferToDouble(const void *buffer, BUFFERTYPE bufferType) {
		switch (bufferType) {
			case BUFFERTYPE_INT8:
				return d8 * (double)(*(int8_t *)buffer);

			case BUFFERTYPE_INT8U:
				return d8 * (double)(*(uint8_t *)buffer + _I8_MIN);

			case BUFFERTYPE_INT16:
				return d16 * (double)(*(int16_t *)buffer);

			case BUFFERTYPE_INT16U:
				return d16 * (double)(*(uint16_t *)buffer + _I16_MIN);

			case BUFFERTYPE_INT24:
				return d24 * (double)((int)(*(int24_t *)buffer));

			case BUFFERTYPE_INT24U:
			{
				// signed int �ɕϊ����Ă�����
				int32_t temp = (int32_t)(*(int24_t *)buffer);
				temp = temp < 0 ? (temp & 0x007FFFFF) | 0x80000000 : temp;
				return d24 * (double)(temp + _I24_MIN);
			}

			case BUFFERTYPE_INT32:
				return d32 * (double)(*(int32_t *)buffer);

			case BUFFERTYPE_INT32U:
				return d32 * ((double)(*(uint32_t *)buffer) + (double)_I32_MIN);

			case BUFFERTYPE_FLOAT:
				return (double)(*(float *)buffer);

			case BUFFERTYPE_DOUBLE:
				return *(double *)buffer;
		}
		throw 1;
	}

	// double���o�b�t�@
	void DoubleToBuffer(double value, void *buffer, BUFFERTYPE bufferType) {
		double cValue = clamp(value, -1.0, 1.0);
		switch (bufferType) {
			case BUFFERTYPE_INT8:
				*(int8_t *)buffer = clamp((int)(cValue * k8), (int)_I8_MIN, (int)_I8_MAX);
				return;

			case BUFFERTYPE_INT8U:
				*(uint8_t *)buffer = clamp((int)(cValue * k8) - _I8_MIN, 0, (int)_UI8_MAX);
				return;

			case BUFFERTYPE_INT16:
				*(int16_t *)buffer = clamp((long)(cValue * k16), (long)_I16_MIN, (long)_I16_MAX);
				return;

			case BUFFERTYPE_INT16U:
				*(uint16_t *)buffer = clamp((long)(cValue * k16) - _I16_MIN, 0L, (long)_UI16_MAX);
				return;

			case BUFFERTYPE_INT24:
				*(int24_t *)buffer = clamp((long)(cValue * k24), (long)_I24_MIN, (long)_I24_MAX);
				return;

			case BUFFERTYPE_INT24U:
				{
					int32_t temp = clamp((int32_t)(cValue * k24) - _I24_MIN, (int32_t)0, (int32_t)_UI24_MAX);
					if (temp & 0x00800000) temp |= 0xFF000000;
					*(int24_t *)buffer = temp;
				}
				return;

			case BUFFERTYPE_INT32:
				*(int32_t *)buffer = clamp((long long)(cValue * k32), (long long)_I32_MIN, (long long)_I32_MAX);
				return;

			case BUFFERTYPE_INT32U:
				*(uint32_t *)buffer = clamp((long long)(cValue * k32) - (long long)_I32_MIN, (long long)0, (long long)_UI32_MAX);
				return;

			case BUFFERTYPE_FLOAT:
				*(float *)buffer = (float)value;
				return;

			case BUFFERTYPE_DOUBLE:
				*(double *)buffer = value;
				return;
		}
		throw 1;
	}

	void Transcode(const void *src, BUFFERTYPE srcType, void *dst, BUFFERTYPE dstType, double coef) {
		DoubleToBuffer(BufferToDouble(src, srcType) * coef, dst, dstType);
	}

	void Transcode(const char *src, BUFFERTYPE srcType, char *dst, BUFFERTYPE dstType, size_t size, double coef) {
		for (size_t i = 0; i < size; i++) {
			DoubleToBuffer(BufferToDouble(src, srcType) * coef, dst, dstType);
			src += SizeOf(srcType);
			dst += SizeOf(dstType);
		}
	}
};