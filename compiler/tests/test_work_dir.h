#pragma once

#include <filesystem>
#include <random>
#include <string>

/**
 * RAII temp directory for isolated esnacc CLI invocations.
 */
class TestWorkDir
{
public:
	TestWorkDir();
	~TestWorkDir();

	TestWorkDir(const TestWorkDir&) = delete;
	TestWorkDir& operator=(const TestWorkDir&) = delete;

	const std::filesystem::path& path() const { return m_path; }

	void CopyFixture(const std::filesystem::path& fixturePath) const;
	void WriteDeprecatedBaseline(const std::filesystem::path& baselineFixturePath) const;
	void CopyBaselineModule() const;

private:
	std::filesystem::path m_path;
};
