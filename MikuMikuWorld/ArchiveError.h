#pragma once
#include "zip.h"
#include <string>

namespace IO
{
	class ArchiveError
	{
	public:
		int get() const;
		std::string getStr() const;
		void set(int code);
		void set(zip_error_t* err);

		ArchiveError();
		~ArchiveError();
	private:
		
		bool shouldCleanError;
		zip_error_t* err;
	};
}