#include "asn-stringconvert.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <vector>
#include <cassert>
#ifdef _WIN32
#include <Windows.h>
#else
#include <iconv.h>
#endif

#ifdef _DEBUG
#define FILLCHAR_W 65535
#define FILLCHAR_A (unsigned char)255
#else
#define FILLCHAR_W 0
#define FILLCHAR_A 0
#endif

#ifdef _WIN32
int getWindowsCodePage(const char* szCodePage)
{
	// https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
	if (strstr(szCodePage, "windows-") == szCodePage)
		return atoi(szCodePage + 8);
	else if (strstr(szCodePage, "iso-8859-") == szCodePage)
		return 28590 + atoi(szCodePage + 9);
	else
		return CP_ACP;
}

std::wstring convertCodePageToWidePlatform(const char* szCodePageString, const char* szCodePage)
{
	if (szCodePageString == NULL || *szCodePageString == '\0')
		return std::wstring();

	const int size = (int)strlen(szCodePageString);
	const int iCodePage = getWindowsCodePage(szCodePage);
	const int sizeNeeded = MultiByteToWideChar(iCodePage, 0, szCodePageString, size, NULL, 0);
	if (!sizeNeeded)
		return std::wstring();

	std::wstring strWide(sizeNeeded, FILLCHAR_W);
#ifdef _DEBUG
	const int iConverted =
#endif
		MultiByteToWideChar(iCodePage, 0, szCodePageString, size, &strWide[0], sizeNeeded);
#ifdef _DEBUG
	if (sizeNeeded != iConverted)
	{
		DWORD dwErr = GetLastError();
		printf("Error in AsnStringConvert::CodePageToWide: %ld", dwErr);
		assert(FALSE);
	}
#endif
	return strWide;
}

std::string convertWideToCodePagePlatform(const wchar_t* szWideString, const char* szCodePage)
{
	if (szWideString == NULL || *szWideString == L'\0')
		return std::string();

	const int size = (int)wcslen(szWideString);
	const int iCodePage = getWindowsCodePage(szCodePage);
	const int sizeNeeded = WideCharToMultiByte(iCodePage, 0, szWideString, size, NULL, 0, NULL, NULL);
	if (!sizeNeeded)
		return std::string();

	std::string strResult(sizeNeeded, FILLCHAR_A);
#ifdef _DEBUG
	const int iConverted =
#endif
		WideCharToMultiByte(iCodePage, 0, szWideString, size, &strResult[0], sizeNeeded, NULL, NULL);
#ifdef _DEBUG
	if (sizeNeeded != iConverted)
	{
		DWORD dwErr = GetLastError();
		printf("Error in AsnStringConvert::WideToCodePage: %ld", dwErr);
		assert(FALSE);
	}
#endif
	return strResult;
}

std::wstring convertUTF8ToWidePlatform(const char* szUTF8)
{
	if (szUTF8 == NULL || *szUTF8 == '\0')
		return std::wstring();

	const int size = (int)strlen(szUTF8);
	const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, szUTF8, size, NULL, 0);
	if (!sizeNeeded)
		return std::wstring();

	std::wstring strWide(sizeNeeded, FILLCHAR_W);
#ifdef _DEBUG
	const int iConverted =
#endif
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szUTF8, size, &strWide[0], sizeNeeded);
#ifdef _DEBUG
	if (sizeNeeded != iConverted)
	{
		DWORD dwErr = GetLastError();
		printf("Error in AsnStringConvert::UTF8ToWide: %ld", dwErr);
		assert(FALSE);
	}
#endif
	return strWide;
}

std::string convertWideToUTF8Platform(const wchar_t* szWideString)
{
	if (szWideString == NULL || *szWideString == L'\0')
		return std::string();

	const int size = (int)wcslen(szWideString);
	const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, szWideString, size, NULL, 0, NULL, NULL);
	if (!sizeNeeded)
		return std::string();

	std::string strUTF8(sizeNeeded, FILLCHAR_A);
#ifdef _DEBUG
	const int iConverted =
#endif
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, szWideString, size, &strUTF8[0], sizeNeeded, NULL, NULL);
#ifdef _DEBUG
	if (sizeNeeded != iConverted)
	{
		DWORD dwErr = GetLastError();
		printf("Error in AsnStringConvert::WideToUTF8: %ld", dwErr);
		assert(FALSE);
	}
#endif
	return strUTF8;
}
#else
const char* getCodePageName(const char* szCodePage)
{
	return (szCodePage && *szCodePage) ? szCodePage : "ISO-8859-1";
}

std::wstring convertCodePageToWidePlatform(const char* szCodePageString, const char* szCodePage)
{
	if (szCodePageString == NULL || *szCodePageString == '\0')
		return std::wstring();

	iconv_t hConverter = iconv_open("WCHAR_T", getCodePageName(szCodePage));
	if (hConverter == (iconv_t)-1)
		throw std::runtime_error("Unable to create iconv converter for source code page");

	size_t inBytesLeft = strlen(szCodePageString);
	char* pszInput = const_cast<char*>(szCodePageString);
	std::vector<wchar_t> strWide(std::max<size_t>(inBytesLeft + 1, 16), FILLCHAR_W);
	char* pszOutput = reinterpret_cast<char*>(strWide.data());
	size_t outBytesLeft = strWide.size() * sizeof(wchar_t);

	while (iconv(hConverter, &pszInput, &inBytesLeft, &pszOutput, &outBytesLeft) == (size_t)-1)
	{
		if (errno != E2BIG)
		{
			iconv_close(hConverter);
			throw std::runtime_error("Unable to convert codepage text to wide string");
		}

		const size_t outBytesUsed = (strWide.size() * sizeof(wchar_t)) - outBytesLeft;
		strWide.resize(strWide.size() * 2, FILLCHAR_W);
		pszOutput = reinterpret_cast<char*>(strWide.data()) + outBytesUsed;
		outBytesLeft = (strWide.size() * sizeof(wchar_t)) - outBytesUsed;
	}

	iconv_close(hConverter);
	const size_t outBytesWritten = (strWide.size() * sizeof(wchar_t)) - outBytesLeft;
	strWide.resize(outBytesWritten / sizeof(wchar_t));
	return std::wstring(strWide.begin(), strWide.end());
}

std::string convertWideToCodePagePlatform(const wchar_t* szWideString, const char* szCodePage)
{
	if (szWideString == NULL || *szWideString == L'\0')
		return std::string();

	iconv_t hConverter = iconv_open(getCodePageName(szCodePage), "WCHAR_T");
	if (hConverter == (iconv_t)-1)
		throw std::runtime_error("Unable to create iconv converter for target code page");

	size_t inBytesLeft = wcslen(szWideString) * sizeof(wchar_t);
	char* pszInput = const_cast<char*>(reinterpret_cast<const char*>(szWideString));
	std::vector<char> strResult(std::max<size_t>(inBytesLeft + 1, 16), FILLCHAR_A);
	char* pszOutput = strResult.data();
	size_t outBytesLeft = strResult.size();

	while (iconv(hConverter, &pszInput, &inBytesLeft, &pszOutput, &outBytesLeft) == (size_t)-1)
	{
		if (errno != E2BIG)
		{
			iconv_close(hConverter);
			throw std::runtime_error("Unable to convert wide string to codepage text");
		}

		const size_t outBytesUsed = strResult.size() - outBytesLeft;
		strResult.resize(strResult.size() * 2, FILLCHAR_A);
		pszOutput = strResult.data() + outBytesUsed;
		outBytesLeft = strResult.size() - outBytesUsed;
	}

	iconv_close(hConverter);
	strResult.resize(strResult.size() - outBytesLeft);
	return std::string(strResult.begin(), strResult.end());
}

std::wstring convertUTF8ToWidePlatform(const char* szUTF8)
{
	return convertCodePageToWidePlatform(szUTF8, "UTF-8");
}

std::string convertWideToUTF8Platform(const wchar_t* szWideString)
{
	return convertWideToCodePagePlatform(szWideString, "UTF-8");
}
#endif

std::string AsnStringConvert::AsciiToUTF8(const char* szASCII, const char* szCodePage /* = "ISO-8859-1" */)
{
	std::string strUTF8;
	try
	{
		// Convert through the platform wide-string form so codepage handling
		// stays aligned between Windows and Linux implementations.
		std::wstring strWide = AsnStringConvert::CodePageToWide(szASCII, szCodePage);
		strUTF8 = AsnStringConvert::WideToUTF8(strWide.c_str());
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strUTF8;
}

std::string AsnStringConvert::UTF8ToAscii(const char* szUTF8, const char* szCodePage /* = "ISO-8859-1" */)
{
	std::string strASCII;
	try
	{
		// Convert through the platform wide-string form so codepage handling
		// stays aligned between Windows and Linux implementations.
		std::wstring strWide = AsnStringConvert::UTF8ToWide(szUTF8);
		strASCII = AsnStringConvert::WideToCodePage(strWide.c_str(), szCodePage);
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strASCII;
}

std::wstring AsnStringConvert::CodePageToWide(const char* szCodePageString, const char* szCodePage /* = "ISO-8859-1" */)
{
	std::wstring strWide;
	try
	{
		if (szCodePageString == NULL || *szCodePageString == '\0')
			return strWide;
		strWide = convertCodePageToWidePlatform(szCodePageString, szCodePage);
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strWide;
}

std::string AsnStringConvert::WideToCodePage(const wchar_t* szWideString, const char* szCodePage /* = "ISO-8859-1" */)
{
	std::string strResult;
	try
	{
		if (szWideString == NULL || *szWideString == L'\0')
			return strResult;
		strResult = convertWideToCodePagePlatform(szWideString, szCodePage);
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strResult;
}

std::wstring AsnStringConvert::UTF8ToWide(const char* szUTF8)
{
	std::wstring strWide;
	try
	{
		if (szUTF8 == NULL || *szUTF8 == '\0')
			return strWide;
		strWide = convertUTF8ToWidePlatform(szUTF8);
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strWide;
}

std::string AsnStringConvert::WideToUTF8(const wchar_t* szWideString)
{
	std::string strUTF8;
	try
	{
		if (szWideString == NULL || *szWideString == L'\0')
			return strUTF8;
		strUTF8 = convertWideToUTF8Platform(szWideString);
	}
	catch (const std::exception& e)
	{
		printf("Exception: %s", e.what());
		assert(0);
	}
	return strUTF8;
}

