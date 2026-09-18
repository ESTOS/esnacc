#include "gen-ts-serializable.h"
#include "gen-ts-combined.h"
#include "gen-ts-converter.h"
#include "../str-util.h"
#include "../structure-util.h"
#include "../comment-util.h"
#include "../../core/asn_comments.h"
#include <assert.h>
#include <string.h>

// Returns the Redux-safe interface name for an ASN.1 type (caller frees).
static char* GetTSSerializableTypeName(const char* definedName)
{
	char* szConverted = FixName(definedName);
	const size_t nameLen = strlen(szConverted) + 2;
	char* szIface = malloc(nameLen);
	if (!szIface)
		snacc_exit("Out of memory");
	sprintf_s(szIface, nameLen, "I%s", szConverted);
	free(szConverted);
	return szIface;
}

void PrintTSSerializableComments(FILE* src, Module* m)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", RemovePath(m->tsSerializableFileName));
	fprintf(src, " * \"%s\" ASN.1 serializable type definitions for Redux-safe plain objects.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
	fprintf(src, DPRINT_DISABLE);
	fprintf(src, ESLINT_DISABLE);
}

void PrintTSSerializableImports(FILE* src, ModuleList* mods, Module* mod)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	if (!mod->imports)
		return;

	char szAlreadyAdded[4096] = {0};
	AsnListNode* saveImport = mod->imports->curr;
	ImportModule* impMod;
	FOR_EACH_LIST_ELMT(impMod, mod->imports)
	{
		Module* referencedModule = GetModuleForImportModule(mods, impMod);
		if (!referencedModule || !ContainsSerializables(referencedModule))
			continue;

		const char* szNameSpace = GetNameSpace(referencedModule);
		if (strstr(szAlreadyAdded, szNameSpace) != NULL)
			continue;

		strcat_s(szAlreadyAdded, sizeof(szAlreadyAdded), szNameSpace);
		fprintf(src, "import type * as %s_Serializable from \"./%s_Serializable%s\";\n", szNameSpace, referencedModule->moduleName, getCommonJSFileExtension());
	}
	mod->imports->curr = saveImport;
}

/**
 * Emit the serializable TypeScript type for a member field (primitive mapping applied).
 */
void PrintTSSerializableMemberType(FILE* hdr, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	Module* mod = NULL;
	enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
	if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			choiceId = BASICTYPE_UTCTIME;
		else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
			{
				mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
				t = t->basicType->a.importTypeRef->link->type;
				choiceId = t->basicType->choiceId;
			}
		}
		else if (choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
			{
				t = t->basicType->a.localTypeRef->link->type;
				choiceId = t->basicType->choiceId;
			}
		}
	}
	if (choiceId == BASICTYPE_OCTETCONTAINING && t->basicType->a.stringContaining->basicType->choiceId == BASICTYPE_UTF8_STR)
		choiceId = BASICTYPE_UTF8_STR;

	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "boolean");
			break;
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "number");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_SEQUENCEOF:
			{
				char* szElementIface = GetTSSerializableTypeName(t->cxxTypeRefInfo->className);
				fprintf(hdr, "%s[]", szElementIface);
				free(szElementIface);
			}
			break;
		case BASICTYPE_NULL:
			fprintf(hdr, "null");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "string");
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				mod = GetImportModuleRefByClassName(t->cxxTypeRefInfo->className, mods, m);
				if (mod)
				{
					char* szIface = GetTSSerializableTypeName(t->cxxTypeRefInfo->className);
					fprintf(hdr, "%s_Serializable.%s", GetNameSpace(mod), szIface);
					free(szIface);
				}
				break;
			}
		case BASICTYPE_LOCALTYPEREF:
			{
				char* szIface = GetTSSerializableTypeName(t->basicType->a.localTypeRef->link->definedName);
				fprintf(hdr, "%s", szIface);
				free(szIface);
				break;
			}
		case BASICTYPE_ANY:
			fprintf(hdr, "unknown");
			break;
		default:
			snacc_exit("Unknown choiceId %d", choiceId);
	}
}

void PrintTSSerializableChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* choice, int novolatilefuncs)
{
	NamedType* e;
	char* szIface = GetTSSerializableTypeName(td->definedName);

	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "export interface %s {\n", szIface);
	fprintf(src, "\t/** Optional ASN.1 type discriminator when converters encode with bAddTypes. */\n");
	fprintf(src, "\t_type?: string;\n");

	FOR_EACH_LIST_ELMT(e, choice->basicType->a.sequence)
	{
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		if (choice->basicType->a.sequence->curr->prev)
			fprintf(src, "\n");

		printMemberComment(src, m, td, e->fieldName, "\t", COMMENTSTYLE_TYPESCRIPT);

		char* szFieldName = FixName(e->fieldName);
		fprintf(src, "\t%s?: ", szFieldName);
		PrintTSSerializableMemberType(src, mods, m, td, choice, e->type);
		fprintf(src, ";\n");
		free(szFieldName);
	}

	fprintf(src, "}\n");
	free(szIface);
}

void PrintTSSerializableSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* seq, int novolatilefuncs)
{
	NamedType* e;
	char* szIface = GetTSSerializableTypeName(td->definedName);

	printSequenceComment(src, m, td, COMMENTSTYLE_TYPESCRIPT);
	fprintf(src, "// [%s]\n", __FUNCTION__);
	fprintf(src, "export interface %s {\n", szIface);
	fprintf(src, "\t/** Optional ASN.1 type discriminator when converters encode with bAddTypes. */\n");
	fprintf(src, "\t_type?: string;\n");

	bool bFirst = true;
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;
		if (IsDeprecatedNoOutputMember(m, td, e->fieldName))
			continue;

		if (!bFirst)
			fprintf(src, "\n");
		bFirst = false;

		printMemberComment(src, m, td, e->fieldName, "\t", COMMENTSTYLE_TYPESCRIPT);

		char* szFieldName = FixName(e->fieldName);
		fprintf(src, "\t%s", szFieldName);
		if (e->type->optional)
			fprintf(src, "?");
		fprintf(src, ": ");
		PrintTSSerializableMemberType(src, mods, m, td, seq, e->type);
		fprintf(src, ";\n");
		free(szFieldName);
	}

	fprintf(src, "}\n");
	free(szIface);
}

void PrintTSSerializableListType(FILE* src, TypeDef* td, Type* type, Module* m, ModuleList* mods)
{
	char* szIface = GetTSSerializableTypeName(td->cxxTypeDefInfo->className);
	const char* szBaseNameArg = NULL;
	char* szBaseName = NULL;
	BasicType* pBase = GetBaseBasicType(type->basicType, &szBaseNameArg);

	fprintf(src, "// [%s]\n", __FUNCTION__);

	switch (pBase->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
			fprintf(src, "export type %s = ", szIface);
			if (pBase->choiceId == BASICTYPE_OCTETSTRING || pBase->choiceId == BASICTYPE_OCTETCONTAINING)
				fprintf(src, "string[]");
			else if (pBase->choiceId == BASICTYPE_BOOLEAN)
				fprintf(src, "boolean[]");
			else if (pBase->choiceId == BASICTYPE_UTF8_STR)
				fprintf(src, "string[]");
			else
				fprintf(src, "number[]");
			fprintf(src, ";\n");
			break;
		case BASICTYPE_LOCALTYPEREF:
			szBaseName = FixName(szBaseNameArg);
			fprintf(src, "export type %s = I%s[];\n", szIface, szBaseName);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				Module* mod = GetImportModuleRefByClassName(szBaseNameArg, mods, m);
				if (mod)
				{
					char* szElementIface = GetTSSerializableTypeName(szBaseNameArg);
					fprintf(src, "export type %s = %s_Serializable.%s[];\n", szIface, GetNameSpace(mod), szElementIface);
					free(szElementIface);
				}
				break;
			}
		default:
			snacc_exit("unsupported choice %i in [PrintTSSerializableListType]", pBase->choiceId);
	}

	if (szBaseName)
		free(szBaseName);
	free(szIface);
}

void PrintTSSerializableSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* setOf, int novolatilefuncs)
{
	PrintTSSerializableListType(src, td, setOf, m, mods);
}

void PrintTSSerializableTypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);
	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_REAL:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_INTEGER:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_ENUMERATED:
			break;
		case BASICTYPE_SEQUENCEOF:
		case BASICTYPE_SETOF:
			PrintTSSerializableSetOfDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				Module* mod = GetImportModuleRefByClassName(td->type->basicType->a.importTypeRef->typeName, mods, m);
				if (mod)
				{
					char* szIface = GetTSSerializableTypeName(td->definedName);
					char* szImportedIface = GetTSSerializableTypeName(td->type->basicType->a.importTypeRef->typeName);
					fprintf(src, "export type %s = %s_Serializable.%s;\n", szIface, GetNameSpace(mod), szImportedIface);
					free(szImportedIface);
					free(szIface);
				}
			}
			break;
		case BASICTYPE_LOCALTYPEREF:
			{
				char* szIface = GetTSSerializableTypeName(td->definedName);
				char* szLocalIface = GetTSSerializableTypeName(td->type->basicType->a.localTypeRef->typeName);
				fprintf(src, "export type %s = %s;\n", szIface, szLocalIface);
				free(szLocalIface);
				free(szIface);
			}
			break;
		case BASICTYPE_CHOICE:
			PrintTSSerializableChoiceDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintTSSerializableSeqDefCode(src, mods, m, td, td->type, novolatilefuncs);
			break;
		default:
			break;
	}
}

bool ContainsSerializables(Module* m)
{
	return ContainsConverters(m);
}

void PrintTSSerializableCode(FILE* src, ModuleList* mods, Module* m, int novolatilefuncs)
{
	fprintf(src, "// [%s]\n", __FUNCTION__);

	PrintTSSerializableComments(src, m);
	PrintTSSerializableImports(src, mods, m);
	fprintf(src, "\n");

	bool bIsFirst = true;
	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			if (!bIsFirst)
				continue;

		enum BasicTypeChoiceId type = td->type->basicType->choiceId;
		if (IsSimpleType(type) || ResolveTypeReferencesToRoot(td->type, NULL)->basicType->choiceId == BASICTYPE_ENUMERATED)
			continue;

		if (!bIsFirst)
			fprintf(src, "\n");
		PrintTSSerializableTypeDefCode(src, mods, m, td, novolatilefuncs);
		bIsFirst = false;
	}
}
