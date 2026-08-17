#include "test_support.h"
#include "test_work_dir.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
std::filesystem::path InputModulePath(const TestWorkDir& workDir)
{
	return workDir.path() / "DeprecatedSuccessor_Test.asn1";
}

std::filesystem::path RefModulePath(const TestWorkDir& workDir)
{
	return workDir.path() / "DeprecatedSuccessor_Ref.asn1";
}

std::filesystem::path UcWebModulePath(const TestWorkDir& workDir)
{
	return workDir.path() / "EUCWeb_Successor_Test.asn1";
}

ProcessResult RunEsnaccOnDeprecatedSuccessorFixture(const TestWorkDir& workDir)
{
	std::filesystem::create_directories(workDir.path() / "out");
	std::vector<std::string> args = {
		"-C",
		"-o",
		(workDir.path() / "out").string(),
		InputModulePath(workDir).string()};
	return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
}

ProcessResult RunEsnaccOnDeprecatedSuccessorFixtureWithRefs(const TestWorkDir& workDir)
{
	std::filesystem::create_directories(workDir.path() / "out");
	std::vector<std::string> args = {
		"-C",
		"-o",
		(workDir.path() / "out").string(),
		RefModulePath(workDir).string(),
		InputModulePath(workDir).string()};
	return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
}

ProcessResult RunEsnaccOnDeprecatedSuccessorFixtureWithUcWeb(const TestWorkDir& workDir)
{
	std::filesystem::create_directories(workDir.path() / "out");
	std::vector<std::string> args = {
		"-C",
		"-o",
		(workDir.path() / "out").string(),
		UcWebModulePath(workDir).string(),
		InputModulePath(workDir).string()};
	return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
}
} // namespace

TEST(DeprecatedSuccessorCliTest, ValidCanonicalNotationProducesNoSuccessorWarnings)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixture(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_EQ(result.output.find("asnValidNone: @deprecated missing canonical successor"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("asnValidSameFile: @deprecated missing canonical successor"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("asnValidExternal: @deprecated missing canonical successor"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("asnSuccessorOnNextLine: @deprecated missing canonical successor"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("asnValidSameFile: @deprecated successor"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, LegacyAndMissingSuccessorsWarnWithoutFailingCompile)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixture(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_NE(result.output.find("asnLegacyProse"), std::string::npos) << result.output;
	EXPECT_NE(result.output.find("asnMissingSuccessor"), std::string::npos) << result.output;
	EXPECT_NE(result.output.find("is not defined in this file"), std::string::npos) << result.output;
	EXPECT_NE(result.output.find("missing canonical successor"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, LegacyOperationWarnsWithSymbolNameInMessage)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixture(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_NE(result.output.find("asnLegacyProse: @deprecated missing canonical successor"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, LoadedCrossModuleSuccessorProducesNoWarning)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Ref.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixtureWithRefs(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_EQ(result.output.find("asnValidCrossModule: @deprecated successor"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, MissingLoadedCrossModuleSuccessorWarns)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Ref.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixtureWithRefs(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_NE(result.output.find("asnInvalidCrossModule"), std::string::npos) << result.output;
	EXPECT_NE(result.output.find("DeprecatedSuccessor_Ref::asnMissingInRef"), std::string::npos) << result.output;
	EXPECT_NE(result.output.find("is not defined in loaded module"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, LoadedUcWebSuccessorProducesNoWarning)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");
	workDir.CopyFixture(FixtureDirectory() / "EUCWeb_Successor_Test.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixtureWithUcWeb(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_EQ(result.output.find("asnValidExternal: @deprecated successor"), std::string::npos) << result.output;
}

TEST(DeprecatedSuccessorCliTest, EnumMemberDeprecationSkipsSuccessorValidation)
{
	TestWorkDir workDir;
	workDir.CopyFixture(FixtureDirectory() / "DeprecatedSuccessor_Test.asn1");

	const ProcessResult result = RunEsnaccOnDeprecatedSuccessorFixture(workDir);
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_EQ(result.output.find("eLegacyEnumValue"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("eValidEnumValue"), std::string::npos) << result.output;
	EXPECT_EQ(result.output.find("AsnEnumMemberDeprecationEnum"), std::string::npos) << result.output;
}
