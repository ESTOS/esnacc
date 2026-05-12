#ifndef ASN_STRINGCONVERT_H
#define ASN_STRINGCONVERT_H

#include <string>

class AsnStringConvert
{
public:
	// Converts an ASCII buffer into an UTF8 string
	static std::string AsciiToUTF8(const char* szASCII, const char* szCodePage = "ISO-8859-1");
	// Converts an UTF8 string into an ASCII string
	static std::string UTF8ToAscii(const char* szUTF8, const char* szCodePage = "ISO-8859-1");

	// Converts a codepage-encoded narrow string into the platform wide-string representation.
	static std::wstring CodePageToWide(const char* szCodePageString, const char* szCodePage = "ISO-8859-1");
	// Converts the platform wide-string representation into a codepage-encoded narrow string.
	static std::string WideToCodePage(const wchar_t* szWideString, const char* szCodePage = "ISO-8859-1");

	// Converts an UTF8 string into the platform wide-string representation.
	static std::wstring UTF8ToWide(const char* szUTF8);
	// Converts the platform wide-string representation into an UTF8 string.
	static std::string WideToUTF8(const wchar_t* szWideString);
};

#endif // ASN_STRINGCONVERT_H