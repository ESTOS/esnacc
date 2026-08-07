#include "test_work_dir.h"

#include "interface_baseline.h"
#include "test_support.h"

#include <fstream>

namespace
{
std::filesystem::path CreateUniqueTempDir()
{
	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_int_distribution<int> distribution(0, 0xFFFFFF);

	auto dir = std::filesystem::temp_directory_path() / ("esnacc-cli-test-" + std::to_string(distribution(generator)));
	std::filesystem::create_directories(dir);
	return dir;
}
} // namespace

TestWorkDir::TestWorkDir()
	: m_path(CreateUniqueTempDir())
{
}

TestWorkDir::~TestWorkDir()
{
	std::error_code ec;
	std::filesystem::remove_all(m_path, ec);
}

void TestWorkDir::CopyFixture(const std::filesystem::path& fixturePath) const
{
	std::filesystem::copy_file(fixturePath, m_path / fixturePath.filename(), std::filesystem::copy_options::overwrite_existing);
}

void TestWorkDir::WriteDeprecatedBaseline(const std::filesystem::path& baselineFixturePath) const
{
	std::filesystem::copy_file(baselineFixturePath, m_path / DEPRECATED_BASELINE_FILENAME, std::filesystem::copy_options::overwrite_existing);
}

void TestWorkDir::CopyBaselineModule() const
{
	CopyFixture(FixtureDirectory() / "Baseline_Test.asn1");
}
