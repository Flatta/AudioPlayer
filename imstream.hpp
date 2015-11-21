#ifndef __IMSTREAM_HPP
#define __IMSTREAM_HPP

#include <sstream>

// may not be safe?
// istringstream‚Å‚Í‚È‚­stringstream‚ğŒp³‚µ‚Ä©g‚Ìwrite‚ğŒÄ‚Ño‚·‚Ì‚ªÅ‚àˆÀ‘S‚»‚¤‚¾‚¯‚Ç
struct imstream : public std::istringstream {
	std::stringstream ss;

	imstream(const void* data, size_t size) {
		ss.write((char *)data, size);
		set_rdbuf(ss.rdbuf());
		seekg(0);
	}
};

#endif