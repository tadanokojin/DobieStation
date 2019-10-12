#ifndef __BIOS_HPP__
#define __BIOS_HPP__

#include <vector>
#include <cstdint>

class BiosReader
{
private:
	static constexpr int BIOS_SIZE = 1024 * 1024 * 4;
	static constexpr int ROMDIR_ENTRY_SIZE = 16;
	static constexpr long NOMATCH = -1;

	std::vector<uint8_t> m_data;

	long match(const uint8_t* match_array, size_t match_len) const;
	long match(const char* match_str) const;

public:
	bool open(const char* path);
	[[nodiscard]] inline const uint8_t* data() const { return m_data.data(); }
};

#endif//__BIOS_HPP__
