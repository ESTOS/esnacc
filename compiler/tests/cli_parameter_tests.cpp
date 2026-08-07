#include "test_support.h"
#include "test_work_dir.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
constexpr const char* kDeprecatedOperation = "asnDeprecatedOp OPERATION";
constexpr const char* kCurrentOperation = "asnCurrentOp OPERATION";

std::filesystem::path InputModulePath(const TestWorkDir& workDir)
{
	return workDir.path() / "Baseline_Test.asn1";
}

std::filesystem::path FilterOutputPath(const TestWorkDir& workDir)
{
	return workDir.path() / "out" / "Baseline_Test.asn1";
}

std::filesystem::path CodegenHeaderPath(const TestWorkDir& workDir)
{
	return workDir.path() / "out" / "Baseline_Test.h";
}

ProcessResult RunEsnaccFilter(const TestWorkDir& workDir, const std::vector<std::string>& extraArgs)
{
	std::filesystem::create_directories(workDir.path() / "out");

	std::vector<std::string> args = {"-C", "-filter", "-o", (workDir.path() / "out").string(), InputModulePath(workDir).string()};
	args.insert(args.end(), extraArgs.begin(), extraArgs.end());
	return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
}

ProcessResult RunEsnaccCodegen(const TestWorkDir& workDir, const std::vector<std::string>& extraArgs)
{
	std::filesystem::create_directories(workDir.path() / "out");

	std::vector<std::string> args = {"-C", "-o", (workDir.path() / "out").string(), InputModulePath(workDir).string()};
	args.insert(args.end(), extraArgs.begin(), extraArgs.end());
	return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
}
} // namespace

TEST(CompilerCliParameterTest, NodeprecatedAutoUsesNewestDeprecatedDate)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();

	const ProcessResult result = RunEsnaccFilter(workDir, {"-nodeprecated"});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_FALSE(FileContains(FilterOutputPath(workDir), kDeprecatedOperation)) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kCurrentOperation)) << result.output;
}

TEST(CompilerCliParameterTest, NodeprecatedDateRemovesSymbolsDeprecatedOnOrBeforeCutoff)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();

	const ProcessResult result = RunEsnaccFilter(workDir, {"-nodeprecated:15.06.2024"});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_FALSE(FileContains(FilterOutputPath(workDir), kDeprecatedOperation)) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kCurrentOperation)) << result.output;
}

TEST(CompilerCliParameterTest, NodeprecatedDateBeforeDeprecationKeepsSymbol)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();

	const ProcessResult result = RunEsnaccFilter(workDir, {"-nodeprecated:01.01.2024"});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kDeprecatedOperation)) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kCurrentOperation)) << result.output;
}

TEST(CompilerCliParameterTest, CompileSucceedsWithoutDeprecatedBaselineFile)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();

	const ProcessResult result = RunEsnaccCodegen(workDir, {});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(std::filesystem::exists(CodegenHeaderPath(workDir))) << result.output;
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_MAJOR_VERSION 0")) << ReadFileToString(CodegenHeaderPath(workDir));
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_BASELINE")) << ReadFileToString(CodegenHeaderPath(workDir));
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_VERSION")) << ReadFileToString(CodegenHeaderPath(workDir));
}

TEST(CompilerCliParameterTest, CommentOnlyDeprecatedBaselineBehavesLikeMissingFile)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();
	workDir.WriteDeprecatedBaseline(FixtureDirectory() / "deprecatedbaseline_comments_only.txt");

	const ProcessResult result = RunEsnaccCodegen(workDir, {});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_MAJOR_VERSION 0")) << ReadFileToString(CodegenHeaderPath(workDir));
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_BASELINE")) << ReadFileToString(CodegenHeaderPath(workDir));
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_VERSION")) << ReadFileToString(CodegenHeaderPath(workDir));
}

TEST(CompilerCliParameterTest, InterfaceVersionDateSetsMajorVersionInGeneratedHeader)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();
	workDir.WriteDeprecatedBaseline(FixtureDirectory() / "deprecatedbaseline_date.txt");

	const ProcessResult result = RunEsnaccCodegen(workDir, {});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_MAJOR_VERSION 20240101")) << ReadFileToString(CodegenHeaderPath(workDir));
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_BASELINE \"2024-01-01T00:00:00Z\"")) << ReadFileToString(CodegenHeaderPath(workDir));
}

TEST(CompilerCliParameterTest, InterfaceVersionLegacyIntegerDoesNotAutoFilterDeprecated)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();
	workDir.WriteDeprecatedBaseline(FixtureDirectory() / "deprecatedbaseline_legacy.txt");

	const ProcessResult result = RunEsnaccFilter(workDir, {});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kDeprecatedOperation)) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kCurrentOperation)) << result.output;
}

TEST(CompilerCliParameterTest, InterfaceVersionLegacyIntegerLabelsGeneratedMajorVersion)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();
	workDir.WriteDeprecatedBaseline(FixtureDirectory() / "deprecatedbaseline_legacy.txt");

	const ProcessResult result = RunEsnaccCodegen(workDir, {});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_TRUE(FileContains(CodegenHeaderPath(workDir), "BASELINE_TEST_MODULE_MAJOR_VERSION 1")) << ReadFileToString(CodegenHeaderPath(workDir));
}

TEST(CompilerCliParameterTest, NodeprecatedDateOverridesInterfaceVersionFile)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();
	workDir.WriteDeprecatedBaseline(FixtureDirectory() / "deprecatedbaseline_date.txt");

	const ProcessResult result = RunEsnaccFilter(workDir, {"-nodeprecated:15.06.2024"});
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_FALSE(FileContains(FilterOutputPath(workDir), kDeprecatedOperation)) << result.output;
}

TEST(CompilerCliParameterTest, NoprivateRemovesPrivateOperationsFromFilterOutput)
{
	TestWorkDir workDir;
	workDir.CopyBaselineModule();

	const std::string modulePath = InputModulePath(workDir).string();
	std::string moduleText = ReadFileToString(InputModulePath(workDir));
	const std::string privateBlock =
		"\n-- @private\nasnPrivateOp OPERATION\n\tARGUMENT\targ NULL\n\tRESULT\t\tres NULL\n::= 9003\n";
	ASSERT_FALSE(moduleText.empty());
	moduleText.insert(moduleText.find("asnCurrentOp OPERATION"), privateBlock);
	{
		std::ofstream output(InputModulePath(workDir), std::ios::binary | std::ios::trunc);
		output << moduleText;
	}

	std::filesystem::create_directories(workDir.path() / "out");
	std::vector<std::string> args = {
		"-C",
		"-filter",
		"-noprivate",
		"-o",
		(workDir.path() / "out").string(),
		InputModulePath(workDir).string(),
	};
	const ProcessResult result = RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
	ASSERT_EQ(result.exitCode, 0) << result.output;
	EXPECT_FALSE(FileContains(FilterOutputPath(workDir), "asnPrivateOp OPERATION")) << result.output;
	EXPECT_TRUE(FileContains(FilterOutputPath(workDir), kCurrentOperation)) << result.output;
}
