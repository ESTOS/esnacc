#include "interface_baseline.h"
#include "test_config.h"

#include <gtest/gtest.h>

#include <filesystem>

extern "C"
{
#include "time_helpers.h"

	int gMajorInterfaceVersion = 0;
	long long gi64NoDeprecatedSymbols = 0;

	long long GetMaxDeprecatedTimestamp()
	{
		return 0;
	}

	int copy_file(const char* source, const char* destination)
	{
		(void)source;
		(void)destination;
		return 1;
	}
}

class InterfaceBaselineTest : public ::testing::Test
{
protected:
	void SetUp() override { ResetInterfaceBaselineStateForTests(); }
};

TEST_F(InterfaceBaselineTest, LoadDeprecatedBaselineFileIgnoresMissingFile)
{
	EXPECT_EQ(LoadDeprecatedBaselineFile("Z:\\definitely-not-a-real-esnacc-test-directory"), 0);
	EXPECT_EQ(gMajorInterfaceVersion, 0);
	EXPECT_EQ(gi64NoDeprecatedSymbols, 0);
}

TEST_F(InterfaceBaselineTest, LoadDeprecatedBaselineFileIgnoresCommentOnlyFile)
{
	const std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "esnacc-baseline-load-test";
	std::filesystem::create_directories(tempDir);
	std::filesystem::copy_file(
		std::filesystem::path(FIXTURES_DIR) / "deprecatedbaseline_comments_only.txt",
		tempDir / DEPRECATED_BASELINE_FILENAME,
		std::filesystem::copy_options::overwrite_existing);

	EXPECT_EQ(LoadDeprecatedBaselineFile(tempDir.string().c_str()), 0);
	EXPECT_EQ(gMajorInterfaceVersion, 0);
	EXPECT_EQ(gi64NoDeprecatedSymbols, 0);

	std::error_code ec;
	std::filesystem::remove_all(tempDir, ec);
}

TEST_F(InterfaceBaselineTest, GetInterfaceBaselineUnixTimestampUsesDeprecatedCutoff)
{
	ApplyDeprecatedCutoffUnix(ConvertDateToUnixTime("01.01.2024"));
	EXPECT_EQ(GetInterfaceBaselineUnixTimestamp(), ConvertDateToUnixTime("01.01.2024"));
}

TEST_F(InterfaceBaselineTest, ParseDateBaselineSetsMajorAndDeprecatedCutoff)
{
	ASSERT_TRUE(ParseInterfaceVersionValue("01.01.2024"));
	EXPECT_EQ(gMajorInterfaceVersion, 20240101);
	EXPECT_EQ(gi64NoDeprecatedSymbols, ConvertDateToUnixTime("01.01.2024"));
}

TEST_F(InterfaceBaselineTest, ParseLegacyIntegerSetsMajorOnly)
{
	ASSERT_TRUE(ParseInterfaceVersionValue("1"));
	EXPECT_EQ(gMajorInterfaceVersion, 1);
	EXPECT_EQ(gi64NoDeprecatedSymbols, 0);
}

TEST_F(InterfaceBaselineTest, ParseDateWithAutoResolveDefersDeprecatedCutoff)
{
	gNodeprecatedAutoResolve = 1;
	ASSERT_TRUE(ParseInterfaceVersionValue("01.01.2024"));
	EXPECT_EQ(gMajorInterfaceVersion, 20240101);
	EXPECT_EQ(gi64NoDeprecatedSymbols, 0);
	EXPECT_EQ(gi64FileBaselineUnix, ConvertDateToUnixTime("01.01.2024"));
}

TEST_F(InterfaceBaselineTest, CliNodeprecatedDateDoesNotApplyFileBaselineCutoff)
{
	gCliNodeprecatedExplicit = 1;
	ASSERT_TRUE(ParseInterfaceVersionValue("01.01.2024"));
	EXPECT_EQ(gMajorInterfaceVersion, 20240101);
	EXPECT_EQ(gi64NoDeprecatedSymbols, 0);
}

TEST_F(InterfaceBaselineTest, ApplyDeprecatedCutoffUnixAlignsMajorVersionLabel)
{
	ApplyDeprecatedCutoffUnix(ConvertDateToUnixTime("15.06.2024"));
	EXPECT_EQ(gMajorInterfaceVersion, 20240615);
	EXPECT_EQ(gi64NoDeprecatedSymbols, ConvertDateToUnixTime("15.06.2024"));
}
