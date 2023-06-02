#ifndef ASN_STRINGCONVERT_H
#define ASN_STRINGCONVERT_H

#include <string>

class AsnStringConvert {
public:
	// Converts an ASCII buffer into an UTF8 string
	static std::string AsciiToUTF8(const char* szASCII, const char* szCodePage = "ISO-8859-1");
	// Converts an UTF8 string into an ASCII string
	static std::string UTF8ToAscii(const char* szUTF8, const char* szCodePage = "ISO-8859-1");

	// Converts an ASCII buffer into an UTF16 (Windows Unicode) string
	static std::wstring AsciiToUTF16(const char* szASCII, const char* szCodePage = "ISO-8859-1");
	// Converts an UTF16 string into an ASCII string
	static std::string UTF16ToAscii(const wchar_t* szUTF16, const char* szCodePage = "ISO-8859-1");

	// Converts an UTF8 string into an UTF16 (Windows Unicode) string
	static std::wstring UTF8ToUTF16(const char* szUTF8);
	// Converts an UTF16 string into an UTF8 string
	static std::string UTF16ToUTF8(const wchar_t* szUTF16);
};

#endif // ASN_STRINGCONVERT_H