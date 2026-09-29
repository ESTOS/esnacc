#include "test_support.h"
#include "test_work_dir.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	std::filesystem::path InputModulePath(const TestWorkDir& workDir)
	{
		return workDir.path() / "TaggedRequired_Test.asn1";
	}

	ProcessResult RunEsnaccCxx(const TestWorkDir& workDir, const std::vector<std::string>& extraArgs = {})
	{
		std::filesystem::create_directories(workDir.path() / "out");

		std::vector<std::string> args = {
			"-C",
			"-o",
			(workDir.path() / "out").string(),
			InputModulePath(workDir).string(),
		};
		args.insert(args.end(), extraArgs.begin(), extraArgs.end());
		return RunProcess(ResolveEsnaccExecutable(), args, workDir.path());
	}
} // namespace

namespace compiler
{

	TEST(Compiler_TaggedRequiredValidationTest, RejectsContextTaggedRequiredMembers)
	{
		TestWorkDir workDir;
		workDir.CopyFixture(FixtureDirectory() / "TaggedRequired_Test.asn1");

		const ProcessResult result = RunEsnaccCxx(workDir);
		EXPECT_NE(result.exitCode, 0) << result.output;
		EXPECT_TRUE(result.output.find("context-tagged member 'stTimestamp' must be OPTIONAL") != std::string::npos) << result.output;
		EXPECT_TRUE(result.output.find("BadSettings") != std::string::npos) << result.output;
	}

	TEST(Compiler_TaggedRequiredValidationTest, CanDisableTaggedRequiredValidation)
	{
		TestWorkDir workDir;
		workDir.CopyFixture(FixtureDirectory() / "TaggedRequired_Test.asn1");

		const ProcessResult result = RunEsnaccCxx(workDir, {"-ValidationLevel", "511"});
		EXPECT_EQ(result.exitCode, 0) << result.output;
	}

} // namespace compiler
