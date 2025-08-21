#include "Score.h"
#include "File.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "IO.h"
#include "Constants.h"
#include <unordered_set>

using namespace IO;

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
