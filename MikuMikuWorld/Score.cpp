#include "Score.h"

namespace MikuMikuWorld
{
	Score::Score()
	{
		metadata.title = "";
		metadata.author = "";
		metadata.artist = "";
		metadata.musicOffset = 0;

		tempoChanges.push_back(Tempo());
		timeSignatures[0] = { 0, 4, 4 };

		fever.startTick = fever.endTick = -1;
	}
}
