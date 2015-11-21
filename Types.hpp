#ifndef __TYPES_HPP
#define __TYPES_HPP

namespace Types {
	using second_t = double;					// 秒を表す型、実数　　　本当はunsigned
	using msec_t = unsigned long long;			// ミリ秒を表す型、整数
	using sample_t = unsigned long long;		// サンプル数を表す型
	using byte_t = unsigned long long;			// バイト数を表す型
	//using byte_t = size_t;
}

#endif