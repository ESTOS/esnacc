#include "test_support.h"
#include "test_work_dir.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	std::filesystem::path InputModulePath(const TestWorkDir& workDir)
	{
		return workDir.path() / "Serializable_Test.asn1";
	}

	std::filesystem::path SerializableOutputPath(const TestWorkDir& workDir)
	{
		return workDir.path() / "out" / "Serializable_Test_Serializable.ts";
	}

	std::filesystem::path ConverterOutputPath(const TestWorkDir& workDir)
	{
		return workDir.path() / "out" / "Serializable_Test_Converter.ts";
	}

	std::filesystem::path TypesOutputPath(const TestWorkDir& workDir)
	{
		return workDir.path() / "out" / "types.ts";
	}

	ProcessResult RunEsnaccTypeScript(const TestWorkDir& workDir, const std::vector<std::string>& extraArgs = {})
	{
		std::filesystem::create_directories(workDir.path() / "out");

		std::vector<std::string> args = {
			"-JTE",
			"-j",
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

	TEST(Compiler_TsSerializableCodegenTest, EmitsSerializableInterfacesForStructuredTypes)
	{
		TestWorkDir workDir;
		workDir.CopyFixture(FixtureDirectory() / "Serializable_Test.asn1");

		const ProcessResult result = RunEsnaccTypeScript(workDir);
		ASSERT_EQ(result.exitCode, 0) << result.output;
		EXPECT_TRUE(std::filesystem::exists(SerializableOutputPath(workDir))) << result.output;

		const std::string serializable = ReadFileToString(SerializableOutputPath(workDir));
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "export interface IAsnPerson")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "name: string")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "avatar?: string")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "status: number")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "export type IAsnPeopleList = IAsnPerson[]")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "export interface IAsnTaggedChoice")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "text?: string")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "code?: number")) << serializable;
		EXPECT_TRUE(FileContains(SerializableOutputPath(workDir), "_type?: string")) << serializable;
		EXPECT_FALSE(FileContains(SerializableOutputPath(workDir), "Uint8Array")) << serializable;
	}

	TEST(Compiler_TsSerializableCodegenTest, ConverterSignaturesReferenceSerializableTypes)
	{
		TestWorkDir workDir;
		workDir.CopyFixture(FixtureDirectory() / "Serializable_Test.asn1");

		const ProcessResult result = RunEsnaccTypeScript(workDir);
		ASSERT_EQ(result.exitCode, 0) << result.output;
		EXPECT_TRUE(std::filesystem::exists(ConverterOutputPath(workDir))) << result.output;

		const std::string converter = ReadFileToString(ConverterOutputPath(workDir));
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "import type * as Serializable_Test_Serializable from \"./Serializable_Test_Serializable.js\"")) << converter;
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "toJSON(s: Serializable_Test.AsnPerson")) << converter;
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "Serializable_Test_Serializable.IAsnPerson | undefined")) << converter;
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "prepareJSONData<Serializable_Test_Serializable.IAsnPerson>")) << converter;
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "fromJSON(data: string | object | undefined")) << converter;
		EXPECT_TRUE(FileContains(ConverterOutputPath(workDir), "Serializable_Test.AsnPerson | undefined")) << converter;
	}

	TEST(Compiler_TsSerializableCodegenTest, TypesFileReExportsSerializableModule)
	{
		TestWorkDir workDir;
		workDir.CopyFixture(FixtureDirectory() / "Serializable_Test.asn1");

		const ProcessResult result = RunEsnaccTypeScript(workDir);
		ASSERT_EQ(result.exitCode, 0) << result.output;
		EXPECT_TRUE(std::filesystem::exists(TypesOutputPath(workDir))) << result.output;
		EXPECT_TRUE(FileContains(TypesOutputPath(workDir), "export * as Serializable_Test_Serializable from \"./Serializable_Test_Serializable.js\"")) << ReadFileToString(TypesOutputPath(workDir));
	}

} // namespace compiler
