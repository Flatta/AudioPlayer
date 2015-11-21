#ifndef __INT24_HPP
#define __INT24_HPP

#include <cstdint>

#define _I24_MAX 8388607
#define _I24_MIN (-8388608)
#define _UI24_MAX 0xFFFFFF

class int24_t {
protected:
	unsigned char m_Internal[3];

public:
	int24_t() {
	}

	int24_t(const int32_t val) {
		*this = val;
	}

	int24_t(const int24_t& val) {
		*this = val;
	}

	operator int32_t() const {
		// Is this a negative?  Then we need to siingn extend.
		if (m_Internal[2] & 0x80) {
			return (0xFF << 24) | (m_Internal[2] << 16) | (m_Internal[1] << 8) | (m_Internal[0] << 0);
		} else {
			return (m_Internal[2] << 16) | (m_Internal[1] << 8) | (m_Internal[0] << 0);
		}
	}

	operator float() const {
		return (float)this->operator int32_t();
	}

	int24_t& operator =(const int24_t& input) {
		m_Internal[0] = input.m_Internal[0];
		m_Internal[1] = input.m_Internal[1];
		m_Internal[2] = input.m_Internal[2];

		return *this;
	}

	int24_t& operator =(const int32_t input) {
		m_Internal[0] = ((unsigned char*)&input)[0];
		m_Internal[1] = ((unsigned char*)&input)[1];
		m_Internal[2] = (((unsigned char*)&input)[2] & 0x7F) | (input < 0 ? 0x80 : 0x00);

		return *this;
	}

	/***********************************************/

	int24_t operator +(const int24_t& val) const {
		return int24_t((int32_t)*this + (int32_t)val);
	}

	int24_t operator -(const int24_t& val) const {
		return int24_t((int32_t)*this - (int32_t)val);
	}

	int24_t operator *(const int24_t& val) const {
		return int24_t((int32_t)*this * (int32_t)val);
	}

	int24_t operator /(const int24_t& val) const {
		return int24_t((int32_t)*this / (int32_t)val);
	}

	/***********************************************/

	int24_t operator +(const int val) const {
		return int24_t((int32_t)*this + val);
	}

	int24_t operator -(const int val) const {
		return int24_t((int32_t)*this - val);
	}

	int24_t operator *(const int val) const {
		return int24_t((int32_t)*this * val);
	}

	int24_t operator /(const int val) const {
		return int24_t((int32_t)*this / val);
	}

	/***********************************************/
	/***********************************************/


	int24_t& operator +=(const int24_t& val) {
		*this = *this + val;
		return *this;
	}

	int24_t& operator -=(const int24_t& val) {
		*this = *this - val;
		return *this;
	}

	int24_t& operator *=(const int24_t& val) {
		*this = *this * val;
		return *this;
	}

	int24_t& operator /=(const int24_t& val) {
		*this = *this / val;
		return *this;
	}

	/***********************************************/

	int24_t& operator +=(const int val) {
		*this = *this + val;
		return *this;
	}

	int24_t& operator -=(const int val) {
		*this = *this - val;
		return *this;
	}

	int24_t& operator *=(const int val) {
		*this = *this * val;
		return *this;
	}

	int24_t& operator /=(const int val) {
		*this = *this / val;
		return *this;
	}

	/***********************************************/
	/***********************************************/

	int24_t operator >>(const int val) const {
		return int24_t((int32_t)*this >> val);
	}

	int24_t operator <<(const int val) const {
		return int24_t((int32_t)*this << val);
	}

	/***********************************************/

	int24_t& operator >>=(const int val) {
		*this = *this >> val;
		return *this;
	}

	int24_t& operator <<=(const int val) {
		*this = *this << val;
		return *this;
	}

	/***********************************************/
	/***********************************************/

	operator bool() const {
		return (int32_t)*this != 0;
	}

	bool operator !() const {
		return !((int32_t)*this);
	}

	int24_t operator -() {
		return int24_t(-(int32_t)*this);
	}

	/***********************************************/
	/***********************************************/

	bool operator ==(const int24_t& val) const {
		return (int32_t)*this == (int32_t)val;
	}

	bool operator !=(const int24_t& val) const {
		return (int32_t)*this != (int32_t)val;
	}

	bool operator >=(const int24_t& val) const {
		return (int32_t)*this >= (int32_t)val;
	}

	bool operator <=(const int24_t& val) const {
		return (int32_t)*this <= (int32_t)val;
	}

	bool operator >(const int24_t& val) const {
		return (int32_t)*this > (int32_t)val;
	}

	bool operator <(const int24_t& val) const {
		return (int32_t)*this < (int32_t)val;
	}

	/***********************************************/

	bool operator ==(const int val) const {
		return (int32_t)*this == val;
	}

	bool operator !=(const int val) const {
		return (int32_t)*this != val;
	}

	bool operator >=(const int val) const {
		return (int32_t)*this >= val;
	}

	bool operator <=(const int val) const {
		return (int32_t)*this <= val;
	}

	bool operator >(const int val) const {
		return ((int32_t)*this) > val;
	}

	bool operator <(const int val) const {
		return (int32_t)*this < val;
	}

	/***********************************************/
	/***********************************************/
};

#endif