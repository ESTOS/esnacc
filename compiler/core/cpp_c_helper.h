#ifndef CPP_HELPER_C_H
#define CPP_HELPER_C_H

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct SASN1File
	{
		// The Filename of the file (e.g. structure.asn1)
		const char* fileName;
		// The full file path (including the name of the file) (e.g. C:\\structure.asn1 or /home/structure.asn1)
		const char* filePath;
		// true if this is an imported file
		// Imported files are held to resolve dependencies in the files the snacc compiler is actually handling.
		// These files generate no output, they are just there to resolve dependencies in the ones beeing parsed
		bool bIsImportedFile;
	} SASN1File;

	/*
	 * Searches for files with the specified pattern and adds them to the filecache (may also be a single file)
	 *
	 * @param szPattern - the pattern we are searching for
	 * @param bIsImportedFile - true if the files we will find are imported files (so will not create output but are loaded to resolved dependencies)
	 * @Returns the amount of files found
	 */
	int findFiles(const char* szPattern, const bool bIsImportedFile);

	/*
	 * Resolves the amount of files which have been loaded to get processed (not imported files!)
	 */
	int getFileCountToProcess();

	/*
	 * Retrieves the a file from the file cache (it delivers both, imported files and files to process)
	 *
	 * @param EASN1File - The properties are put into this struct
	 * @param loopID - The id under which we loop over the files (If you need to loop from different places you need to specify a different loopID)
	 * @returns true in case a file found and the struct has been updated, false if no furhter file has been found
	 */
	bool getNextFile(SASN1File* file, int loopID);

	/*
	 * Retrieves the path with delimiter from a path that may or may not have one
	 * @param szPath - The path we will touch
	 * @param iLen - The len of the buffer szPath
	 * @returns true on success (Path was copied to szPath)
	 */
	bool getDirectoryWithDelimiterFromPath(char* szPath, unsigned long ulLen);

	/*
	 * Retrieves the current working directory including the platform specific path delimiter
	 * @param szPath - The path that will receive the working directory
	 * @param iLen - The len of the buffer szPath
	 * @returns true on success (Path was copied to szPath)
	 */
	bool getWorkingDirectoryWithDelimiter(char* szPath, unsigned long ulLen);

	/*
	 * Creates the directory path (directories) as specified in szPath
	 * @param szPath - The path that shall get created
	 * @returns true on success - also if the directory was already existing (false on error)
	 */
	bool createDirectories(const char* szPath);

	/*
	 * Writes the typescript file content to disc
	 * Adopts the content for ESM and or specific node versions
	 *
	 * @param buffer - the buffer of the embedded ressource
	 * @param ulSize - the size of the embedded ressource
	 * @param szFileName - the filename the file shall be written to
	 * @param bWriteESM - true to write esm files, or false for commonjs
	 * @param nNodeVersion - the node version
	 * @returns true on success
	 */
	bool writeFile(const char* buffer, const unsigned long ulSize, const char* szFileName, const bool bWriteESM, const int nNodeVersion);

#ifdef __cplusplus
}
#endif

#endif // CPP_HELPER_H