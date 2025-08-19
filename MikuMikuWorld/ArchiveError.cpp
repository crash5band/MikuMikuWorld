#include "ArchiveError.h"

namespace IO
{
	ArchiveError::ArchiveError() : err(NULL), shouldCleanError(false) { }

	ArchiveError::~ArchiveError()
	{
		if (shouldCleanError && err != NULL)
		{
			zip_error_fini(err);
			delete err;
			err = NULL;
		}
	}

	int ArchiveError::get() const
	{
		if (err == NULL) return ZIP_ER_OK;
		return zip_error_code_zip(err);
	}

	std::string ArchiveError::getStr() const
	{
		if (err == NULL) return {};
		return zip_error_strerror(err);
	}

	void ArchiveError::set(int code)
	{
		if (!shouldCleanError) err = nullptr;
		if (err == NULL)
			err = new zip_error_t;
		else
			zip_error_fini(err);
		zip_error_init_with_code(err, code);
		shouldCleanError = true;
	}

	void ArchiveError::set(zip_error_t* ext_err)
	{
		if (shouldCleanError && err != NULL)
		{
			zip_error_fini(err);
			delete err;
		}
		err = ext_err;
		shouldCleanError = false;
	}
}

