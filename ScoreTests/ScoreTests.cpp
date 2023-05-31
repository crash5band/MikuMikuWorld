#include "pch.h"
#include "CppUnitTest.h"
#include "Score.h"
#include "Tempo.h"
#include "Constants.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace mmw = MikuMikuWorld;

namespace ScoreTests
{
	TEST_CLASS(ScoreTests)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			std::vector<mmw::Tempo> tempos{{ 0, 120 }, { 1920, 160 }, { 2400, 180 }};
			const mmw::Tempo& target = mmw::getTempoAt(1920, tempos);

			Assert::AreSame(tempos[1], target);
		}
	};
}
