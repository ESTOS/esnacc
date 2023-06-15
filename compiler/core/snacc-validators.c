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
	NO_DUPLICATE_OPERATIONIDS = 1,
	OPERATION_ARGUMENT_RESULT_ERROR_ARE_CHOICES_OR_SEQUENCES = 2,
	OPERATION_ERRORS_ARE_OF_SAME_TYPE = 4,
	SEQUENCES_AND_CHOICES_ARE_EXTENDABLE = 8,
	VALIDATE_TYPE_WHITELISTE = 16
};

void ValidateNoDuplicateOperationIDs(ModuleList* allMods);
void ValidateArgumentResultErrorAreSequencesOrChoices(ModuleList* allMods);
void ValidateOnlySupportedObjects(ModuleList* allMods);

void ValidateASN1Data(ModuleList* allMods)
{
	if (giValidationLevel & NO_DUPLICATE_OPERATIONIDS)
	{
		printf("- Validating that operationIDs are not used twice...\n");
		ValidateNoDuplicateOperationIDs(allMods);
	}
	if (giValidationLevel & OPERATION_ARGUMENT_RESULT_ERROR_ARE_CHOICES_OR_SEQUENCES)
	{
		printf("- Validating that operation arguments, results and errors are sequences or choices (only types are extendable)...\n");
		ValidateArgumentResultErrorAreSequencesOrChoices(allMods);
	}
	if (giValidationLevel & OPERATION_ERRORS_ARE_OF_SAME_TYPE)
		printf("- Validating that errors are of the same type to generalize error handling...\n");
	if (giValidationLevel & SEQUENCES_AND_CHOICES_ARE_EXTENDABLE)
		printf("- Validating that all sequences and choices contain ... to allow extending them...\n");
	if (giValidationLevel & VALIDATE_TYPE_WHITELISTE)
		printf("- Validating that only allow types from the esnacc_whiteliste.json are used...\n");
	if (giValidationLevel >= 2)
		ValidateOnlySupportedObjects(allMods);
}

void ValidateNoDuplicateOperationIDs(ModuleList* allMods)
{
	Module* currMod;

	const int MALLOC_SIZE = 50000;
	int* ids = malloc(MALLOC_SIZE);
	if (!ids)
	{
		snacc_exit("Out of memory");
		return;
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
	if (nWeHaveErrors)
		snacc_exit_now(__FUNCTION__, "\nYou must ensure that the method/event IDs are only used once!\nNow terminating...\n");

	free(ids);
}

void ValidateArgumentResultErrorAreSequencesOrChoices(ModuleList* allMods)
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

					char* pszArgument = NULL;
					char* pszResult = NULL;
					char* pszError = NULL;
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

						bool bErrorIssue = false;
						bool bWrongErrorObject = false;
						if (errorType)
						{
							bErrorIssue = errorType->basicType->choiceId != BASICTYPE_SEQUENCE;
							bWrongErrorObject = pszError && strcmp(pszError, "AsnRequestError") != 0;
						}
						if (bArgumentIssue || bResultIssue || bErrorIssue || bWrongErrorObject)
						{
							if (szLastErrorFile != currMod->asn1SrcFileName)
							{
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
								fprintf(stderr, "- %s is using %s as error which is a ", vd->definedName, pszError);
								PrintTypeById(stderr, errorType->basicType->choiceId);
								fprintf(stderr, ".\n  You must use a SEQUENCE here (expandability).\n");
								nWeHaveErrors++;
							}
							if (bWrongErrorObject)
							{
								fprintf(stderr, "- %s is using %s as error but must use AsnRequestError.\n", vd->definedName, pszError);
								nWeHaveErrors++;
							}
						}
					}
				}
			}
		}
	}
	if (nWeHaveErrors)
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "*************************************************************\n");
		fprintf(stderr, "* Methods may contain issues if they are flagged deprecated *\n");
		fprintf(stderr, "*************************************************************\n\n");
		fprintf(stderr, "- found %i errors\n", nWeHaveErrors);
		snacc_exit_now(__FUNCTION__, "Now terminating...\n");
	}
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

// Returns true when an invalid element was found
bool recurseFindInvalid(Module* mod, Type* type, const char* szPath, const char* szElementName)
{
#define TESTBUFFERSIZE 256
#define BUFFERSIZE 4096

	bool bFoundInvalid = false;
	char szCurrentPath[BUFFERSIZE] = {0};
	strcpy_s(szCurrentPath, BUFFERSIZE, szPath);

	enum BasicTypeChoiceId choiceId = type->basicType->choiceId;

	if (szElementName)
	{
		if ((choiceId == BASICTYPE_SEQUENCE || choiceId == BASICTYPE_BITSTRING) && IsDeprecatedFlaggedSequence(mod, szElementName))
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
		bFoundInvalid = recurseFindInvalid(mod, type->basicType->a.localTypeRef->link->type, szCurrentPath, NULL);
	else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		bFoundInvalid = recurseFindInvalid(mod, type->basicType->a.importTypeRef->link->type, szCurrentPath, NULL);
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
			bFoundInvalid = recurseFindInvalid(mod, subType, szCurrentPath, NULL);
		}
		else if (choiceId == BASICTYPE_SEQUENCE)
		{
			NamedTypeList* typeList = type->basicType->a.sequence;
			NamedType* subType;
			FOR_EACH_LIST_ELMT(subType, typeList)
			{
				// Due to possible recursion we need to store the current position
				AsnListNode* oldCurr = typeList->curr;
				if (recurseFindInvalid(mod, subType->type, szCurrentPath, subType->fieldName))
					bFoundInvalid = true;
				typeList->curr = oldCurr;
			}
		}
	}
	return bFoundInvalid;
}

void ValidateOnlySupportedObjects(ModuleList* allMods)
{
	Module* currMod;
	int nWeHaveErrors = 0;
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			TypeDef* vd;
			FOR_EACH_LIST_ELMT(vd, currMod->typeDefs)
			{
				char szPath[128] = {0};
				sprintf_s(szPath, 128, "%s ", currMod->asn1SrcFileName);
				if (recurseFindInvalid(currMod, vd->type, szPath, vd->definedName))
					nWeHaveErrors++;
			}
		}
	}

	if (nWeHaveErrors)
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "**********************************\n");
		fprintf(stderr, "* Found not supported asn1 types *\n");
		fprintf(stderr, "**********************************\n");
		snacc_exit_now(__FUNCTION__, "Now terminating...\n");
	}
}
