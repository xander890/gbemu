#include "util.h"
#include<fstream>

void LoadBinaryData(const std::string& file, std::vector<uint8>& dest)
{
	std::ifstream ifs(file, std::ios::binary | std::ios::ate);

	if (!ifs)
		throw std::runtime_error(file + ": " + std::strerror(errno));

	auto end = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	size_t size = std::size_t(end - ifs.tellg());

	if (size == 0) // avoid undefined behavior 
		return;

	dest.resize(size);

	if (!ifs.read((char*)dest.data(), dest.size()))
		throw std::runtime_error(file + ": " + std::strerror(errno));
}
