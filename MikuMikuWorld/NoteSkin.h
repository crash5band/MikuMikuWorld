#pragma once
#include <vector>
#include <string>

namespace MikuMikuWorld
{
	enum class NoteSkinItem
	{
		Notes,
		LongNote,
		TouchLine
	};

	class NoteSkinList
	{
	public:
		size_t count() const;
		const std::vector<std::string>& getSkinNames() const;

		int getItemIndex(NoteSkinItem item) const;
		void add(std::string name, int notes, int longNote, int touchLine);
		void remove(size_t index);

	private:
		/// <summary>
		/// Contains the indexes of the notes textures loaded with <see cref="ResourceManager"/>
		/// </summary>
		struct NoteSkin
		{
			std::string name;
			int notes{ -1 };
			int holdPath{ -1 };
			int touchLine{ -1 };
		};

		std::vector<NoteSkin> skins;
		std::vector<std::string> skinNames;
	};

	extern NoteSkinList noteSkins;
}