#include "bios.hpp"
#include <fstream>
#include <algorithm>
#include <functional>
#include <cstring>
#include <cstdio>


long BiosReader::match(const uint8_t* match_array, size_t match_len) const
{
    auto res = std::search(m_data.begin(), m_data.end(),
        std::boyer_moore_searcher(match_array, match_array + match_len));

    return res != m_data.end() ? res - m_data.begin() : NOMATCH;
}

long BiosReader::match(const char* match_str) const
{
    return match((const uint8_t*)match_str, strlen(match_str));
}

bool BiosReader::open(const char* path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    	return false;

    auto size = file.tellg();
    if (size < 0)
    	return false;
    file.seekg(0, std::ios::beg);

    m_data.resize(BIOS_SIZE);
    file.read((char*)m_data.data(), std::min<std::streamsize>(size, BIOS_SIZE));

    printf("%li\n", match("KERNEL"));
    printf("%li\n", match("ROMDIR"));
    printf("%li\n", match("RESET"));
    printf("%li\n", match("ROMVER"));

	return false;
}
