#include "cpp_c_helper.h"
#include "cpp_helper.h"

extern "C"
{
	int findFiles(const char* szPattern, const bool bIsImporteFile)
	{
		return CPPHelper::FindFiles(szPattern, bIsImporteFile);
	}

	int getFileCountToProcess()
	{
		return CPPHelper::GetFileCountToProcess();
	}

	bool getNextFile(SASN1File* file, int loopID)
	{
		return CPPHelper::GetNextFile(file, loopID);
	}

	bool getDirectoryWithDelimiterFromPath(char* szPath, unsigned long ulLen)
	{
		return CPPHelper::GetDirectoryWithDelimiterFromPath(szPath, ulLen);
	}

	bool getWorkingDirectoryWithDelimiter(char* szPath, unsigned long ulLen)
	{
		return CPPHelper::GetWorkingDirectoryWithDelimiter(szPath, ulLen);
	}

	bool createDirectories(const char* szPath)
	{
		return CPPHelper::CreateDirectories(szPath);
	}
}