#ifndef __FINPUT_HPP
#define __FINPUT_HPP

#include <istream>
#include <fstream>
#include "imstream.hpp"

namespace FInput {
	std::istream* File(const char *filename) {
		return new std::ifstream(filename, std::ios::in | std::ios::binary);
	}

	std::istream* File(const wchar_t *filename) {
		return new std::ifstream(filename, std::ios::in | std::ios::binary);
	}

	std::istream* Memory(const void *data, size_t size) {
		return new imstream(data, size);
	}
}

#endif