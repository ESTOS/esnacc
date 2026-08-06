#include "snacc-validation-rules.h"

#include "../../c-lib/include/asn-config.h"
#include "../../c-lib/include/platform-functions.h"
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace
{
	/**
	 * Case-insensitive ASCII compare for @ignorevalidation rule tokens.
	 * Uses mytolower (platform-functions) so MSVC and GCC/Clang builds share the same path.
	 */
	bool equalsIgnoreCase(const std::string& left, const char* pszRight)
	{
		const char* pszRightSafe = pszRight ? pszRight : "";
		std::string leftCopy(left);
		std::string rightCopy(pszRightSafe);
		if (!leftCopy.empty())
			mytolower(&leftCopy[0]);
		if (!rightCopy.empty())
			mytolower(&rightCopy[0]);
		return leftCopy == rightCopy;
	}

	struct SValidationRuleAlias
	{
		const char* pszAlias;
		EValidationCheck check;
	};

	static const SnaccValidationRuleDesc g_canonicalRules[] = {
#define X(id, bit, tag, desc) {SNACC_VAL_##id, bit, tag, desc},
		SNACC_VALIDATION_RULES_LIST(X)
#undef X
	};

	/* Legacy @ignorevalidation names kept for compatibility with earlier 7.0.12 drafts. */
	static const SValidationRuleAlias g_ruleAliases[] = {
		{"duplicate-opid", SNACC_VAL_UNIQUE_OPERATION_ID},
		{"arg-shape", SNACC_VAL_ROSE_PAYLOAD_EXTENDABLE},
		{"error-type", SNACC_VAL_UNIFORM_OPERATION_ERROR},
		{"extendable", SNACC_VAL_SEQUENCE_HAS_ELLIPSIS},
		{"whitelist", SNACC_VAL_PRIMITIVE_TYPE_WHITELIST},
		{"rose-shape", SNACC_VAL_ROSE_INVOKE_EVENT_SHAPE},
		{"mixed-optionals", SNACC_VAL_NO_MIXED_OPTIONAL_ENCODING},
		{"mixed-optional", SNACC_VAL_NO_MIXED_OPTIONAL_ENCODING},
		{"explicit-optionals", SNACC_VAL_NO_UNTAGGED_OPTIONAL_MEMBERS},
		{"explicit-optional", SNACC_VAL_NO_UNTAGGED_OPTIONAL_MEMBERS},
		{"optional-params-bag", SNACC_VAL_NO_ASN_OPTIONAL_PARAMETERS},
		{"optional-params", SNACC_VAL_NO_ASN_OPTIONAL_PARAMETERS},
		{"optionalparams", SNACC_VAL_NO_ASN_OPTIONAL_PARAMETERS},
	};

	std::string trimToken(const std::string& token)
	{
		size_t start = 0;
		while (start < token.size() && std::isspace(static_cast<unsigned char>(token[start])))
			++start;
		size_t end = token.size();
		while (end > start && std::isspace(static_cast<unsigned char>(token[end - 1])))
			--end;
		return token.substr(start, end - start);
	}

	std::vector<std::string> splitRuleTokens(const std::string& spec)
	{
		std::vector<std::string> tokens;
		std::string current;
		for (char ch : spec)
		{
			if (ch == ',' || ch == ';' || std::isspace(static_cast<unsigned char>(ch)))
			{
				if (!current.empty())
				{
					tokens.push_back(trimToken(current));
					current.clear();
				}
			}
			else
				current += ch;
		}
		if (!current.empty())
			tokens.push_back(trimToken(current));
		return tokens;
	}

	bool lookupCanonicalRuleName(const std::string& token, unsigned int* pBit)
	{
		for (const SnaccValidationRuleDesc& rule : g_canonicalRules)
		{
			if (equalsIgnoreCase(token, rule.pszTagName))
			{
				*pBit = rule.nBit;
				return true;
			}
		}
		return false;
	}

	bool lookupRuleAlias(const std::string& token, unsigned int* pBit)
	{
		for (const SValidationRuleAlias& alias : g_ruleAliases)
		{
			if (equalsIgnoreCase(token, alias.pszAlias))
			{
				*pBit = static_cast<unsigned int>(alias.check);
				return true;
			}
		}
		return false;
	}

	bool lookupRuleName(const std::string& token, unsigned int* pBit)
	{
		return lookupCanonicalRuleName(token, pBit) || lookupRuleAlias(token, pBit);
	}

	unsigned int parseIgnoreValidationRulesSpecInternal(const std::string& spec, std::string* pError)
	{
		const std::string trimmed = trimToken(spec);
		if (trimmed.empty())
		{
			if (pError)
			{
				*pError = "@ignorevalidation requires at least one rule name "
						  "(e.g. no-mixed-optional-encoding, no-untagged-optional-members, no-asn-optional-parameters).";
			}
			return 0;
		}

		unsigned int mask = 0;
		for (const std::string& token : splitRuleTokens(trimmed))
		{
			unsigned int bit = 0;
			if (lookupRuleName(token, &bit))
			{
				mask |= bit;
				continue;
			}

			bool bAllDigits = !token.empty();
			for (char ch : token)
			{
				if (!std::isdigit(static_cast<unsigned char>(ch)))
				{
					bAllDigits = false;
					break;
				}
			}
			if (bAllDigits)
			{
				mask |= static_cast<unsigned int>(std::stoul(token));
				continue;
			}

			if (pError)
			{
				*pError = "Unknown @ignorevalidation rule '";
				*pError += token;
				*pError += "'.";
			}
			return 0;
		}

		if (!mask && pError)
			*pError = "@ignorevalidation rule list did not resolve to any validation bit.";

		return mask;
	}
} // namespace

size_t SnaccGetValidationRuleCount(void)
{
	return sizeof(g_canonicalRules) / sizeof(g_canonicalRules[0]);
}

const SnaccValidationRuleDesc* SnaccGetValidationRule(size_t index)
{
	if (index >= SnaccGetValidationRuleCount())
		return NULL;
	return &g_canonicalRules[index];
}

unsigned int ParseIgnoreValidationRulesSpec(const char* pszSpec, char* pszError, size_t cbError)
{
	std::string error;
	const unsigned int mask = parseIgnoreValidationRulesSpecInternal(pszSpec ? pszSpec : "", &error);
	if (!mask && pszError && cbError > 0 && !error.empty())
		strcpy_s(pszError, cbError, error.c_str());
	return mask;
}

unsigned int ParseIgnoreValidationRulesSpec(const std::string& spec, std::string* pError)
{
	return parseIgnoreValidationRulesSpecInternal(spec, pError);
}

void PrintValidationLevelHelp(FILE* fp)
{
	if (!fp)
		return;

	fprintf(fp, "   0 no validation\n");
	for (size_t i = 0; i < SnaccGetValidationRuleCount(); ++i)
	{
		const SnaccValidationRuleDesc* rule = SnaccGetValidationRule(i);
		if (!rule)
			continue;
		fprintf(fp, "   %u %s\n", rule->nBit, rule->pszDescription);
	}
}

void PrintIgnoreValidationRuleNames(FILE* fp)
{
	if (!fp)
		return;

	fprintf(fp, "   @ignorevalidation rule names (comma/space separated; map to -ValidationLevel bits):\n     ");
	for (size_t i = 0; i < SnaccGetValidationRuleCount(); ++i)
	{
		const SnaccValidationRuleDesc* rule = SnaccGetValidationRule(i);
		if (!rule)
			continue;
		fprintf(fp, "%s (%u)%s", rule->pszTagName, rule->nBit, (i + 1 < SnaccGetValidationRuleCount()) ? ", " : "\n");
	}
	fprintf(fp, "   Example: -- @ignorevalidation no-mixed-optional-encoding, no-untagged-optional-members, no-asn-optional-parameters\n");
}
