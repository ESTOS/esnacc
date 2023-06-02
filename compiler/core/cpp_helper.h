#ifndef CPP_HELPER_H
#define CPP_HELPER_H

#include <string>
#include <map>
#include <filesystem>

class CASN1File {
public:
	// The Filename of the file (e.g. structure.asn1)
	std::string m_strFileName;
	// The full file path (including the name of the file) (e.g. C:\\structure.asn1 or /home/structure.asn1)
	std::string m_strFilePath;
	// true if this is an imported file
	// Imported files are held to resolve dependencies in the files the snacc compiler is actually handling.
	// These files generate no output, they are just there to resolve dependencies in the ones beeing parsed
	bool m_bIsImportedFile;
};

// The list of files we are processing, the key is the filename
class CASN1Files : public std::map<std::string, CASN1File> {
};
// A globalobject to hold the files we are processing (makes it easier to hold this in cpp than in c)
extern CASN1Files fileCache;

// The list of loop ids which are used to loop over the EASN1Files
class CASN1FileLoopIDs : public std::map<int, CASN1Files::const_iterator> {
};
// A globalobject to hold the loop ids and corresponding iterators to loop over the files from c
extern CASN1FileLoopIDs fileCacheLoopIDs;

struct SASN1File;

class CPPHelper
{
public:
	/*
	 * Searches for files with the specified pattern and adds them to the filecache (may also be a single file)
	 *
	 * @param szPattern - the pattern we are searching for
	 * @param bIsImportedFile - true if the files we will find are imported files (so will not create output but are loaded to resolved dependencies)
	 * @Returns the amount of files found
	 */
	static int FindFiles(const char* szPattern, const bool bIsImportedFile);

	/*
	 * Resolves the amount of files which have been loaded to get processed (not imported files!)
	 */
	static int GetFileCountToProcess();

	/*
	 * Retrieves the a file from the file cache (it delivers both, imported files and files to process)
	 *
	 * @param EASN1File - The properties are put into this struct
	 * @param loopID - The id under which we loop over the files (If you need to loop from different places you need to specify a different loopID)
	 * @returns true in case a file found and the struct has been updated, false if no furhter file has been found
	 */
	static bool GetNextFile(SASN1File* file, int loopID);

	/*
	 * Retrieves the path with delimiter from a path that may or may not have one
	 * @param szPath - The path we will touch
	 * @param iLen - The len of the buffer szPath
	 * @returns true on success (Path was copied to szPath)
	 */
	static bool GetDirectoryWithDelimiterFromPath(char* szPath, unsigned long ulLen);

	/*
	 * Retrieves the current working directory including the platform specific path delimiter
	 * @param szPath - The path that will receive the working directory
	 * @param iLen - The len of the buffer szPath
	 * @returns true on success (Path was copied to szPath)
	 */
	static bool GetWorkingDirectoryWithDelimiter(char* szPath, unsigned long ulLen);

private:
	/*
	 * Helper method to add a file into the internal cache map
	 * @param file - The file to add
	 * @param bIsImportedFile - true if it is an imported file
	 * @returns true if the file has been added (or updated) in the cache map
	 */
	static bool AddToCache(const std::filesystem::path& file, const bool bIsImportedFile);

	/*
	 * Converts a file system wildcard notation into a regular expression
	 * @param wildcard - the wildcard we want to replace e.g. *.asn1
	 * @returns the regular expression for the wildcard e.g. .*\\.asn1
	 */
	static std::string wildcardToRegex(const std::string& wildcard);
};

#endif // CPP_HELPER_H