#include "snacc-validators.h"
#include "../core/asn1module.h"
#include "../core/print.h"
#include "snacc.h"
#include "compiler/back-ends/structure-util.h"
#include "mem.h"
#include <string.h>
#include <assert.h>

enum EVALIDATIONCHECK
{
	// Do not allow duplicated operation ids
	NO_DUPLICATE_OPERATIONIDS = 1,
	// Check that arguments results and error are extendable objects (not e.g. lists which are not extendable)
	OPERATION_ARGUMENT_RESULT_ERROR_ARE_CHOICES_OR_SEQUENCES = 2,
	// Check that all errors are the same type to ensure generalized handling
	OPERATION_ERRORS_ARE_OF_SAME_TYPE = 4,
	// Check that all sequences have an extension attribute as last element (...)
	SEQUENCES_ARE_EXTENDABLE = 8,
	// Validate that the used Attribute types are mentioned in the esnacc_whitelist.txt (side by side with the asn1 files)
	VALIDATE_TYPE_WHITELISTE = 16,
	// Validate that invokes consist of an argument, result and error and events only of an error
	VALIDATE_PROPER_INVOKE_EVENT_ARGUMENTS = 32,
	// Validate that optionals are not encoded mixed (implicit and explicit in the same object)
	VALIDATE_OPTIONALS_NO_MIXED_OPTIONALS = 64,
	// Validate that optional are not encoded explicit (without number)
	VALIDATE_OPTIONALS_NO_EXPLICIT_OPTIONALS = 128
};

// These methods return true on success (no error) or false on error

// Reports multi use of operation ids
bool ValidateNoDuplicateOperationIDs(ModuleList* allMods);

// Reports if arguments, results, and errors are NOT sequences or choices
bool ValidateArgumentResultErrorAreSequencesOrChoices(ModuleList* allMods);

// Reports if different error objects are use
bool ValidateErrorsAreOfSameType(ModuleList* allMods);

// Reports if a sequences do not contain the extension field (...) as last element
bool ValidateSequencesAreExtendable(ModuleList* allMods);

// Reports if only supported objects from the whitelist are embedded (fails if others are used)
bool ValidateOnlySupportedObjects(ModuleList* allMods);

// Validates that the ROSE arguments are properly specified (invoke with argument, result and error, event only with argument)
bool ValidateProperROSEArguments(ModuleList* allMods);

// Validates that optionals are either only encoded context specific or at least not shared (implicit and explicit)
bool ValidateOptionals(ModuleList* allMods);

enum BasicTypeChoiceId getType(const char* szType)
{
	if (strcmp(szType, "BASICTYPE_BOOLEAN") == 0)
		return BASICTYPE_BOOLEAN;
	else if (strcmp(szType, "BASICTYPE_INTEGER") == 0)
		return BASICTYPE_INTEGER;
	else if (strcmp(szType, "BASICTYPE_BITSTRING") == 0)
		return BASICTYPE_BITSTRING;
	else if (strcmp(szType, "BASICTYPE_OCTETSTRING") == 0)
		return BASICTYPE_OCTETSTRING;
	else if (strcmp(szType, "BASICTYPE_NULL") == 0)
		return BASICTYPE_NULL;
	else if (strcmp(szType, "BASICTYPE_OID") == 0)
		return BASICTYPE_OID;
	else if (strcmp(szType, "BASICTYPE_REAL") == 0)
		return BASICTYPE_REAL;
	else if (strcmp(szType, "BASICTYPE_ENUMERATED") == 0)
		return BASICTYPE_ENUMERATED;
	else if (strcmp(szType, "BASICTYPE_SEQUENCE") == 0)
		return BASICTYPE_SEQUENCE;
	else if (strcmp(szType, "BASICTYPE_SEQUENCEOF") == 0)
		return BASICTYPE_SEQUENCEOF;
	else if (strcmp(szType, "BASICTYPE_SET") == 0)
		return BASICTYPE_SET;
	else if (strcmp(szType, "BASICTYPE_SETOF") == 0)
		return BASICTYPE_SETOF;
	else if (strcmp(szType, "BASICTYPE_CHOICE") == 0)
		return BASICTYPE_CHOICE;
	else if (strcmp(szType, "BASICTYPE_SELECTION") == 0)
		return BASICTYPE_SELECTION;
	else if (strcmp(szType, "BASICTYPE_COMPONENTSOF") == 0)
		return BASICTYPE_COMPONENTSOF;
	else if (strcmp(szType, "BASICTYPE_ANY") == 0)
		return BASICTYPE_ANY;
	else if (strcmp(szType, "BASICTYPE_ANYDEFINEDBY") == 0)
		return BASICTYPE_ANYDEFINEDBY;
	else if (strcmp(szType, "BASICTYPE_LOCALTYPEREF") == 0)
		return BASICTYPE_LOCALTYPEREF;
	else if (strcmp(szType, "BASICTYPE_IMPORTTYPEREF") == 0)
		return BASICTYPE_IMPORTTYPEREF;
	else if (strcmp(szType, "BASICTYPE_MACROTYPE") == 0)
		return BASICTYPE_MACROTYPE;
	else if (strcmp(szType, "BASICTYPE_MACRODEF") == 0)
		return BASICTYPE_MACRODEF;
	else if (strcmp(szType, "BASICTYPE_NUMERIC_STR") == 0)
		return BASICTYPE_NUMERIC_STR;
	else if (strcmp(szType, "BASICTYPE_PRINTABLE_STR") == 0)
		return BASICTYPE_PRINTABLE_STR;
	else if (strcmp(szType, "BASICTYPE_UNIVERSAL_STR") == 0)
		return BASICTYPE_UNIVERSAL_STR;
	else if (strcmp(szType, "BASICTYPE_IA5_STR") == 0)
		return BASICTYPE_IA5_STR;
	else if (strcmp(szType, "BASICTYPE_BMP_STR") == 0)
		return BASICTYPE_BMP_STR;
	else if (strcmp(szType, "BASICTYPE_UTF8_STR") == 0)
		return BASICTYPE_UTF8_STR;
	else if (strcmp(szType, "BASICTYPE_UTCTIME") == 0)
		return BASICTYPE_UTCTIME;
	else if (strcmp(szType, "BASICTYPE_GENERALIZEDTIME") == 0)
		return BASICTYPE_GENERALIZEDTIME;
	else if (strcmp(szType, "BASICTYPE_GRAPHIC_STR") == 0)
		return BASICTYPE_GRAPHIC_STR;
	else if (strcmp(szType, "BASICTYPE_VISIBLE_STR") == 0)
		return BASICTYPE_VISIBLE_STR;
	else if (strcmp(szType, "BASICTYPE_GENERAL_STR") == 0)
		return BASICTYPE_GENERAL_STR;
	else if (strcmp(szType, "BASICTYPE_OBJECTDESCRIPTOR") == 0)
		return BASICTYPE_OBJECTDESCRIPTOR;
	else if (strcmp(szType, "BASICTYPE_VIDEOTEX_STR") == 0)
		return BASICTYPE_VIDEOTEX_STR;
	else if (strcmp(szType, "BASICTYPE_T61_STR") == 0)
		return BASICTYPE_T61_STR;
	else if (strcmp(szType, "BASICTYPE_EXTERNAL") == 0)
		return BASICTYPE_EXTERNAL;
	else if (strcmp(szType, "BASICTYPE_OCTETCONTAINING") == 0)
		return BASICTYPE_OCTETCONTAINING;
	else if (strcmp(szType, "BASICTYPE_BITCONTAINING") == 0)
		return BASICTYPE_BITCONTAINING;
	else if (strcmp(szType, "BASICTYPE_RELATIVE_OID") == 0)
		return BASICTYPE_RELATIVE_OID;
	else if (strcmp(szType, "BASICTYPE_EXTENSION") == 0)
		return BASICTYPE_EXTENSION;
	else if (strcmp(szType, "BASICTYPE_SEQUENCET") == 0)
		return BASICTYPE_SEQUENCET;
	else if (strcmp(szType, "BASICTYPE_OBJECTCLASS") == 0)
		return BASICTYPE_OBJECTCLASS;
	else if (strcmp(szType, "BASICTYPE_OBJECTCLASSFIELDTYPE") == 0)
		return BASICTYPE_OBJECTCLASSFIELDTYPE;
	else if (strcmp(szType, "BASICTYPE_ASNSYSTEMTIME") == 0)
		return BASICTYPE_ASNSYSTEMTIME;
	else
		return BASICTYPE_UNKNOWN;
}

const char* getTypeName(enum BasicTypeChoiceId choiceId)
{
	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			return "BASICTYPE_BOOLEAN";
		case BASICTYPE_INTEGER:
			return "BASICTYPE_INTEGER";
		case BASICTYPE_BITSTRING:
			return "BASICTYPE_BITSTRING";
		case BASICTYPE_OCTETSTRING:
			return "BASICTYPE_OCTETSTRING";
		case BASICTYPE_NULL:
			return "BASICTYPE_NULL";
		case BASICTYPE_OID:
			return "BASICTYPE_OID";
		case BASICTYPE_REAL:
			return "BASICTYPE_REAL";
		case BASICTYPE_ENUMERATED:
			return "BASICTYPE_ENUMERATED";
		case BASICTYPE_SEQUENCE:
			return "BASICTYPE_SEQUENCE";
		case BASICTYPE_SEQUENCEOF:
			return "BASICTYPE_SEQUENCEOF";
		case BASICTYPE_SET:
			return "BASICTYPE_SET";
		case BASICTYPE_SETOF:
			return "BASICTYPE_SETOF";
		case BASICTYPE_CHOICE:
			return "BASICTYPE_CHOICE";
		case BASICTYPE_SELECTION:
			return "BASICTYPE_SELECTION";
		case BASICTYPE_COMPONENTSOF:
			return "BASICTYPE_COMPONENTSOF";
		case BASICTYPE_ANY:
			return "BASICTYPE_ANY";
		case BASICTYPE_ANYDEFINEDBY:
			return "BASICTYPE_ANYDEFINEDBY";
		case BASICTYPE_LOCALTYPEREF:
			return "BASICTYPE_LOCALTYPEREF";
		case BASICTYPE_IMPORTTYPEREF:
			return "BASICTYPE_IMPORTTYPEREF";
		case BASICTYPE_MACROTYPE:
			return "BASICTYPE_MACROTYPE";
		case BASICTYPE_MACRODEF:
			return "BASICTYPE_MACRODEF";
		case BASICTYPE_NUMERIC_STR:
			return "BASICTYPE_NUMERIC_STR";
		case BASICTYPE_PRINTABLE_STR:
			return "BASICTYPE_PRINTABLE_STR";
		case BASICTYPE_UNIVERSAL_STR:
			return "BASICTYPE_UNIVERSAL_STR";
		case BASICTYPE_IA5_STR:
			return "BASICTYPE_IA5_STR";
		case BASICTYPE_BMP_STR:
			return "BASICTYPE_BMP_STR";
		case BASICTYPE_UTF8_STR:
			return "BASICTYPE_UTF8_STR";
		case BASICTYPE_UTCTIME:
			return "BASICTYPE_UTCTIME";
		case BASICTYPE_GENERALIZEDTIME:
			return "BASICTYPE_GENERALIZEDTIME";
		case BASICTYPE_GRAPHIC_STR:
			return "BASICTYPE_GRAPHIC_STR";
		case BASICTYPE_VISIBLE_STR:
			return "BASICTYPE_VISIBLE_STR";
		case BASICTYPE_GENERAL_STR:
			return "BASICTYPE_GENERAL_STR";
		case BASICTYPE_OBJECTDESCRIPTOR:
			return "BASICTYPE_OBJECTDESCRIPTOR";
		case BASICTYPE_VIDEOTEX_STR:
			return "BASICTYPE_VIDEOTEX_STR";
		case BASICTYPE_T61_STR:
			return "BASICTYPE_T61_STR";
		case BASICTYPE_EXTERNAL:
			return "BASICTYPE_EXTERNAL";
		case BASICTYPE_OCTETCONTAINING:
			return "BASICTYPE_OCTETCONTAINING";
		case BASICTYPE_BITCONTAINING:
			return "BASICTYPE_BITCONTAINING";
		case BASICTYPE_RELATIVE_OID:
			return "BASICTYPE_RELATIVE_OID";
		case BASICTYPE_EXTENSION:
			return "BASICTYPE_EXTENSION";
		case BASICTYPE_SEQUENCET:
			return "BASICTYPE_SEQUENCET";
		case BASICTYPE_OBJECTCLASS:
			return "BASICTYPE_OBJECTCLASS";
		case BASICTYPE_OBJECTCLASSFIELDTYPE:
			return "BASICTYPE_OBJECTCLASSFIELDTYPE";
		case BASICTYPE_ASNSYSTEMTIME:
			return "BASICTYPE_ASNSYSTEMTIME";
		default:
			assert(FALSE);
			return "UNKNOWN";
	}
}

void ValidateASN1Data(ModuleList* allMods)
{
	bool bSucceeded = true;
	if (giValidationLevel & NO_DUPLICATE_OPERATIONIDS)
	{
		if (!ValidateNoDuplicateOperationIDs(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & OPERATION_ARGUMENT_RESULT_ERROR_ARE_CHOICES_OR_SEQUENCES)
	{
		if (!ValidateArgumentResultErrorAreSequencesOrChoices(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & OPERATION_ERRORS_ARE_OF_SAME_TYPE)
	{
		if (!ValidateErrorsAreOfSameType(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & SEQUENCES_ARE_EXTENDABLE)
	{
		if (!ValidateSequencesAreExtendable(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & VALIDATE_TYPE_WHITELISTE)
	{
		if (!ValidateOnlySupportedObjects(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & VALIDATE_PROPER_INVOKE_EVENT_ARGUMENTS)
	{
		if (!ValidateProperROSEArguments(allMods))
			bSucceeded = false;
	}
	if (giValidationLevel & (VALIDATE_OPTIONALS_NO_MIXED_OPTIONALS | VALIDATE_OPTIONALS_NO_EXPLICIT_OPTIONALS))
	{
		if (!ValidateOptionals(allMods))
			bSucceeded = false;
	}
	if (!bSucceeded)
		snacc_exit("Validation failed. Terminating...");
}

bool ValidateNoDuplicateOperationIDs(ModuleList* allMods)
{
	Module* currMod;

	const int MALLOC_SIZE = 50000;
	int* ids = malloc(MALLOC_SIZE);
	if (!ids)
	{
		snacc_exit("Out of memory");
		return false;
	}
	memset(ids, 0x00, MALLOC_SIZE);
	int counter = 0;
	int nWeHaveErrors = 0;

	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (HasROSEOperations(currMod))
			{
				ValueDef* vd;
				FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
				{
					if (!IsROSEValueDef(currMod, vd))
						continue;

					int methodID = vd->value->basicValue->a.integer;
					for (int iCount = 0; iCount < counter; iCount++)
					{
						if (ids[iCount] == methodID)
						{
							if (!nWeHaveErrors)
								fprintf(stderr, "*** Validating that operationIDs are not used twice... ***\n");
							fprintf(stderr, "Method/Event ID %i has been used multiple times.\n", methodID);
							nWeHaveErrors = 1;
							break;
						}
					}

					ids[counter] = methodID;
					counter++;
				}
			}
		}
	}

	free(ids);

	if (nWeHaveErrors)
		fprintf(stderr, "\n");

	return nWeHaveErrors ? false : true;
}

bool ValidateArgumentResultErrorAreSequencesOrChoices(ModuleList* allMods)
{
	Module* currMod;
	int nWeHaveErrors = 0;
	const char* szLastErrorFile = NULL;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (HasROSEOperations(currMod))
			{
				ValueDef* vd;
				FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
				{
					if (!IsROSEValueDef(currMod, vd))
						continue;

					if (IsDeprecatedFlaggedOperation(currMod, vd->definedName))
						continue;

					const char* pszArgument = NULL;
					const char* pszResult = NULL;
					const char* pszError = NULL;
					Type* argumentType = NULL;
					Type* resultType = NULL;
					Type* errorType = NULL;
					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
					{
						struct BasicType* argumentBasicType = NULL;
						bool bArgumentIssue = false;
						if (argumentType)
						{
							argumentBasicType = ResolveBasicTypeReferences(argumentType->basicType, NULL);
							if (!argumentBasicType)
								bArgumentIssue = true;
							else
							{
								enum BasicTypeChoiceId choiceId = argumentBasicType->choiceId;
								if (choiceId != BASICTYPE_SEQUENCE && choiceId != BASICTYPE_CHOICE)
									bArgumentIssue = true;
							}
						}
						struct BasicType* resultBasicType = NULL;
						bool bResultIssue = false;
						if (resultType)
						{
							resultBasicType = ResolveBasicTypeReferences(resultType->basicType, NULL);
							if (!resultBasicType)
								bResultIssue = true;
							else
							{
								enum BasicTypeChoiceId choiceId = resultBasicType->choiceId;
								if (choiceId != BASICTYPE_SEQUENCE && choiceId != BASICTYPE_CHOICE)
									bResultIssue = true;
							}
						}
						struct BasicType* errorBasicType = NULL;
						bool bErrorIssue = false;
						if (errorType)
						{
							errorBasicType = ResolveBasicTypeReferences(errorType->basicType, NULL);
							if (!errorBasicType)
								bErrorIssue = true;
							else
							{
								enum BasicTypeChoiceId choiceId = errorBasicType->choiceId;
								if (choiceId != BASICTYPE_SEQUENCE && choiceId != BASICTYPE_CHOICE)
									bErrorIssue = true;
							}
						}
						if (bArgumentIssue || bResultIssue || bErrorIssue)
						{
							if (szLastErrorFile != currMod->asn1SrcFileName)
							{
								if (!nWeHaveErrors)
									fprintf(stderr, "*** Validating that operation arguments, results and errors are sequences or choices (only types are extendable)... ***\n");
								if (szLastErrorFile)
									fprintf(stderr, "\n");
								fprintf(stderr, "Errors in %s:\n", currMod->asn1SrcFileName);
								szLastErrorFile = currMod->asn1SrcFileName;
							}
							if (bArgumentIssue)
							{
								fprintf(stderr, "- %s is using %s as argument which is a ", vd->definedName, pszArgument);
								PrintTypeById(stderr, argumentType->basicType->choiceId);
								fprintf(stderr, ".\n  You must use a SEQUENCE or CHOICE here (expandability).\n");
								nWeHaveErrors++;
							}
							if (bResultIssue)
							{
								fprintf(stderr, "- %s is using %s as result which is a ", vd->definedName, pszResult);
								PrintTypeById(stderr, resultType->basicType->choiceId);
								fprintf(stderr, ".\n  You must use a SEQUENCE or CHOICE here (expandability).\n");
								nWeHaveErrors++;
							}
							if (bErrorIssue)
							{
								fprintf(stderr, "- %s is using %s as error which is a ", vd->definedName, pszResult);
								PrintTypeById(stderr, errorType->basicType->choiceId);
								fprintf(stderr, ".\n  You must use a SEQUENCE or CHOICE here (expandability).\n");
								nWeHaveErrors++;
							}
						}
					}
				}
			}
		}
	}

	if (nWeHaveErrors)
		fprintf(stderr, "\n");

	return nWeHaveErrors ? false : true;
}

bool ValidateErrorsAreOfSameType(ModuleList* allMods)
{
	Module* currMod;
	int nWeHaveErrors = 0;
	char* szErrorTypes = NULL;
	char* szFirstName = NULL;
	int iFirstType = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (HasROSEOperations(currMod))
			{
				ValueDef* vd;
				FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
				{
					if (!IsROSEValueDef(currMod, vd))
						continue;

					if (IsDeprecatedFlaggedOperation(currMod, vd->definedName))
						continue;

					const char* pszError = NULL;
					Type* errorType = NULL;
					if (GetROSEDetails(currMod, vd, NULL, NULL, &pszError, NULL, NULL, &errorType, false))
					{
						struct BasicType* errorBasicType = NULL;
						if (errorType)
						{
							errorBasicType = ResolveBasicTypeReferences(errorType->basicType, NULL);
							if (errorBasicType)
							{
								char szBuffer[512] = {0};
								sprintf_s(szBuffer, 512, "%i:%s", errorBasicType->choiceId, pszError);
								if (!szErrorTypes)
								{
									size_t stLen = strlen(szBuffer) + 1;
									szErrorTypes = malloc(stLen);
									if (!szErrorTypes)
									{
										snacc_exit("Out of memory 1");
										return false;
									}
									strcpy_s(szErrorTypes, stLen, szBuffer);
									stLen = strlen(pszError) + 1;
									szFirstName = malloc(stLen);
									if (!szFirstName)
									{
										snacc_exit("Out of memory 2");
										return false;
									}
									strcpy_s(szFirstName, stLen, pszError);
									iFirstType = errorBasicType->choiceId;
								}
								else if (strstr(szErrorTypes, szBuffer) == NULL)
								{
									if (!nWeHaveErrors)
									{
										fprintf(stderr, "*** Validating that errors are of the same type to generalize error handling... ***\n");
										fprintf(stderr, "  Different error objects found. Use the same error object for all errors to generalize error handling!\n");
										fprintf(stderr, "  - Name: %s Type: %s\n", szFirstName, getTypeName(iFirstType));
									}
									fprintf(stderr, "  - Name: %s Type: %s\n", pszError, getTypeName(errorBasicType->choiceId));
									size_t stLen = strlen(szErrorTypes) + strlen(szBuffer) + 2;
									char* szNewErrorTypes = realloc(szErrorTypes, stLen);
									if (!szNewErrorTypes)
									{
										snacc_exit("Out of memory 3");
										return false;
									}
									szErrorTypes = szNewErrorTypes;
									strcat_s(szErrorTypes, stLen, " ");
									strcat_s(szErrorTypes, stLen, szBuffer);
									nWeHaveErrors++;
								}
							}
						}
					}
				}
			}
		}
	}
	if (nWeHaveErrors)
		fprintf(stderr, "  -> Found %i different error objects\n\n", nWeHaveErrors + 1);
	Free(szErrorTypes);
	Free(szFirstName);

	return nWeHaveErrors ? false : true;
}

bool ValidateSequencesAreExtendable(ModuleList* allMods)
{
	int nWeHaveErrors = 0;
	int iErrorCounter = 0;
	const char* szLastErrorFile = NULL;
	Module* mod;
	FOR_EACH_LIST_ELMT(mod, allMods)
	{
		if (mod->ImportedFlag == FALSE)
		{
			TypeDef* td;
			FOR_EACH_LIST_ELMT(td, mod->typeDefs)
			{
				if (IsDeprecatedFlaggedSequence(mod, td->definedName))
					continue;

				struct BasicType* type = td->type->basicType;
				enum BasicTypeChoiceId choiceId = type->choiceId;
				bool bFailed = false;
				const char* szType = 0;
				if (choiceId == BASICTYPE_SEQUENCE)
				{
					NamedType* last = LAST_LIST_ELMT(type->a.sequence);
					if (!last)
						bFailed = true;
					else if (last->type->basicType->choiceId != BASICTYPE_EXTENSION)
						bFailed = true;
					szType = "SEQUENCE";
				}
				else
					continue;

				if (!bFailed)
					continue;

				if (!nWeHaveErrors)
					fprintf(stderr, "*** Validating that all sequences contain ... to allow extending them... ***\n");

				if (szLastErrorFile != mod->asn1SrcFileName)
				{
					if (szLastErrorFile)
						fprintf(stderr, "  -> File contained %i error(s)\n\n", iErrorCounter);
					iErrorCounter = 0;
					fprintf(stderr, "Errors in %s:\n", mod->asn1SrcFileName);
					szLastErrorFile = mod->asn1SrcFileName;
				}

				fprintf(stderr, "- %s %s does not contain ... (EXTENSION) as last element\n", szType, td->definedName);
				nWeHaveErrors++;
				iErrorCounter++;
			}
		}
	}

	if (iErrorCounter)
		fprintf(stderr, "  -> File contained %i error(s)\n\n", iErrorCounter);

	return nWeHaveErrors ? false : true;
}

bool isSupportedType(enum BasicTypeChoiceId choiceId)
{
	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_SEQUENCE:
		case BASICTYPE_SEQUENCEOF:
		case BASICTYPE_CHOICE:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_EXTENSION:
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
			// We support these tags
			return true;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			// Each import should be resolved BEFORE calling this method
			assert(FALSE);
			return false;
		default:
			return false;
	}
}

// Returns true when an invalid element was found
bool recurseFindInvalid(Module* mod, Type* type, int* supportedTypes, const char* szPath, const char* szElementName)
{
#define TESTBUFFERSIZE 256
#define BUFFERSIZE 4096
	enum BasicTypeChoiceId choiceId = type->basicType->choiceId;

	if (szElementName)
	{
		if (choiceId == BASICTYPE_SEQUENCE && IsDeprecatedFlaggedSequence(mod, szElementName))
			return false;
		else if (strstr(szPath, "::") == NULL && IsDeprecatedFlaggedSequence(mod, szElementName))
			return false;
	}

	bool bFoundInvalid = false;
	char szCurrentPath[BUFFERSIZE] = {0};
	strcpy_s(szCurrentPath, BUFFERSIZE, szPath);

	if (choiceId != BASICTYPE_LOCALTYPEREF && choiceId != BASICTYPE_IMPORTTYPEREF)
	{
		int i = 0;
		bool bFound = false;
		while (supportedTypes[i] != 0)
		{
			if (supportedTypes[i] == choiceId)
			{
				bFound = true;
				break;
			}
			i++;
		}
		if (!bFound)
		{
			fprintf(stderr, "Unsupported type %s found in %s\n", getTypeName(choiceId), szCurrentPath);
			return true;
		}
	}

	if (szElementName)
	{
		if (choiceId == BASICTYPE_SEQUENCE && IsDeprecatedFlaggedSequence(mod, szElementName))
			return false;
		char szNewName[TESTBUFFERSIZE] = {0};
		strcat_s(szNewName, TESTBUFFERSIZE, "::");
		strcat_s(szNewName, TESTBUFFERSIZE, szElementName);
		if ((choiceId == BASICTYPE_SEQUENCE || choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF) && type->cxxTypeRefInfo->className)
		{
			if (IsDeprecatedFlaggedSequence(mod, type->cxxTypeRefInfo->className))
				return false;
			strcat_s(szNewName, TESTBUFFERSIZE, "(");
			strcat_s(szNewName, TESTBUFFERSIZE, type->cxxTypeRefInfo->className);
			strcat_s(szNewName, TESTBUFFERSIZE, ")");
		}

		char szTest2[TESTBUFFERSIZE] = {0};
		strcat_s(szTest2, TESTBUFFERSIZE, szNewName);
		strcat_s(szTest2, TESTBUFFERSIZE, "::");
		if (strstr(szPath, szTest2))
		{
			// Recursion check -> haben wir schon (raus...)
			return false;
		}

		const char* pos = strstr(szPath, szNewName);
		if (pos)
		{
			// Is it at the end of the string?
			const size_t len1 = strlen(pos);
			const size_t len2 = strlen(szNewName);
			if (len1 == len2)
			{
				// Recursion check -> haben wir schon (raus...)
				return false;
			}
		}

		strcat_s(szCurrentPath, BUFFERSIZE, szNewName);
	}

	if (choiceId == BASICTYPE_LOCALTYPEREF)
		bFoundInvalid = recurseFindInvalid(mod, type->basicType->a.localTypeRef->link->type, supportedTypes, szCurrentPath, NULL);
	else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		bFoundInvalid = recurseFindInvalid(mod, type->basicType->a.importTypeRef->link->type, supportedTypes, szCurrentPath, NULL);
	else
	{
		if (!isSupportedType(choiceId))
		{
			fprintf(stderr, "Unsupported type %s found in %s\n", getTypeName(choiceId), szCurrentPath);
			return true;
		}

		if (choiceId == BASICTYPE_SEQUENCEOF)
		{
			Type* subType = type->basicType->a.sequenceOf;
			bFoundInvalid = recurseFindInvalid(mod, subType, supportedTypes, szCurrentPath, NULL);
		}
		else if (choiceId == BASICTYPE_SEQUENCE)
		{
			NamedTypeList* typeList = type->basicType->a.sequence;
			NamedType* subType;
			FOR_EACH_LIST_ELMT(subType, typeList)
			{
				// Due to possible recursion we need to store the current position
				AsnListNode* oldCurr = typeList->curr;
				if (recurseFindInvalid(mod, subType->type, supportedTypes, szCurrentPath, subType->fieldName))
					bFoundInvalid = true;
				typeList->curr = oldCurr;
			}
		}
	}
	return bFoundInvalid;
}

#define MAX_STR_LENGTH 100
#define MAX_STRINGS 50

void freeStrings(char** strings, int numStrings)
{
	int i;
	for (i = 0; i < numStrings; i++)
		free(strings[i]);
	free(strings);
}

char** loadStringsFromFile(const char* filename, int* numStrings)
{
	FILE* file = NULL;

	if (fopen_s(&file, filename, "r") != 0 || file == NULL)
		return NULL;

	char** strings = (char**)malloc(MAX_STRINGS * sizeof(char*));
	if (strings == NULL)
	{
		fclose(file);
		snacc_exit("Out of memory 1");
		return NULL; // Error handling
	}

	int i;
	for (i = 0; i < MAX_STRINGS; i++)
	{
		strings[i] = (char*)malloc(MAX_STR_LENGTH * sizeof(char));
		if (strings[i] == NULL)
		{
			snacc_exit("Out of memory 2");
			fclose(file);
			freeStrings(strings, i);
			return NULL;
		}
	}

	*numStrings = 0;
	char buffer[4096];
	while (fgets(buffer, sizeof(buffer), file) != NULL)
	{
		if (strlen(buffer) == 0)
			continue;
		if (buffer[0] == '/' && buffer[1] == '/')
			continue;

		// Remove trailing newline character
		buffer[strcspn(buffer, "\n")] = '\0';

		// Copy the string to the list
		strcpy_s(strings[*numStrings], MAX_STR_LENGTH - 1, buffer);
		strings[*numStrings][MAX_STR_LENGTH - 1] = '\0';

		(*numStrings)++;

		if (*numStrings >= MAX_STRINGS)
		{
			snacc_exit("The whitelist may contain a maximum of %i entries.", MAX_STRINGS);
			break;
		}
	}

	fclose(file);
	return strings;
}

bool ValidateOnlySupportedObjects(ModuleList* allMods)
{
	Module* currMod;
	int nWeHaveErrors = 0;
	bool bConfigLoaded = false;
	int* supportedTypes = 0;

	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		int nWeHaveErrorsInFile = 0;
		if (currMod->ImportedFlag == FALSE)
		{
			if (!bConfigLoaded)
			{
				char* szFilePath = getFilePath(currMod->asn1SrcFileName);
				{
					const char* szWhiteListJSON = "esnacc_whitelist.txt";
					size_t folderSize = strlen(szFilePath);
					size_t size = folderSize + strlen(szWhiteListJSON) + 1;
					char* szPath = realloc(szFilePath, size);
					if (!szPath)
					{
						snacc_exit("Out of memory 1");
						return true;
					}
					szFilePath = szPath;
					szFilePath[folderSize] = '\0';
					strcat_s(szFilePath, size, szWhiteListJSON);
				}

				int iCount = 0;
				char** szWhiteList = loadStringsFromFile(szFilePath, &iCount);
				if (!szWhiteList)
				{
					free(szFilePath);
					return true;
				}

				int iSize = (iCount + 1) * sizeof(int);
				supportedTypes = (int*)malloc(iSize);
				if (!supportedTypes)
				{
					snacc_exit("Out of memory 2");
					return false;
				}
				memset(supportedTypes, 0x00, iSize);
				int i;
				int iFound = 0;
				for (i = 0; i < iCount; i++)
				{
					int iType = getType(szWhiteList[i]);
					if (iType == BASICTYPE_UNKNOWN)
					{
						if (!nWeHaveErrors)
							fprintf(stderr, "*** Validating that all sequences contain ... to allow extending them... ***\n");
						if (!nWeHaveErrorsInFile)
							fprintf(stderr, "Errors in %s:\n", szFilePath);
						fprintf(stderr, "Unknown type '%s'\n", szWhiteList[i]);
						nWeHaveErrorsInFile++;
						nWeHaveErrors++;
					}
					else
					{
						supportedTypes[iFound] = iType;
						iFound++;
					}
				}
				free(szFilePath);
				freeStrings(szWhiteList, iCount);
				if (nWeHaveErrors)
				{
					free(supportedTypes);
					return false;
				}
			}

			TypeDef* vd;
			FOR_EACH_LIST_ELMT(vd, currMod->typeDefs)
			{
				char szPath[_MAX_PATH] = {0};
				sprintf_s(szPath, _MAX_PATH, "%s ", currMod->asn1SrcFileName);
				if (recurseFindInvalid(currMod, vd->type, supportedTypes, szPath, vd->definedName))
					nWeHaveErrors++;
			}
		}
	}

	if (supportedTypes)
		free(supportedTypes);

	return nWeHaveErrors ? false : true;
}

bool ValidateProperROSEArguments(ModuleList* allMods)
{
	Module* currMod;
	int nWeHaveErrors = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			if (HasROSEOperations(currMod))
			{
				ValueDef* vd;
				bool bFirstInFile = true;
				FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
				{
					if (!IsROSEValueDef(currMod, vd))
						continue;

					if (IsDeprecatedFlaggedOperation(currMod, vd->definedName))
						continue;

					const char* pszArgument = NULL;
					const char* pszResult = NULL;
					const char* pszError = NULL;
					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, NULL, NULL, NULL, false))
					{
						bool bMissingArgument = false;
						bool bMissingError = false;
						bool bEventHasError = false;
						if (!pszArgument)
							bMissingArgument = true;
						if (pszResult && !pszError)
							bMissingError = true;
						if (!pszResult && pszError)
							bEventHasError = true;
						if (bMissingArgument || bMissingError || bEventHasError)
						{
							if (!nWeHaveErrors)
								fprintf(stderr, "*** Validating that all ROSE operations have proper arguments... ***\n");
							if (bFirstInFile)
							{
								bFirstInFile = false;
								fprintf(stderr, "%s:\n", currMod->asn1SrcFileName);
							}

							fprintf(stderr, "- %s has broken ROSE arguments:\n", vd->definedName);
							if (bMissingArgument)
								fprintf(stderr, "  * Argument is missing\n");
							if (bMissingError)
								fprintf(stderr, "  * Method is an invoke but the error has not been specified (invokes must contain an error)\n");
							if (bEventHasError)
								fprintf(stderr, "  * Method is an event but an error has been specified (events have no error)\n");
							nWeHaveErrors++;
						}
					}
				}
			}
		}
	}

	return nWeHaveErrors ? false : true;
}

bool ValidateOptionals(ModuleList* allMods)
{
	int nWeHaveErrors = 0;
	int iErrorCounter = 0;
	const char* szLastErrorFile = NULL;
	Module* mod;
	FOR_EACH_LIST_ELMT(mod, allMods)
	{
		if (mod->ImportedFlag == FALSE)
		{
			TypeDef* td;
			FOR_EACH_LIST_ELMT(td, mod->typeDefs)
			{
				if (IsDeprecatedFlaggedSequence(mod, td->definedName))
					continue;

				struct BasicType* type = td->type->basicType;
				char* szFirstImplicitOptional = NULL;
				char* szFirstExplicitOptional = NULL;
				bool bContextSpecific_Implicit_Optional = false;
				bool bNotContextSpecific_Explicit_Optional = false;
				if (type->choiceId == BASICTYPE_SEQUENCE)
				{
					NamedTypeList* typeList = type->a.sequence;
					NamedType* subType;
					FOR_EACH_LIST_ELMT(subType, typeList)
					{
						enum BasicTypeChoiceId choice = subType->type->basicType->choiceId;

						if (choice == BASICTYPE_EXTENSION)
							continue;
						if (!subType->type->optional)
							continue;

						bool bIsImplicit = subType->type->implicit;

						if (!bIsImplicit)
						{
							BasicType* resolvedType = ResolveBasicTypeReferences(subType->type->basicType, NULL);
							// If the typeref points to a choice or any the implicit flag is not be properly set (seems to be an issue in the yacc bison code)
							if (resolvedType->choiceId == BASICTYPE_CHOICE || resolvedType->choiceId == BASICTYPE_ANY)
							{
								// So in this case we look if we have a context ID (which also points out the implicit flag)
								if (GetContextID(subType->type) >= 0)
								{
									// We have a context id, so this is an implicit element
									bIsImplicit = true;
								}
							}
						}

						if (bIsImplicit)
						{
							// param2		[0] UTF8String OPTIONAL,
							bContextSpecific_Implicit_Optional = true;
							if (!szFirstImplicitOptional)
								szFirstImplicitOptional = subType->fieldName;
						}
						else
						{
							// param2		UTF8String OPTIONAL,
							bNotContextSpecific_Explicit_Optional = true;
							if (!szFirstExplicitOptional)
								szFirstExplicitOptional = subType->fieldName;
						}

						if (bContextSpecific_Implicit_Optional && bNotContextSpecific_Explicit_Optional)
							break;
					}
				}
				else
					continue;

				int nError = 0;

				// Validate that optionals are not encoded mixed (implicit and explicit in the same object)
				if (giValidationLevel & VALIDATE_OPTIONALS_NO_MIXED_OPTIONALS)
				{
					if (bNotContextSpecific_Explicit_Optional & bContextSpecific_Implicit_Optional)
						nError |= VALIDATE_OPTIONALS_NO_MIXED_OPTIONALS;
				}

				// Validate that optional are not encoded explicit (without number)
				if (giValidationLevel & VALIDATE_OPTIONALS_NO_EXPLICIT_OPTIONALS)
				{
					if (bNotContextSpecific_Explicit_Optional)
						nError |= VALIDATE_OPTIONALS_NO_EXPLICIT_OPTIONALS;
				}

				if (!nError)
					continue;

				if (!nWeHaveErrors)
					fprintf(stderr, "*** Validating sequences concerning usage of optional params... ***\n");

				if (szLastErrorFile != mod->asn1SrcFileName)
				{
					if (szLastErrorFile)
						fprintf(stderr, "  -> File contained %i error(s)\n\n", iErrorCounter);
					fprintf(stderr, "Errors in %s:\n", mod->asn1SrcFileName);
					szLastErrorFile = mod->asn1SrcFileName;
					iErrorCounter = 0;
				}

				if (nError & VALIDATE_OPTIONALS_NO_MIXED_OPTIONALS)
					fprintf(stderr, "- %s contains explicit (%s) and implicit (%s) optionals, use only implicit (with number)\n", szFirstExplicitOptional, szFirstImplicitOptional, td->definedName);
				else if (nError & VALIDATE_OPTIONALS_NO_EXPLICIT_OPTIONALS)
					fprintf(stderr, "- %s contains explicit optionals, use only implicit (with number)\n", td->definedName);

				iErrorCounter++;
				nWeHaveErrors++;
			}
		}
	}

	if (iErrorCounter)
		fprintf(stderr, "  -> File contained %i error(s)\n\n", iErrorCounter);

	return nWeHaveErrors ? false : true;
}