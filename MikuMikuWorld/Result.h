#pragma once
#include <string>

namespace MikuMikuWorld
{
	enum class ResultStatus
	{
		Success,
		Warning,
		Error
	};

	class Result
	{
	private:
		ResultStatus status;
		std::string message;

	public:
		Result(ResultStatus _status, std::string _msg = "")
			: status{ _status }, message{ _msg }
		{

		}

		ResultStatus getStatus() const
		{
			return status;
		}

		std::string getMessage() const
		{
			return message;
		}

		static Result Ok()
		{
			return Result(ResultStatus::Success);
		}
	};
}