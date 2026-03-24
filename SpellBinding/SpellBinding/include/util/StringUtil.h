#pragma once

// Utilities for string conversion and UTF-8 cleanup.
namespace UTIL::STRING {

	static inline std::string WideToUTF8(const std::wstring& wstr)
	{
		if (wstr.empty()) {
			return {};
		}

		std::int32_t sizeRequired = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
		if (sizeRequired <= 0) {
			return {};
		}

		std::string result(sizeRequired, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, result.data(), sizeRequired, nullptr, nullptr);

		result.pop_back();
		return result;
	}

	static inline std::string SanitizeUtf8(const std::string& input)
	{
		if (input.empty()) {
			return {};
		}

		const auto* data = input.c_str();
		const auto size = static_cast<int>(input.size());

		// Fast path: already valid UTF-8.
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, size, nullptr, 0) > 0) {
			return input;
		}

		// Try converting from the active ANSI codepage. Skyrim strings are often
		// emitted in the system codepage rather than UTF-8.
		const int wideLen = MultiByteToWideChar(CP_ACP, 0, data, size, nullptr, 0);
		if (wideLen > 0) {
			std::wstring wide(static_cast<std::size_t>(wideLen), L'\0');
			MultiByteToWideChar(CP_ACP, 0, data, size, wide.data(), wideLen);
			const int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLen, nullptr, 0, nullptr, nullptr);
			if (utf8Len > 0) {
				std::string result(static_cast<std::size_t>(utf8Len), '\0');
				WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLen, result.data(), utf8Len, nullptr, nullptr);
				if (!result.empty() && result.back() == '\0') {
					result.pop_back();
				}
				return result;
			}
		}

		// Last resort: strip obvious control bytes and preserve ASCII.
		std::string result;
		result.reserve(input.size());
		for (unsigned char ch : input) {
			if (ch < 0x80) {
				result.push_back(static_cast<char>(ch));
			} else {
				result.append("\xEF\xBF\xBD");
			}
		}
		return result;
	}
}
