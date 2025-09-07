#include "Sonolus.h"
#include "SHA-1.hpp"

namespace Sonolus
{
	int skinVersion = 4;
	int backgroundVersion = 2;
	int effectVersion = 5;
	int particleVersion = 3;
	int engineVersion = 13;
	int levelVersion = 1;

	std::string hash(const std::vector<uint8_t>& data)
	{
		IO::SHA1 sha1;
		sha1.update(data);
		return sha1.final();
	}
}