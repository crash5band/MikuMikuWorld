#include "NoteSkin.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld
{
	NoteSkinList noteSkins{};

	void NoteSkinList::add(std::string name, int notes, int longNote, int touchLine)
	{
		skins.push_back({ name, notes, longNote, touchLine });
		skinNames.push_back(name);
	}

	void NoteSkinList::remove(size_t index)
	{
		if (index >= skins.size())
			return;

		skins.erase(skins.cbegin() + index);
		skinNames.erase(skinNames.cbegin() + index);
	}

	int NoteSkinList::getItemIndex(NoteSkinItem item) const
	{
		size_t activeSkinIndex = config.notesSkin;
		if (!isArrayIndexInBounds(activeSkinIndex, skins))
			return -1;

		switch (item)
		{
		case NoteSkinItem::Notes:
			return skins[activeSkinIndex].notes;
		case NoteSkinItem::LongNote:
			return skins[activeSkinIndex].holdPath;
		case NoteSkinItem::TouchLine:
			return skins[activeSkinIndex].touchLine;
		default:
			return -1;
		}
	}

	size_t NoteSkinList::count() const
	{
		return skins.size();
	}

	const std::vector<std::string>& NoteSkinList::getSkinNames() const
	{
		return skinNames;
	}
}