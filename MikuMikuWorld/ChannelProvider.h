#pragma once
#include <unordered_map>

class ChannelProvider
{
private:
	struct TickRange { int start; int end; };
	std::unordered_map<int, TickRange> channels;

public:
	ChannelProvider()
	{
		for (int i = 0; i < 36; ++i)
			channels[i] = TickRange{ 0, 0 };
	}

	int generateChannel(int startTick, int endTick)
	{
		for (auto& it : channels)
		{
			int start = it.second.start;
			int end = it.second.end;
			if ((start == 0 && end == 0) || endTick < start || startTick > end)
			{
				channels[it.first] = TickRange{ startTick, endTick };
				return it.first;
			}
		}

		throw("No more channels available");
	}
};
