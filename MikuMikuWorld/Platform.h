#pragma once
#ifdef _WIN32

#define MMW_WINDOWS
#include <Windows.h>
#include <cstdio>
#include <string>
#include <vector>
#elif defined(__APPLE__)

#define MMW_MACOS
#import <string>
#import <vector>
#elif defined(__unix__) || defined(__unix)

#define MMW_LINUX
#include "unistd.h"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#endif

namespace IO {
	enum class FileDialogResult : uint8_t;
	enum class DialogType : uint8_t;
	enum class DialogSelectType : uint8_t;
	struct FileDialogFilter;
	class FileDialog;

	enum class MessageBoxButtons : uint8_t;
	enum class MessageBoxIcon : uint8_t;
	enum class MessageBoxResult : uint8_t;
}

namespace Platform {
	void OpenUrl(const std::string& url);
	IO::FileDialogResult OpenFileDialog(IO::DialogType type, IO::DialogSelectType selectType, IO::FileDialog& dialogOptions);
	IO::MessageBoxResult OpenMessageBox(const std::string& title, const std::string& message, IO::MessageBoxButtons buttons, IO::MessageBoxIcon icon, void* parentWindow = nullptr);
	std::string GetCurrentLanguageCode();
	std::string GetBuildVersion();
	FILE* OpenFile(const std::string& filename, const std::string& mode);

	std::vector<std::string> GetCommandLineArgs();
	std::string GetConfigPath(const std::string& app_root);
	std::string GetResourcePath(const std::string& app_root);

	namespace Bit {
		static inline bool IsLittleEndian() {
			#ifdef MMW_WINDOWS
			return true;
			#else
			return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
			#endif
		}
		static inline uint16_t ByteSwap16(uint16_t x)
		{
			#ifdef MMW_WINDOWS
			return _byteswap_ushort(x);
			#else
			return __builtin_bswap16(x);
			#endif
		}
		static inline uint32_t ByteSwap32(uint32_t x)
		{
			#ifdef MMW_WINDOWS
			return _byteswap_ulong(x);
			#else
			return __builtin_bswap32(x);;
			#endif
		}
		static inline uint64_t ByteSwap64(uint64_t x)
		{
			#ifdef MMW_WINDOWS
			return _byteswap_uint64(x);
			#else
			return __builtin_bswap64(x);;
			#endif
		}
		static inline float ByteSwapf32(float f)
		{
			static_assert(sizeof(float) == sizeof(uint32_t), "Unexpected float format");
			uint32_t asInt;
			std::memcpy(&asInt, reinterpret_cast<const void *>(&f), sizeof(uint32_t));
			asInt = ByteSwap32(asInt);
			std::memcpy(&f, reinterpret_cast<void *>(&asInt), sizeof(float));
			return f;
		}
	}
}