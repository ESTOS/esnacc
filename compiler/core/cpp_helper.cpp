#include "cpp_helper.h"
#include "cpp_c_helper.h"
#include "../../c-lib/include/asn-incl.h"
#include <filesystem>
#include <string>
#include <cstring> // -> MingW, Clang, GCC
#include <regex>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
using namespace std;

CASN1Files fileCache;
CASN1FileLoopIDs fileCacheLoopIDs;

enum class ECOMPAREOPERATOR
{
	UNKNOWN = 0,
	EQUAL = 1,
	LARGER_THAN = 2,
	LOWER_THAN = 3,
	LARGER_OR_EQUAL = 4,
	LOWER_OR_EQUAL = 5
};

enum class EADDLINE
{
	NOCHANGE = 0,
	SET = 1,
	RESET = 2
};

ECOMPAREOPERATOR getCompareOperator(const std::string& input, int& nVersion, const char* szLine)
{
	std::string compare;
	nVersion = 0;

	std::string strVersion;

	for (const auto element : input)
		if (element == '>' || element == '<' || element == '=')
			compare += element;
		else
			strVersion += element;

	nVersion = atoi(strVersion.c_str());

	if (!nVersion)
		fprintf(stderr, "Unkown version '%s' in line '%s'", strVersion.c_str(), szLine);

	ECOMPAREOPERATOR op = ECOMPAREOPERATOR::UNKNOWN;

	if (compare == "==")
		op = ECOMPAREOPERATOR::EQUAL;
	else if (compare == "<")
		op = ECOMPAREOPERATOR::LOWER_THAN;
	else if (compare == ">")
		op = ECOMPAREOPERATOR::LARGER_THAN;
	else if (compare == ">=" || compare == "=>")
		op = ECOMPAREOPERATOR::LOWER_OR_EQUAL;
	else if (compare == "<=" || compare == "=<")
		op = ECOMPAREOPERATOR::LOWER_OR_EQUAL;
	else
		fprintf(stderr, "Unkown compare operator '%s' in line '%s'", compare.c_str(), szLine);

	return op;
}

std::string trimAndUppercase(const std::string& input)
{
	std::string output;
	for (const auto element : input)
	{
		if (std::isspace(element))
			continue;
		output += (char)std::toupper((int)element);
	}
	return output;
}

std::vector<std::string> splitByNewline(const char* buffer, size_t length)
{
	std::string strBuffer(buffer, length);
	std::istringstream stream(strBuffer);
	std::vector<std::string> lines;
	std::string line;

	// Use std::getline to split the string by '\r\n'
	while (std::getline(stream, line, '\n'))
	{
		// Remove '\r' if it's there
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		lines.push_back(line);
	}

	return lines;
}

int CPPHelper::FindFiles(const char* szPattern, const bool bIsImportedFile)
{
	// Possible values for szPattern;
	// somefile.asn1 - dedicated file
	// *.asn1 - a wildcard which resolves to a list of files internally (either windows or linux without global expansion in the shell set -f)
	// a path in which we search for *asn1 files... e.g. . /folder

	fs::path path;
	if (strlen(szPattern) == 0 || strcmp(szPattern, ".") == 0)
	{
		path = fs::current_path();
		path += fs::path::preferred_separator;
	}
	else
	{
		path = fs::absolute(szPattern);
		path.make_preferred();
	}

	auto strPattern = path.filename().string();
	if (strPattern.empty())
		strPattern = "*.asn1";

	int iCount = 0;
	if (strPattern.find("*") == std::string::npos)
	{
		// If there is no wildcard involved...
		if (AddToCache(path, bIsImportedFile))
			iCount++;
	}
	else
	{
		std::string regex = wildcardToRegex(strPattern.c_str());
		std::regex pattern(regex);

		const auto& parentPath = path.parent_path();
		for (auto& file : fs::directory_iterator(parentPath))
		{
			if (!file.is_regular_file())
				continue;

			const auto& filePath = file.path();
			const auto& strFileName = filePath.filename().string();

			if (!std::regex_match(strFileName, pattern))
				continue;

			if (AddToCache(filePath, bIsImportedFile))
				iCount++;
		}
	}

	return iCount;
}

bool CPPHelper::AddToCache(const fs::path& file, const bool bIsImportedFile)
{
	if (!fs::exists(file))
		return false;
	const auto& strFileName = file.filename().string();
	const auto iter = fileCache.find(strFileName);
	if (iter != fileCache.end())
	{
		// Was already in the list, check if the bIsImporteFile will now change...
		// if the file was so far only in the list as ImportedFile and is now a non import (a file to actually write output for) -> change the flag to non imported
		if (!bIsImportedFile && iter->second.m_bIsImportedFile)
		{
			iter->second.m_bIsImportedFile = false;
			return true;
		}
	}
	else
	{
		CASN1File asn1file;
		asn1file.m_strFileName = strFileName;
		asn1file.m_strFilePath = file.string();
		asn1file.m_bIsImportedFile = bIsImportedFile;
		fileCache[asn1file.m_strFileName] = asn1file;
		return true;
	}
	return false;
}

int CPPHelper::GetFileCountToProcess()
{
	int iCount = 0;
	for (const auto& element : fileCache)
		if (!element.second.m_bIsImportedFile)
			iCount++;
	return iCount;
}

bool CPPHelper::GetNextFile(SASN1File* file, int loopID)
{
	CASN1Files::const_iterator iterFile;
	auto iterLoopIDs = fileCacheLoopIDs.find(loopID);
	if (iterLoopIDs == fileCacheLoopIDs.end())
	{
		iterFile = fileCache.begin();
		fileCacheLoopIDs[loopID] = iterFile;
	}
	else
	{
		iterLoopIDs->second++;
		iterFile = iterLoopIDs->second;
	}
	if (iterFile == fileCache.end())
		return false;

	const auto& asn1File = iterFile->second;
	file->fileName = asn1File.m_strFileName.c_str();
	file->filePath = asn1File.m_strFilePath.c_str();
	file->bIsImportedFile = asn1File.m_bIsImportedFile;
	return true;
}

std::string CPPHelper::wildcardToRegex(const std::string& wildcard)
{
	std::string regex;
	for (char ch : wildcard)
	{
		switch (ch)
		{
			case '*':
				regex += ".*";
				break;
			case '?':
				regex += ".";
				break;
			case '.':
				regex += "\\.";
				break;
			case '\\':
				regex += "\\\\";
				break;
			default:
				regex += ch;
				break;
		}
	}
	return regex;
}

bool CPPHelper::GetDirectoryWithDelimiterFromPath(char* szPath, unsigned long ulLen)
{
	fs::path path = szPath;

	// Replace non platform conforming delimiters
	path.make_preferred();

	auto fullPath = path.string();
	if (fullPath.empty())
		return false;

	if (fullPath.back() != fs::path::preferred_separator)
		fullPath += fs::path::preferred_separator;

	if (fullPath.length() > ulLen)
		return false;

#if _WIN32
	strcpy_s(szPath, ulLen, fullPath.c_str());
#else
	strcpy(szPath, fullPath.c_str());
#endif

	return true;
}

bool CPPHelper::GetWorkingDirectoryWithDelimiter(char* szPath, unsigned long ulLen)
{
	auto path = fs::current_path();
	path += fs::path::preferred_separator;
	auto strPath = path.string();
	if (strPath.length() > ulLen)
		return false;

#if _WIN32
	strcpy_s(szPath, ulLen, strPath.c_str());
#else
	strcpy(szPath, strPath.c_str());
#endif

	return true;
}

bool CPPHelper::CreateDirectories(const char* szPath)
{
	for (int iCount = 1; iCount < 11; iCount++)
	{
		try
		{
			fs::path directoryPath = szPath;

			auto last = directoryPath.generic_string().back();
			if (last == '/' || last == '\\')
				directoryPath = directoryPath.parent_path();

			auto printablePath = directoryPath.generic_string();
			fs::file_status status = fs::status(directoryPath);
			if (fs::exists(status))
				return true;
			bool bCreated = fs::create_directories(directoryPath);
			if (bCreated)
				return true;
			else if (iCount == 10)
				printf("Creating folder %s finally failed after %i retires", printablePath.c_str(), iCount);
			else
				printf("Creating folder %s failed for the %i. time", printablePath.c_str(), iCount);
		}
		catch (...)
		{
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

bool CPPHelper::WriteFile(const char* buffer, const unsigned long ulSize, const char* szFileName, const bool bWriteESM, const int nNodeVersion)
{
	FILE* file = 0;
	if (fopen_s(&file, szFileName, "wb") != 0 || file == 0)
	{
		fprintf(stderr, "Could not open file %s for writing", szFileName);
		exit(1);
	}

	auto szCheck = szFileName + strlen(szFileName) - 3;

	if (strcmp(szCheck, ".ts") == 0)
	{
		auto strLines = splitByNewline(buffer, ulSize);
		bool bAddLines = true;
		for (auto strLine : strLines)
		{
			EADDLINE eAddlines = EADDLINE::NOCHANGE;
			if (bWriteESM)
			{
				auto posBegin = strLine.find("from \".");
				if (posBegin != std::string::npos)
				{
					posBegin += 7;
					// Special syntax for esm modules....
					auto posEnd = strLine.find("\"", posBegin);
					if (posEnd != std::string::npos)
						strLine.insert(posEnd, ".js");
				}
			}
			if (strLine.substr(0, 2) == "//")
			{
				std::string strCopy = trimAndUppercase(strLine.substr(2));

				if (strCopy.substr(0, 6) == "IFNODE")
				{
					strCopy = strCopy.substr(6);
					int nVersion;
					const auto op = getCompareOperator(strCopy.c_str(), nVersion, strLine.c_str());
					if (op == ECOMPAREOPERATOR::LOWER_THAN && nNodeVersion < nVersion)
						eAddlines = EADDLINE::SET;
					else if (op == ECOMPAREOPERATOR::LOWER_OR_EQUAL && nNodeVersion <= nVersion)
						eAddlines = EADDLINE::SET;
					else if (op == ECOMPAREOPERATOR::LARGER_THAN && nNodeVersion > nVersion)
						eAddlines = EADDLINE::SET;
					else if (op == ECOMPAREOPERATOR::LARGER_OR_EQUAL && nNodeVersion >= nVersion)
						eAddlines = EADDLINE::SET;
					else if (op == ECOMPAREOPERATOR::EQUAL && nNodeVersion == nVersion)
						eAddlines = EADDLINE::SET;
					bAddLines = false;
				}
				else if (strCopy.substr(0, 4) == "ELSE")
				{
					if (bAddLines)
						bAddLines = false;
					else
						eAddlines = EADDLINE::SET;
				}
				else if (strCopy.substr(0, 5) == "ENDIF")
				{
					if (bAddLines)
						bAddLines = false;
					eAddlines = EADDLINE::SET;
				}
			}

			if (bAddLines)
			{
				fwrite(strLine.c_str(), 1, strLine.length(), file);
#ifdef _WIN32
				fwrite("\r\n", 1, 2, file);
#else
				fwrite("\n", 1, 1, file);
#endif
			}
			if (eAddlines == EADDLINE::SET)
				bAddLines = true;
			else if (eAddlines == EADDLINE::RESET)
				bAddLines = false;
		}
	}
	else
	{
		fwrite(buffer, 1, ulSize, file);
	}

	fclose(file);

	return true;
}