#include "../include/asn-stringconvert.h"
#include <locale>
#include <codecvt>
#include <utility>
#include <cassert>
#ifdef _WIN32
    #include <Windows.h>
#endif

template<class Facet>
struct deletable_facet : Facet
{
    template<class... Args>
    deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};

#ifdef _DEBUG
    #define FILLCHAR_W 65535
    #define FILLCHAR_A (unsigned char)255
#else
    #define FILLCHAR_W 0
    #define FILLCHAR_A 0
#endif

#ifdef _WIN32
    int getWindowsCodePage(const char* szCodePage) {
        // https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
        if(strstr(szCodePage, "windows-") == szCodePage)
            return atoi(szCodePage + 8);
        else if(strstr(szCodePage, "iso-8859-") == szCodePage)
            return 28590 + atoi(szCodePage + 9);
        else
            return CP_ACP;
    }
#endif

std::string AsnStringConvert::AsciiToUTF8(const char* szASCII, const char* szCodePage /* = "ISO-8859-1" */) {
    std::string strUTF8;
    try
    {
        #ifdef _WIN32
            // 1. Convert to Unicode
            std::wstring strUTF16 = AsnStringConvert::AsciiToUTF16(szASCII, szCodePage);
            // 2. Convert to ASCII with codepage
            strUTF8 = AsnStringConvert::UTF16ToUTF8(strUTF16.c_str());
        #else
            std::wstring_convert<deletable_facet<std::codecvt_utf8<wchar_t>>> converter;
            std::wstring utf16_str = converter.from_bytes(szASCII);
            strUTF8 = converter.to_bytes(utf16_str);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strUTF8;
}

std::string AsnStringConvert::UTF8ToAscii(const char* szUTF8, const char* szCodePage /* = "ISO-8859-1" */) {
    std::string strASCII;
    try
    {
        #ifdef _WIN32
            // 1. Convert to Unicode
            std::wstring strUTF16 = AsnStringConvert::UTF8ToUTF16(szUTF8);
            // 2. Convert to ASCII with codepage
            strASCII = AsnStringConvert::UTF16ToAscii(strUTF16.c_str(), szCodePage);
        #else
            std::wstring_convert<deletable_facet<std::codecvt_utf8<wchar_t>>> converter;
            std::wstring utf16_str = converter.from_bytes(szUTF8);
            strASCII = converter.to_bytes(utf16_str);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strASCII;
}


std::wstring AsnStringConvert::AsciiToUTF16(const char* szASCII, const char* szCodePage /* = "ISO-8859-1" */) {
    std::wstring strUTF16;
    try
    {
        #ifdef _WIN32
            int size = (int)strlen(szASCII);
            if (size) {
                int iCodePage = getWindowsCodePage(szCodePage);
                int size_needed = MultiByteToWideChar(iCodePage, 0, szASCII, size, NULL, 0);
                if (size_needed) {
                    strUTF16.resize(size_needed, FILLCHAR_W);
                    #ifdef _DEBUG
                        int iConverted =
                    #endif
                    MultiByteToWideChar(iCodePage, 0, szASCII, size, &strUTF16[0], size_needed);
                    #ifdef _DEBUG
                        if (size_needed != iConverted) {
                            DWORD dwErr = GetLastError();
                            dwErr;
                            assert(FALSE);
                        }
                    #endif
                }
            }
        #else
            std::wstring_convert<deletable_facet<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>>> converter;
            strUTF16 = converter.from_bytes(szASCII);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strUTF16;
}

std::string AsnStringConvert::UTF16ToAscii(const wchar_t* szUTF16, const char* szCodePage /* = "ISO-8859-1" */) {
    std::string strASCII;
    try
    {
        #ifdef _WIN32
            int size = (int)wcslen(szUTF16);
            if (size) {
            int iCodePage = getWindowsCodePage(szCodePage);
                int size_needed = WideCharToMultiByte(iCodePage, 0, szUTF16, size, NULL, 0, NULL, NULL);
            if(size_needed) {
                strASCII.resize(size_needed, FILLCHAR_A);
                #ifdef _DEBUG
                    int iConverted =
                #endif
                WideCharToMultiByte(iCodePage, 0, szUTF16, size, &strASCII[0], size_needed, NULL, NULL);
                #ifdef _DEBUG
                    if (size_needed != iConverted) {
                        DWORD dwErr = GetLastError();
                        dwErr;
                        assert(FALSE);
                    }
                #endif
                }
            }
        #else
            std::wstring_convert<deletable_facet<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>>> converter;
            strASCII = converter.to_bytes(szUTF16);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strASCII;
}

std::wstring AsnStringConvert::UTF8ToUTF16(const char* szUTF8) {
    std::wstring strUTF16;
    try
    {
        #ifdef _WIN32
            int size = (int)strlen(szUTF8);
            if (size) {
                int size_needed = MultiByteToWideChar(CP_UTF8, 0, szUTF8, size, NULL, 0);
            if (size_needed) {
                strUTF16.resize(size_needed, FILLCHAR_W);
                #ifdef _DEBUG
                    int iConverted =
                #endif
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szUTF8, size, &strUTF16[0], size_needed);
                #ifdef _DEBUG
                    if (size_needed != iConverted) {
                        DWORD dwErr = GetLastError();
                        dwErr;
                        assert(FALSE);
                    }
                #endif
                }
            }
        #else
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            strUTF16 = converter.from_bytes(szUTF8);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strUTF16;
}

std::string AsnStringConvert::UTF16ToUTF8(const wchar_t* szUTF16) {
    std::string strUTF8;
    try
    {
        #ifdef _WIN32
            int size = (int)wcslen(szUTF16);
            if (size) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, szUTF16, size, NULL, 0, NULL, NULL);
            if (size_needed) {
                strUTF8.resize(size_needed, FILLCHAR_A);
                #ifdef _DEBUG
                    int iConverted =
                #endif
                WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, szUTF16, size, &strUTF8[0], size_needed, NULL, NULL);
                #ifdef _DEBUG
                    if (size_needed != iConverted) {
                        DWORD dwErr = GetLastError();
                        dwErr;
                        assert(FALSE);
                    }
                #endif
                }
            }
        #else
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            strUTF8 = converter.to_bytes(szUTF16);
        #endif
    }
    catch(const std::exception &e)
    {
        printf("Exception: %s", e.what());
        assert(0);
    }
    return strUTF8;
}

