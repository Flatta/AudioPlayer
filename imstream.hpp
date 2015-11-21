#ifndef __IMSTREAM_HPP
#define __IMSTREAM_HPP

#include <sstream>

// may not be safe?
// istringstream�ł͂Ȃ�stringstream���p�����Ď��g��write���Ăяo���̂��ł����S����������
struct imstream : public std::istringstream {
	std::stringstream ss;

	imstream(const void* data, size_t size) {
		ss.write((char *)data, size);
		set_rdbuf(ss.rdbuf());
		seekg(0);
	}
};

#endif