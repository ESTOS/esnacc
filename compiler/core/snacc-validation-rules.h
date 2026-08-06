#ifndef SNACC_VALIDATION_RULES_H
#define SNACC_VALIDATION_RULES_H

#include <stddef.h>
#include <stdio.h>

/*
 * Single source of truth for esnacc -ValidationLevel bits and @ignorevalidation rule names.
 * Included from C and C++ translation units.
 *
 * Extend SNACC_VALIDATION_RULES_LIST only — enum values, tag names, and help text follow from here.
 */
#define SNACC_VALIDATION_RULES_LIST(X)                                                                                          \
	X(UNIQUE_OPERATION_ID, 1, "unique-operation-id",                                                                             \
	  "Validates that operation IDs are unique within the module")                                                              \
	X(ROSE_PAYLOAD_EXTENDABLE, 2, "rose-payload-extendable",                                                                   \
	  "ROSE argument, result, and error types must be SEQUENCE or CHOICE")                                                      \
	X(UNIFORM_OPERATION_ERROR, 4, "uniform-operation-error",                                                                     \
	  "All ROSE operation ERROR types in a module must be the same type")                                                        \
	X(SEQUENCE_HAS_ELLIPSIS, 8, "sequence-has-ellipsis",                                                                         \
	  "SEQUENCE types must end with ... for extensibility")                                                                      \
	X(PRIMITIVE_TYPE_WHITELIST, 16, "primitive-type-whitelist",                                                                  \
	  "Only ASN.1 primitive types listed in esnacc_whitelist.txt are allowed")                                                   \
	X(ROSE_INVOKE_EVENT_SHAPE, 32, "rose-invoke-event-shape",                                                                   \
	  "ROSE invoke operations need argument, result, and error; events only an argument")                                        \
	X(NO_MIXED_OPTIONAL_ENCODING, 64, "no-mixed-optional-encoding",                                                            \
	  "SEQUENCE must not mix context-tagged [n] OPTIONAL and untagged OPTIONAL members")                                         \
	X(NO_UNTAGGED_OPTIONAL_MEMBERS, 128, "no-untagged-optional-members",                                                       \
	  "OPTIONAL SEQUENCE members must use context tags [n], not untagged OPTIONAL")                                              \
	X(NO_ASN_OPTIONAL_PARAMETERS, 256, "no-asn-optional-parameters",                                                             \
	  "SEQUENCE must not declare optionalParams / AsnOptionalParameters members")

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum EValidationCheck
	{
#define X(id, bit, tag, desc) SNACC_VAL_##id = bit,
		SNACC_VALIDATION_RULES_LIST(X)
#undef X
	} EValidationCheck;

	/* All per-type / per-operation checks (excludes unique-operation-id). */
#define SNACC_VAL_ALL_TYPE_CHECKS 0x000001FE

	typedef struct SnaccValidationRuleDesc
	{
		EValidationCheck check;
		unsigned int nBit;
		const char* pszTagName;
		const char* pszDescription;
	} SnaccValidationRuleDesc;

	/* Canonical rules (one row per bit). */
	size_t SnaccGetValidationRuleCount(void);
	const SnaccValidationRuleDesc* SnaccGetValidationRule(size_t index);

	/* Parse @ignorevalidation rule list (canonical names, legacy aliases, and/or numeric bits). */
	unsigned int ParseIgnoreValidationRulesSpec(const char* pszSpec, char* pszError, size_t cbError);

	void PrintValidationLevelHelp(FILE* fp);
	void PrintIgnoreValidationRuleNames(FILE* fp);

#ifdef __cplusplus
}

enum class EValidationCheckClass : unsigned int
{
#define X(id, bit, tag, desc) id = SNACC_VAL_##id,
	SNACC_VALIDATION_RULES_LIST(X)
#undef X
};

#include <string>

unsigned int ParseIgnoreValidationRulesSpec(const std::string& spec, std::string* pError);
#endif

#endif /* SNACC_VALIDATION_RULES_H */
