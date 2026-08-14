#include "test_support.h"

#include "test_config.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
std::string QuoteArgument(const std::string& arg)
{
	std::string quoted = "\"";
	for (const char ch : arg)
	{
		if (ch == '"')
			quoted += "\\\"";
		quoted += ch;
	}
	quoted += '"';
	return quoted;
}
} // namespace

ProcessResult RunProcess(const std::filesystem::path& executable, const std::vector<std::string>& args, const std::filesystem::path& workingDirectory)
{
	ProcessResult result;

	std::ostringstream command;
	command << QuoteArgument(executable.string());
	for (const std::string& arg : args)
		command << ' ' << QuoteArgument(arg);

	const std::filesystem::path outputFile = workingDirectory / ".esnacc_process_output.txt";
	std::error_code removeEc;

#ifdef _WIN32
	std::string commandLine = "cd /d " + QuoteArgument(workingDirectory.string()) + " && " + command.str() + " > " + QuoteArgument(outputFile.string()) + " 2>&1";
	result.exitCode = std::system(commandLine.c_str());
#else
	std::string commandLine = "cd " + QuoteArgument(workingDirectory.string()) + " && " + command.str() + " > " + QuoteArgument(outputFile.string()) + " 2>&1";
	result.exitCode = std::system(commandLine.c_str());
#endif

	result.output = ReadFileToString(outputFile);
	std::filesystem::remove(outputFile, removeEc);

	return result;
}

std::string ReadFileToString(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
		return {};

	std::ostringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

bool FileContains(const std::filesystem::path& path, const std::string& needle)
{
	return ReadFileToString(path).find(needle) != std::string::npos;
}

std::filesystem::path ResolveEsnaccExecutable()
{
	if (const char* pszEnv = std::getenv("ESNACC_EXECUTABLE"))
	{
		if (*pszEnv)
			return pszEnv;
	}
	throw std::runtime_error("ESNACC_EXECUTABLE environment variable is not set");
}

std::filesystem::path FixtureDirectory()
{
	return FIXTURES_DIR;
}
