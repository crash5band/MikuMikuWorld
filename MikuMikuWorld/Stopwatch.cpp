#include "Stopwatch.h"

namespace MikuMikuWorld
{
	using clock_type = std::chrono::steady_clock;
	using second_type = std::chrono::duration<double, std::ratio<1> >;

	void Stopwatch::reset()
	{
		begin = clock_type::now();
	}

	double Stopwatch::elapsed() const
	{
		return std::chrono::duration_cast<second_type>(clock_type::now() - begin).count();
	}

	int Stopwatch::elapsedMinutes() const
	{
		return elapsed() / 60;
	}
}