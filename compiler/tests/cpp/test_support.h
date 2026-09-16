#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

/**
 * Result of running an external process (exit code and merged stdout/stderr).
 */
struct ProcessResult
{
	int exitCode = -1;
	std::string output;
};

/**
 * Runs a program with arguments in the given working directory.
 */
ProcessResult RunProcess(const std::filesystem::path& executable, const std::vector<std::string>& args, const std::filesystem::path& workingDirectory);

/**
 * Reads a file into a string; returns empty string when the file is missing.
 */
std::string ReadFileToString(const std::filesystem::path& path);

/**
 * Returns true when haystack contains needle.
 */
bool FileContains(const std::filesystem::path& path, const std::string& needle);

/**
 * Resolves the esnacc executable path from ESNACC_EXECUTABLE or test_config.h.
 */
std::filesystem::path ResolveEsnaccExecutable();

/**
 * Directory containing checked-in ASN.1 CLI test fixtures.
 */
std::filesystem::path FixtureDirectory();
