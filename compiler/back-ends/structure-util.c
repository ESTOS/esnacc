#include "structure-util.h"
#include "../core/asn_comments.h"
#include "../core/asn1module.h"
#include <assert.h>
#include <string.h>

int HasROSEOperations(Module* m)
{
	ValueDef* vd;
	int iHasOperations = 0;
	// check for existing operation defines....
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (IsROSEValueDef(m, vd))
		{
			iHasOperations = 1;
			break;
		}
	}

	return iHasOperations;
}

bool IsDeprecatedNoOutput(const long long i64DeprecatedValue)
{
	if (!gi64NoDeprecatedSymbols || !i64DeprecatedValue)
		return false;
	// If set to 1 no date has been specified on the command line, so we remove ANY deprecated value no matter when it was flagged deprecated
	if (gi64NoDeprecatedSymbols == 1)
		return true;
	return gi64NoDeprecatedSymbols >= i64DeprecatedValue;
}

bool IsROSEValueDef(Module* mod, ValueDef* vd)
{
	if (vd->value->type == NULL)
		return false;
	if (vd->value->type->basicType->choiceId != BASICTYPE_MACROTYPE)
		return false;
	if (vd->value->type->basicType->a.macroType->choiceId != MACROTYPE_ROSOPERATION)
		return false;
	if (gi64NoDeprecatedSymbols)
	{
		RosOperationMacroType* pRoseOperation = vd->value->type->basicType->a.macroType->a.rosOperation;
		if (!pRoseOperation)
			return false;
		asnoperationcomment com;
		if (GetOperationComment_UTF8(mod->moduleName, vd->definedName, &com))
		{
			if (IsDeprecatedNoOutput(com.i64Deprecated))
				return false;
		}
	}

	return true;
}

bool GetROSEDetails(Module* mod, ValueDef* vd, const char** ppszArgument, const char** ppszResult, const char** ppszError, Type** argumentType, Type** resultType, Type** errorType, bool bResolveToRoot)
{
	if (ppszArgument)
		*ppszArgument = NULL;
	if (ppszResult)
		*ppszResult = NULL;
	if (ppszError)
		*ppszError = NULL;

	Type* (*resolver)(Type* type, const char** szName) = NULL;
	// Depends on the caller we resolved any type to root or one level (just one local type ref or imported type ref is resolved)
	if (bResolveToRoot)
		resolver = &ResolveTypeReferencesToRoot;
	else
		resolver = &ResolveTypeReferencesOneLevel;

	if (!IsROSEValueDef(mod, vd))
		return false;

	RosOperationMacroType* pOperation = vd->value->type->basicType->a.macroType->a.rosOperation;
	if (!pOperation)
		return false;

	bool bRetVal = false;

	if (IsROSEValueDef(mod, vd))
	{
		struct NamedType* pArgument = pOperation->arguments;
		if (pArgument && (ppszArgument || argumentType))
		{
			if (pArgument->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || pArgument->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				if (argumentType)
					*argumentType = resolver(pArgument->type, ppszArgument);
				else
					resolver(pArgument->type, ppszArgument);
				bRetVal = true;
			}
		}

		struct NamedType* pResult = pOperation->result;
		if (pResult && (ppszResult || resultType))
		{
			if (pResult->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || pResult->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				if (resultType)
					*resultType = resolver(pResult->type, ppszResult);
				else
					resolver(pResult->type, ppszResult);
				bRetVal = true;
			}
		}

		TypeOrValueList* pErrors = pOperation->errors;
		if (pErrors && pErrors->count)
		{
			TypeOrValue* first = (TypeOrValue*)FIRST_LIST_ELMT(pErrors);
			if (first->choiceId == TYPEORVALUE_TYPE)
			{
				if (first->a.type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || first->a.type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
				{
					if (errorType)
						*errorType = resolver(first->a.type, ppszError);
					else
						resolver(first->a.type, ppszError);
					bRetVal = true;
				}
			}
		}
	}
	return bRetVal;
}

BasicType* ResolveBasicTypeReferences(BasicType* type, const char** szName)
{
	BasicType* returnType = type;
	while (returnType->choiceId == BASICTYPE_LOCALTYPEREF || returnType->choiceId == BASICTYPE_IMPORTTYPEREF)
	{
		if (returnType->choiceId == BASICTYPE_LOCALTYPEREF)
		{
			TypeRef* localTypeRef = returnType->a.localTypeRef;
			if (szName)
				*szName = localTypeRef->typeName;
			returnType = localTypeRef->link->type->basicType;
		}
		else if (returnType->choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			TypeRef* importTypeRef = returnType->a.importTypeRef;
			if (szName)
				*szName = importTypeRef->typeName;
			returnType = importTypeRef->link->type->basicType;
		}
	}
	return returnType;
}

Type* ResolveTypeReferencesOneLevel(Type* type, const char** szName)
{
	Type* returnType = type;
	if (returnType->basicType->choiceId == BASICTYPE_LOCALTYPEREF)
	{
		TypeRef* localTypeRef = returnType->basicType->a.localTypeRef;
		if (szName)
			*szName = localTypeRef->typeName;
		returnType = localTypeRef->link->type;
	}
	else if (returnType->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
	{
		TypeRef* importTypeRef = returnType->basicType->a.importTypeRef;
		if (szName)
			*szName = importTypeRef->typeName;
		returnType = importTypeRef->link->type;
	}
	return returnType;
}

Type* ResolveTypeReferencesToRoot(Type* type, const char** szName)
{
	Type* returnType = type;
	while (returnType->basicType->choiceId == BASICTYPE_LOCALTYPEREF || returnType->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
	{
		if (returnType->basicType->choiceId == BASICTYPE_LOCALTYPEREF)
		{
			TypeRef* localTypeRef = returnType->basicType->a.localTypeRef;
			if (szName)
				*szName = localTypeRef->typeName;
			returnType = localTypeRef->link->type;
		}
		else if (returnType->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			TypeRef* importTypeRef = returnType->basicType->a.importTypeRef;
			if (szName)
				*szName = importTypeRef->typeName;
			returnType = importTypeRef->link->type;
		}
	}
	return returnType;
}

Type* GetRootType(Type* type, const char** szName)
{
	Type* returnType = type;
	while (returnType->basicType->choiceId == BASICTYPE_LOCALTYPEREF || returnType->basicType->choiceId == BASICTYPE_IMPORTTYPEREF || returnType->basicType->choiceId == BASICTYPE_SEQUENCEOF || returnType->basicType->choiceId == BASICTYPE_SETOF)
	{
		if (returnType->basicType->choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (szName)
				*szName = returnType->basicType->a.localTypeRef->typeName;
			returnType = returnType->basicType->a.localTypeRef->link->type;
		}
		else if (returnType->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (szName)
				*szName = returnType->basicType->a.localTypeRef->typeName;
			returnType = returnType->basicType->a.importTypeRef->link->type;
		}
		else if (returnType->basicType->choiceId == BASICTYPE_SEQUENCEOF)
		{
			if (szName)
				*szName = returnType->basicType->a.sequenceOf->typeName;
			returnType = returnType->basicType->a.sequenceOf;
		}
		else if (returnType->basicType->choiceId == BASICTYPE_SETOF)
		{
			if (szName)
				*szName = returnType->basicType->a.setOf->typeName;
			returnType = returnType->basicType->a.setOf;
		}
	}

	return returnType;
}

int IsSimpleType(const enum BasicTypeChoiceId type)
{
	switch (type)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_REAL:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_INTEGER:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_BITSTRING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_NULL:
		case BASICTYPE_ANY:
			return TRUE;
		case BASICTYPE_SEQUENCE:
		case BASICTYPE_SETOF:
		case BASICTYPE_CHOICE:
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
		case BASICTYPE_SEQUENCEOF:
			break;
		default:
			snacc_exit("Unknown type %d", type);
	}
	return FALSE;
}

const char* GetNameSpace(Module* mod)
{
	if (!mod)
		return NULL;
	return mod->moduleName;
}

enum BasicTypeChoiceId GetBaseBasicTypeChoiceId(BasicType* basicType)
{
	BasicType* type = basicType;
	if (type->choiceId == BASICTYPE_LOCALTYPEREF)
		type = type->a.localTypeRef->link->type->basicType;
	else if (type->choiceId == BASICTYPE_IMPORTTYPEREF)
		type = type->a.importTypeRef->link->type->basicType;
	else if (type->choiceId == BASICTYPE_SEQUENCEOF)
		type = type->a.sequenceOf->basicType;
	else if (type->choiceId == BASICTYPE_SETOF)
		type = type->a.setOf->basicType;
	return type->choiceId;
}

BasicType* GetBaseBasicType(BasicType* type, const char** szName)
{
	BasicType* returnType = type;
	if (returnType->choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (szName)
			*szName = returnType->a.localTypeRef->typeName;
		returnType = returnType->a.localTypeRef->link->type->basicType;
	}
	else if (returnType->choiceId == BASICTYPE_IMPORTTYPEREF)
	{
		if (szName)
			*szName = returnType->a.importTypeRef->typeName;
		returnType = returnType->a.importTypeRef->link->type->basicType;
	}
	else if (returnType->choiceId == BASICTYPE_SEQUENCEOF)
	{
		returnType = returnType->a.sequenceOf->basicType;
		if (szName)
			GetBaseBasicType(returnType, szName);
	}
	else if (returnType->choiceId == BASICTYPE_SETOF)
	{
		returnType = returnType->a.setOf->basicType;
		if (szName)
			GetBaseBasicType(returnType, szName);
	}
	return returnType;
}

BasicType* ResolveArrayRootType(BasicType* type, const char** szName)
{
	BasicType* returnType = type;
	if (returnType->choiceId == BASICTYPE_SEQUENCEOF)
	{
		returnType = returnType->a.sequenceOf->basicType;
		if (szName)
			GetBaseBasicType(returnType, szName);
	}
	else if (returnType->choiceId == BASICTYPE_SETOF)
	{
		returnType = returnType->a.setOf->basicType;
		if (szName)
			GetBaseBasicType(returnType, szName);
	}
	return returnType;
}

char* GetImportFileName(char* Impname, ModuleList* mods)
{
	if (!mods)
	{
		snacc_exit("Invalid function argument mods = NULL");
		return NULL;
	}

	AsnListNode* stored = mods->curr;
	Module* currMod;
	char* fileName = NULL;
	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		/* Find the import Module in the Modules and
		 * return the header file name
		 */
		if (strcmp(Impname, currMod->modId->name) == 0)
		{
			/* Set the file name and break */
			fileName = currMod->cxxHdrFileName;
			break;
		}
	}
	mods->curr = stored;
	return fileName;
}

Module* GetImportModuleRefByClassName(const char* className, ModuleList* mods, Module* mod)
{
	if (!mods)
	{
		snacc_exit("Invalid argument, mods is NULL");
		return NULL;
	}

	AsnListNode* stored = mods->curr;
	Module* returnMod = NULL;
	{
		ImportModule* impMod;
		AsnListNode* storeImport = mod->imports->curr;
		FOR_EACH_LIST_ELMT(impMod, mod->imports)
		{
			bool bFound = false;
			ImportElmtList* ElemList = impMod->importElmts;
			if (!ElemList)
			{
				snacc_exit("Invalid argument, importElemtns is NULL");
				return NULL;
			}
			AsnListNode* elemStore = ElemList->curr;
			ImportElmt* impElem;
			FOR_EACH_LIST_ELMT(impElem, ElemList)
			{
				if (strcmp(impElem->name, className) == 0)
				{
					bFound = true;
					break;
				}
			}
			ElemList->curr = elemStore;
			if (bFound)
			{
				Module* currMod;
				FOR_EACH_LIST_ELMT(currMod, mods)
				{
					if (strcmp(impMod->modId->name, currMod->modId->name) == 0)
					{
						returnMod = currMod;
						break;
					}
				}
				break;
			}
		}
		if (!mod->imports)
		{
			snacc_exit("Invalid parameter, imports is NULL");
			return NULL;
		}
		mod->imports->curr = storeImport;
	}

	if (!returnMod)
	{
		TypeDef* typeDef;
		AsnListNode* storeTypeDef = mod->typeDefs->curr;
		FOR_EACH_LIST_ELMT(typeDef, mod->typeDefs)
		{
			if (strcmp(typeDef->definedName, className) == 0)
			{
				returnMod = mod;
				break;
			}
		}
		if (!mod->typeDefs)
		{
			snacc_exit("Invalid parameter, typeDefs is NULL");
			return NULL;
		}
		mod->typeDefs->curr = storeTypeDef;
		if (!returnMod)
		{
			fprintf(stderr, "GetImportModuleRefByClassName() - error unresolved reference");
			assert(FALSE);
		}
	}

	mods->curr = stored;

	return returnMod;
}

Module* GetModuleForImportModule(ModuleList* mods, ImportModule* impMod)
{
	if (!mods)
	{
		snacc_exit("Invalid argument, mods is NULL");
		return NULL;
	}

	Module* module = NULL;
	AsnListNode* saved = mods->curr;
	Module* currMod;
	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (strcmp(impMod->modId->name, currMod->modId->name) == 0)
		{
			module = currMod;
			break;
		}
	}
	mods->curr = saved;

	if (!module)
	{
		fprintf(stderr, "GetModuleForImportModule() - error unresolved reference");
		assert(FALSE);
	}

	return module;
}

bool IsDeprecatedFlaggedModule(Module* mod)
{
	asnmodulecomment comment;

	if (GetModuleComment_UTF8(mod->moduleName, &comment))
	{
		if (comment.i64Deprecated)
			return true;
	}

	return false;
}

bool IsDeprecatedFlaggedMember(Module* mod, const TypeDef* td, const char* szElement)
{
	enum BasicTypeChoiceId type = td->cxxTypeDefInfo->asn1TypeId;

	// Deprecated may get skipped for:
	// - choices
	// - enums
	// - sequences if the property is optional
	// -> so in any other case false, not skippable

	if (type != BASICTYPE_CHOICE && type != BASICTYPE_ENUMERATED && type != BASICTYPE_SEQUENCE)
		return false;

	asnmembercomment comment;
	if (GetMemberComment_UTF8(mod->moduleName, td->definedName, szElement, &comment))
	{
		if (comment.i64Deprecated)
			return true;
	}

	return false;
}

bool IsDeprecatedNoOutputModule(Module* mod)
{
	if (!gi64NoDeprecatedSymbols)
		return false;

	asnmodulecomment comment;
	if (GetModuleComment_UTF8(mod->moduleName, &comment))
		return IsDeprecatedNoOutput(comment.i64Deprecated);
	else
		return false;
}

bool IsDeprecatedNoOutputMember(Module* mod, const TypeDef* td, const char* szElement)
{
	if (!gi64NoDeprecatedSymbols)
		return false;

	enum BasicTypeChoiceId type = td->cxxTypeDefInfo->asn1TypeId;

	// Deprecated may get skipped for:
	// - choices
	// - enums
	// - sequences if the property is optional
	// -> so in any other case false, not skippable

	if (type != BASICTYPE_CHOICE && type != BASICTYPE_ENUMERATED && type != BASICTYPE_SEQUENCE)
		return false;

	asnmembercomment comment;
	if (GetMemberComment_UTF8(mod->moduleName, td->definedName, szElement, &comment))
	{
		if (IsDeprecatedNoOutput(comment.i64Deprecated))
		{
			if (type == BASICTYPE_SEQUENCE)
			{
				// We need to check if the property is optional, if that is the case we can skip it
				NamedType* e;
				bool bReturn = false;
				AsnListNode* curr = td->type->basicType->a.sequence->curr;
				FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
				{
					if (!e->fieldName)
						continue;
					if (strcmp(e->fieldName, szElement) != 0)
						continue;
					bReturn = e->type->optional;
					break;
				}
				if (!td->type->basicType->a.sequence)
				{
					snacc_exit("Invalid parameter, td->type->basicType->a.sequence is NULL");
					return false;
				}
				td->type->basicType->a.sequence->curr = curr;
				return bReturn;
			}
			else
				return true;
		}
	}

	return false;
}

bool IsDeprecatedFlaggedSequence(Module* mod, const char* szSequenceName)
{
	asnsequencecomment comment;
	if (GetSequenceComment_UTF8(mod->moduleName, szSequenceName, &comment))
	{
		if (comment.i64Deprecated)
			return true;
	}
	return false;
}

bool IsDeprecatedNoOutputSequence(Module* mod, const char* szSequenceName)
{
	if (!gi64NoDeprecatedSymbols)
		return false;

	asnsequencecomment comment;
	if (GetSequenceComment_UTF8(mod->moduleName, szSequenceName, &comment))
		return IsDeprecatedNoOutput(comment.i64Deprecated);
	else
		return false;
}

bool IsDeprecatedFlaggedOperation(Module* mod, const char* szOperationName)
{
	asnoperationcomment comment;
	if (GetOperationComment_UTF8(mod->moduleName, szOperationName, &comment))
	{
		if (comment.i64Deprecated)
			return true;
	}
	return false;
}

bool IsDeprecatedNoOutputOperation(Module* mod, const char* szOperationName)
{
	if (!gi64NoDeprecatedSymbols)
		return false;

	asnoperationcomment comment;
	if (GetOperationComment_UTF8(mod->moduleName, szOperationName, &comment))
		return IsDeprecatedNoOutput(comment.i64Deprecated);
	else
		return false;
}

int GetContextID(struct Type* type)
{
	int iResult = -1;
	if (type->tags->count)
	{
		Tag* pTag = NULL;
		FOR_EACH_LIST_ELMT(pTag, type->tags)
		{
			if (pTag->tclass == CNTX)
			{
				iResult = pTag->code;
				break;
			}
		}
	}
	return iResult;
}
