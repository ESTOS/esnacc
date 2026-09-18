#include "snacc-deprecated-successor.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
	DeprecatedSuccessorFields parseArrow(const char* pszLine)
	{
		DeprecatedSuccessorFields fields;
		EXPECT_TRUE(ParseDeprecatedSuccessorArrowLine(pszLine ? pszLine : "", fields));
		return fields;
	}
} // namespace

namespace compiler
{

	TEST(Compiler_DeprecatedSuccessorParserTest, ParsesNoneWithOptionalComment)
	{
		const DeprecatedSuccessorFields fields = parseArrow("-> (none) optional comment");
		EXPECT_EQ(fields.scope, EDeprecatedSuccessorNone);
		EXPECT_TRUE(fields.symbol.empty());
		EXPECT_EQ(fields.proseAfter, "optional comment");
	}

	TEST(Compiler_DeprecatedSuccessorParserTest, ParsesSameFileSymbol)
	{
		const DeprecatedSuccessorFields fields = parseArrow("-> asnResetUserAbsentState");
		EXPECT_EQ(fields.scope, EDeprecatedSuccessorSameFile);
		EXPECT_EQ(fields.symbol, "asnResetUserAbsentState");
		EXPECT_TRUE(fields.qualifier.empty());
	}

	TEST(Compiler_DeprecatedSuccessorParserTest, ParsesQualifiedSymbol)
	{
		const DeprecatedSuccessorFields fields = parseArrow("-> UCWeb::asnGetTime");
		EXPECT_EQ(fields.scope, EDeprecatedSuccessorQualified);
		EXPECT_EQ(fields.qualifier, "UCWeb");
		EXPECT_EQ(fields.symbol, "asnGetTime");
	}

	TEST(Compiler_DeprecatedSuccessorParserTest, FindsArrowAfterDateRemainder)
	{
		DeprecatedSuccessorFields fields;
		std::string remainder;
		EXPECT_TRUE(FindAndParseDeprecatedSuccessorInText("22.08.2024 -> asnFoo", fields, remainder));
		EXPECT_EQ(fields.scope, EDeprecatedSuccessorSameFile);
		EXPECT_EQ(fields.symbol, "asnFoo");
		EXPECT_EQ(remainder, "22.08.2024");
	}

	TEST(Compiler_DeprecatedSuccessorParserTest, RejectsLegacyProseWithoutArrow)
	{
		DeprecatedSuccessorFields fields;
		std::string remainder;
		EXPECT_FALSE(FindAndParseDeprecatedSuccessorInText("see asnFoo", fields, remainder));
		EXPECT_FALSE(ParseDeprecatedSuccessorArrowLine("see asnFoo", fields));
	}

} // namespace compiler
