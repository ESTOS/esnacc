/*
*   compiler/back_ends/HJS_gen/gen_hjs_code.c - routines for printing C++ code from type trees
*
*   assumes that the type tree has already been run through the
*   c++ type generator (c++_gen/types.c).
*
*  This was hastily written - it has some huge routines in it.
*  Needs a lot of cleaning up and modularization...
*
* Mike Sample
* 92
* Copyright (C) 1991, 1992 Michael Sample
*           and the University of British Columbia
*
* 2014 ESTOS/stm
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
*/

#include "gen-hjs-code.h"

#include "../../../c-lib/include/print.h"
#include "../tag-util.h"  /* get GetTags/FreeTags/CountTags/TagByteLen */
#include "../str-util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../core/asn_comments.h"

// implementation handled inside the gen-ts-code.c
extern char* str_replace(const char* string, const char* substr, const char* replacement);

void printCommentJS(FILE* src, const char* szPrefix, const char* szString, const char* szSuffix)
{
	fprintf(src, "%s", szPrefix);

	const char* posBegin = szString;
	const char* posEnd = strstr(szString, "\\n");
	if (posEnd)
	{
		size_t stSize = strlen(szString) + 10;
		char* szBuffer = malloc(stSize);
		if (szBuffer)
		{
			int iFirst = 1;
			while (posEnd != NULL)
			{
				memset(szBuffer, 0x00, stSize);
				memcpy(szBuffer, posBegin, posEnd - posBegin);
				char* szBuffer2 = str_replace(szBuffer, "\\t", "\t");
				if (!iFirst)
					fprintf(src, "\n *");

				if (strlen(szBuffer2))
					fprintf(src, " ");

				fprintf(src, "%s", szBuffer2);
				free(szBuffer2);
				iFirst = 0;
				posBegin = posEnd + 2;
				posEnd = strstr(posBegin, "\\n");
			}
			free(szBuffer);
		}
	}
	else
	{
		fprintf(src, "%s", szString);
	}

	fprintf(src, "%s", szSuffix);
}


static Module *GetImportModuleRef(char *Impname, ModuleList *mods)
{
	Module *currMod = NULL;
	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		/* Find the import Module in the Modules and
		* return the header file name
		*/
		if ((strcmp(Impname, currMod->modId->name) == 0))
		{
			break;
		}
	}
	return currMod;
}

static ImportModule *FindHJSModule(char *definedName, ModuleList *mods, Module *mod) {
	ImportModuleList *ModLists;
	ImportModule *impMod;

	ModLists = mod->imports;

	FOR_EACH_LIST_ELMT(impMod, ModLists)
	{
		ImportElmt *impElmt;

		if (impMod->moduleRef == NULL)
			impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);

		FOR_EACH_LIST_ELMT(impElmt, impMod->importElmts)
		{
			if (strcmp(definedName, impElmt->name) == 0) {
				return impMod;
			}
		}

	}

	return NULL;
}

static FILE *getHJSFilePointer(char* pPrefix, Module *mod)
{
	FILE *p = NULL;
	char* dir = mod->hjsFileName;
	size_t size = strlen(dir) + 3 + strlen(pPrefix) + 4;
	char* fileName = malloc(size); // dir + \ + filename + .js + /0

	sprintf_s(fileName, size, "%s/%s.js", dir, pPrefix);

#ifdef _WIN32
	if (fopen_s(&p, fileName, "wb") != 0 || p == NULL)
	{
		printf("\nERROR: Failed to open file '%s'\n", fileName);
		perror("fopen error");
	}
#else // _WIN32
	p = fopen(fileName, "wb");
	if (p == NULL)
	{
		printf("\nERROR: Failed to open file '%s'\n", fileName);
		perror("fopen error");
	}
#endif // _WIN32

	free(fileName);

	return p;
}

static char* getHJSClassName(char* prefix, char* suffix)
{
	size_t size = strlen(prefix) + strlen(suffix) + 1;
	char* className = malloc(size); // strlen + /0

	sprintf_s(className, size, "%s%s", prefix, suffix);
	className[0] = (char)toupper(className[0]);

	return className;
}

static int IsBackboneCollection(TypeDef *td, Type *t)
{
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF || t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		&& t->basicType->a.localTypeRef->link != NULL
		&& t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF
		&& strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") != 0
		&& strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") != 0
		&& strcmp(t->cxxTypeRefInfo->className, "SEQInteger") != 0
		&& strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") != 0
		)
	{
		return 1;
	}
	else
		return 0;

} /* IsBackboneCollection */




static void PrintHJSComments(FILE *src, TypeDef *td, Module *m) {

	fprintf(src, "/**\n");
	fprintf(src, " * [PrintHJSComments] %s\n", td->definedName);
	fprintf(src, " *\n");
	fprintf(src, " * NOTE: This is a machine generated file - editing not recommended\n");
	fprintf(src, " */\n");

	asnsequencecomment sequenceComment;
	if (GetSequenceComment_UTF8(m->className, td->definedName, &sequenceComment) && (strlen(sequenceComment.szShort) || strlen(sequenceComment.szLong)))
	{
		fprintf(src, "/**\n");
		if (strlen(sequenceComment.szShort))
			printCommentJS(src, " * ", sequenceComment.szShort, "\n");
		if (strlen(sequenceComment.szLong))
			printCommentJS(src, " * @remarks ", sequenceComment.szLong, "\n");
		if (sequenceComment.iDeprecated)
			fprintf(src, " * @deprecated *\n");
		if (sequenceComment.iPrivate)
			fprintf(src, " * @private\n");
		fprintf(src, " */\n");
	}
}

static void PrintHJSImports(FILE *src, TypeDef *td, ModuleList *mods, Module *mod)
{
	NamedType *e;

	fprintf(src, "var HumanModel = require('../../lib/human-model');\n\n");

	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		char* className = e->type->cxxTypeRefInfo->className;

		if (strcmp(className, "AsnOptionalParameters") == 0
			|| strcmp(className, "UTF8StringList") == 0
			|| strcmp(className, "SEQInteger") == 0
			|| strcmp(className, "AsnContactIDs") == 0
			|| strcmp(className, "AsnInt") == 0
			|| strcmp(className, "AsnSystemTime") == 0)
		{
			continue;
		}

		if (e->type->basicType != NULL && e->type->basicType->a.localTypeRef != NULL && e->type->basicType->a.localTypeRef->module != NULL) {
			fprintf(src, "var %s = require('./%s');\n", className, className);
		}
		else if (e->type->basicType != NULL && e->type->basicType->a.importTypeRef != NULL) {
			ImportModule *impMod = FindHJSModule(className, mods, mod);

			if (impMod != NULL) {
				fprintf(src, "var %s = require('../%s/%s');\n", className, impMod->moduleRef->hjsFileName, className);
			}
			else {
				fprintf(src, "// No module found for %s\n", className);
			}
		}
	}

	fprintf(src, "\n");
}

static void PrintHJSNativeType(FILE *hdr, int basicTypeChoiseId) {
	switch (basicTypeChoiseId) {
	case BASICTYPE_BOOLEAN:
		fprintf(hdr, "boolean");
		break;
	case BASICTYPE_INTEGER:
		fprintf(hdr, "number");
		break;
	case BASICTYPE_OCTETSTRING:
	case BASICTYPE_OCTETCONTAINING:
		fprintf(hdr, "string");
		break;
	case BASICTYPE_ENUMERATED:
		fprintf(hdr, "number"); //FIXME
		break;
	case BASICTYPE_REAL:
		fprintf(hdr, "number");
		break;
	case BASICTYPE_UTF8_STR:
		fprintf(hdr, "string");
		break;
	case BASICTYPE_UTCTIME:
		fprintf(hdr, "string");
		break;
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_NULL:
		fprintf(hdr, "number");
		break;
	default:
		exit(1);
		break;
	}
}


static void PrintHJSDefaultValue(FILE *hdr, TypeDef *td, Type *t)
{
	// fprintf(hdr, "[PrintHJSDefaultValue] ");

	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF || t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		&& t->basicType->a.localTypeRef->link != NULL
		&& t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF)
	{
		// Use HumanModel default which is a new array or object instance
		/*
		if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(td->definedName, "AsnContactIDs") == 0) {
			fprintf(hdr, ", default: []");
		}

		else {
			fprintf(hdr, ", default: {}");
		}
		*/

	}
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF || t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		&& t->basicType->a.localTypeRef->link != NULL
		&& t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		// fprintf(hdr, "[ENUMERATED] ");
		CNamedElmt *n;
		Type *enumType = t->basicType->a.localTypeRef->link->type;
		if (HasNamedElmts(enumType) != 0)
		{
			FOR_EACH_LIST_ELMT_NOITERATE(n, enumType->cxxTypeRefInfo->namedElmts)
			{
				fprintf(hdr, ", default: %s.%s", t->cxxTypeRefInfo->className, n->name);
				break;
			}
		}
	}
	else
	{
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId)
		{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, ", default: false");
			break;
		case BASICTYPE_INTEGER:
			fprintf(hdr, ", default: 0");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, ", default: ''");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, ", default: 0"); // FIXME
			break;
		case BASICTYPE_SEQUENCEOF: // Array
			// Use default from HumenModel which is a new Array instance
			//fprintf(hdr, "[]");//  , t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, ", default: 0.0");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, ", default: ''");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, ", default: ''");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
				fprintf(hdr, ", default: 0");
			break;
		case BASICTYPE_IMPORTTYPEREF:
		case BASICTYPE_LOCALTYPEREF:
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			{
				fprintf(hdr, ", default: ''"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im JS als String �bermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0)
			{
				fprintf(hdr, ", default: ''");
			}
			else
			{
				// Use default from HumanModel which is a new object instance
				//fprintf(hdr, "{}");
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] {}"); //  , t->cxxTypeRefInfo->className);
			break;
		}
	}
}

static void PrintHJSType(FILE *hdr, TypeDef *td, Type *t)
{
	fprintf(hdr, "{type: '");


	// fprintf(hdr, "[PrintHJSType] ");
	if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF || t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		&& t->basicType->a.localTypeRef->link != NULL
		&& t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF)
	{
		// fprintf(hdr, "[SEQUENCE OF] ");
		if (strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		{
			fprintf(hdr, "object"); // AsnOptionalParameters are objects not arrays
		}
		else if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") == 0) {
			fprintf(hdr, "array");
		}
		else
		{
			fprintf(hdr, "object");
			fprintf(hdr, "', hmType: '%s", t->cxxTypeRefInfo->className);
		}
	}
	else if ((t->basicType->choiceId == BASICTYPE_LOCALTYPEREF || t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
		&& t->basicType->a.localTypeRef->link != NULL
		&& t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		// fprintf(hdr, "[ENUMERATED] ");
		fprintf(hdr, "number");
	}
	else
	{
		// fprintf(hdr, "[BASIC TYPE] ");
		switch (t->basicType->choiceId) {
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_UTCTIME:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			PrintHJSNativeType(hdr, t->basicType->choiceId);
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(hdr, "%s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_EXTENSION:
			fprintf(hdr, "object");
			break;
		case BASICTYPE_IMPORTTYPEREF:
		case BASICTYPE_LOCALTYPEREF:
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			{
				fprintf(hdr, "string"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im JS als String �bermittelt.
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnContactID") == 0)
			{
				fprintf(hdr, "string");
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
			{
				fprintf(hdr, "object"); // AsnOptionalParameters are objects not arrays
			}
			else if (strcmp(t->cxxTypeRefInfo->className, "UTF8StringList") == 0 || strcmp(t->cxxTypeRefInfo->className, "SEQInteger") == 0 || strcmp(t->cxxTypeRefInfo->className, "AsnContactIDs") == 0) {
				fprintf(hdr, "array");
			}
			else
			{
				fprintf(hdr, "object");
				fprintf(hdr, "', hmType: '%s", t->cxxTypeRefInfo->className);
			}
			break;
		default:
			fprintf(hdr, "[UNKNOWN BASIC TYPE] %s", t->cxxTypeRefInfo->className);
			break;
		}
	}

	fprintf(hdr, "'");
} /* PrintCxxType */


static void PrintHJSBitstringDefCode(TypeDef *td, Module *mod)
{
	CNamedElmt *n;
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);

	PrintHJSComments(src, td, mod);
	char* szConverted = FixName(td->definedName);
	fprintf(src, "// [PrintHJSBitstringDefCode] %s\n", szConverted);
	free(szConverted);

	{
		asnsequencecomment sequenceComment;
		if (GetSequenceComment_UTF8(mod->className, td->definedName, &sequenceComment) && (strlen(sequenceComment.szShort) || strlen(sequenceComment.szLong)))
		{
			fprintf(src, "/**\n");
			if (strlen(sequenceComment.szShort))
				printCommentJS(src, " * @remarks ", sequenceComment.szShort, "\n");
			if (strlen(sequenceComment.szLong))
				printCommentJS(src, " * @remarks ", sequenceComment.szLong, "\n");
			if (sequenceComment.iDeprecated)
				fprintf(src, " * @deprecated *\n");
			if (sequenceComment.iPrivate)
				fprintf(src, " * @private\n");
			fprintf(src, " */\n");
		}
	}

	fprintf(src, "var %s = {\n", td->definedName);
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			asnmembercomment comment;
			if (GetMemberComment_UTF8(mod->className, td->definedName, n->name, &comment) && strlen(comment.szShort))
			{
				int iMultiline = comment.iDeprecated || comment.iPrivate;
				const char* szRemarksPrefix = iMultiline ? "\t * " : "\t/** ";
				const char* prefix = iMultiline ? "\t * " : "\t/** ";
				const char* suffix = iMultiline ? "\n" : " */\n";
				if (iMultiline)
					fprintf(src, "\t/**\n");

				printCommentJS(src, szRemarksPrefix, comment.szShort, suffix);

				if (comment.iDeprecated)
					fprintf(src, "%s@deprecated *%s", prefix, suffix);

				if (comment.iPrivate)
					fprintf(src, "%s@private%s", prefix, suffix);

				if (iMultiline)
					fprintf(src, "\t*/\n");
			}

			fprintf(src, "  %s: %d", n->name, 0x00000001 << n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}

	/* close class definition */
	fprintf(src, "};\n\n\n");

	fprintf(src, "module.exports = %s;", td->definedName);
	fclose(src);
	free(name);

} /* PrintHJSBitstringDefCode */

static void PrintHJSEnumDefCode(TypeDef *td, Module *mod)
{
	CNamedElmt *n;
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);

	fprintf(src, "var HumanModel = require('../../lib/human-model');\n\n");

	PrintHJSComments(src, td, mod);
	char* szConverted = FixName(td->definedName);
	fprintf(src, "// [PrintHJSEnumDefCode] %s\n", szConverted);
	free(szConverted);

	fprintf(src, "var %s = HumanModel.register('%s', {\n", td->definedName, td->definedName);
	if (HasNamedElmts(td->type) != 0) {
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			asnmembercomment memberComment;
			if (GetMemberComment_UTF8(mod->className, td->definedName, n->name, &memberComment) && (strlen(memberComment.szShort)))
			{
				int iMultiline = memberComment.iDeprecated || memberComment.iPrivate;
				const char* szRemarksPrefix = iMultiline ? "\t * " : "\t/** ";
				const char* prefix = iMultiline ? "\t * " : "\t/** ";
				const char* suffix = iMultiline ? "\n" : " */\n";
				if (iMultiline)
					fprintf(src, "\t/**\n");

				printCommentJS(src, szRemarksPrefix, memberComment.szShort, suffix);

				if (memberComment.iDeprecated)
					fprintf(src, "%s@deprecated *%s", prefix, suffix);

				if (memberComment.iPrivate)
					fprintf(src, "%s@private%s", prefix, suffix);

				if (iMultiline)
					fprintf(src, "\t*/\n");
			}

			fprintf(src, "\t %s: %d", n->name, n->value);
			if (((td->type->cxxTypeRefInfo->namedElmts)->curr->next && ((td->type->cxxTypeRefInfo->namedElmts)->curr->next->data) != NULL))
				fprintf(src, ",");
			fprintf(src, "\n");
		}
	}

	/* close class definition */
	fprintf(src, "});\n\n");

	fprintf(src, "module.exports = %s;", td->definedName);

	fclose(src);
	free(name);

} /* PrintHJSEnumDefCode */



static void PrintHJSModuleCode(Module* mod)
{
	asnmodulecomment moduleComment;
	if (GetModuleComment_UTF8(mod->className, &moduleComment) && (strlen(moduleComment.szLong) || strlen(moduleComment.szShort)))
	{
		FILE* src = getHJSFilePointer(mod->className, mod);
		fprintf(src, "/**\n");
		fprintf(src, " * [ModuleName] %s\n", moduleComment.szModuleName);
		fprintf(src, " * [Category]   %s\n", moduleComment.szCategory);
		if (strlen(moduleComment.szShort))
			printCommentJS(src, " * ", moduleComment.szShort, "\n");
		if (strlen(moduleComment.szLong))
			printCommentJS(src, " * @remarks", moduleComment.szLong, "\n");
		if (moduleComment.iDeprecated)
			fprintf(src, " * @deprecated *\n");
		if (moduleComment.iPrivate)
			fprintf(src, " * @private\n");
		fprintf(src, " */\n");
		fclose(src);
	}
} /* PrintHJSModuleCode */



static void PrintHJSChoiceDefCode(TypeDef *td, ModuleList *mods, Module *mod)
{
	NamedType *e;
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);
	NamedType **pSeqElementNamedType = NULL;
	int propertyCounter = 0;

	PrintHJSImports(src, td, mods, mod);

	PrintHJSComments(src, td, mod);
	fprintf(src, "// [PrintHJSChoiceDefCode]\n");

	fprintf(src, "var %s = HumanModel.define({\n", td->definedName);
	fprintf(src, "\ttype: '%s',\n", td->definedName);
	fprintf(src, "\tprops: {\n");

	/* Write out properties */
	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
 		if (IsBackboneCollection(td, e->type) != 0) {
			continue;
		}

		if (propertyCounter > 0) {
			fprintf(src, ",");
			fprintf(src, "\n");
		}

		asnmembercomment comment;
		if (GetMemberComment_UTF8(mod->className, td->definedName, e->fieldName, &comment) && strlen(comment.szShort))
		{
			int iMultiline = comment.iDeprecated || comment.iPrivate;
			const char* szRemarksPrefix = iMultiline ? "\t\t * " : "\t\t/** ";
			const char* prefix = iMultiline ? "\t\t * " : "\t\t/** ";
			const char* suffix = iMultiline ? "\n" : " */\n";
			if (iMultiline)
				fprintf(src, "\t\t/**\n");

			printCommentJS(src, szRemarksPrefix, comment.szShort, suffix);

			if (comment.iDeprecated)
				fprintf(src, "%s@deprecated *%s", prefix, suffix);

			if (comment.iPrivate)
				fprintf(src, "%s@private%s", prefix, suffix);

			if (iMultiline)
				fprintf(src, "\t\t*/\n");
		}

		fprintf(src, "\t\t");
		fprintf(src, "%s: ", e->fieldName);
		PrintHJSType(src, td, e->type);

		fprintf(src, "}");

		propertyCounter++;
	}

	fprintf(src, "\n\t}\n");

	if (pSeqElementNamedType)
		free(pSeqElementNamedType);

	/* close class definition */
	fprintf(src, "});\n\n");

	fprintf(src, "module.exports = %s;", td->definedName);

	fclose(src);
	free(name);

} /* PrintCxxChoiceDefCode */


static void PrintHJSSeqDefCode(TypeDef *td, ModuleList *mods, Module *mod)
{
	NamedType *e;
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);
	int propertyCounter = 0;
	int collectionCounter = 0;
	int collectionBracketOpened = 0;

	PrintHJSImports(src, td, mods, mod);

	PrintHJSComments(src, td, mod);
	fprintf(src, "// [PrintHJSSeqDefCode]\n");

	fprintf(src, "var %s = HumanModel.define({\n", td->definedName);
	fprintf(src, "\ttype: '%s',\n", td->definedName);

	fprintf(src, "\tprops: {\n");

	/* Write out properties */
	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		if (IsBackboneCollection(td, e->type) != 0) {
			continue;
		}

		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION) {
			continue;
		}

		if (propertyCounter > 0) {
			fprintf(src, ",");
			fprintf(src, "\n");
		}

		asnmembercomment comment;
		if (GetMemberComment_UTF8(mod->className, td->definedName, e->fieldName, &comment) && strlen(comment.szShort))
		{
			fprintf(src, "\n");
			int iMultiline = comment.iDeprecated || comment.iPrivate;
			const char* szRemarksPrefix = iMultiline ? "\t\t * @remarks " : "\t\t/** @remarks ";
			const char* prefix = iMultiline ? "\t\t * " : "\t\t/** ";
			const char* suffix = iMultiline ? "\n" : " */\n";
			if (iMultiline)
				fprintf(src, "\t\t/**\n");

			printCommentJS(src, szRemarksPrefix, comment.szShort, suffix);

			if (comment.iDeprecated)
				fprintf(src, "%s@deprecated *%s", prefix, suffix);

			if (comment.iPrivate)
				fprintf(src, "%s@private%s", prefix, suffix);

			if (iMultiline)
				fprintf(src, "\t\t*/\n");
		}


		fprintf(src, "\t\t");
		fprintf(src, "%s: ", e->fieldName);

		PrintHJSType(src, td, e->type);

		if (e->type->optional) {
			// required: false ist in HumanModel der Default
			// fprintf(src, "', required: false");
		}
		else {
			fprintf(src, ", required: true ");
			PrintHJSDefaultValue(src, td, e->type);
		}

		fprintf(src, "}");
		propertyCounter++;
	}

	fprintf(src, "\n\t}");

	/* Write out collections */
	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		if (IsBackboneCollection(td, e->type) == 0) {
			continue;
		}

		if (collectionCounter == 0) {
			fprintf(src, ",\n\tcollections: {\n");
			collectionBracketOpened = 1;
		}
		else {
			fprintf(src, ",");
			fprintf(src, "\n");
		}

		asnmembercomment comment;
		if (GetMemberComment_UTF8(mod->className, td->definedName, e->fieldName, &comment) && strlen(comment.szShort))
		{
			int iMultiline = comment.iDeprecated || comment.iPrivate;
			const char* szRemarksPrefix = iMultiline ? "\t\t * " : "\t\t/** ";
			const char* prefix = iMultiline ? "\t\t * " : "\t\t/** ";
			const char* suffix = iMultiline ? "\n" : " */\n";
			if (iMultiline)
				fprintf(src, "\t\t/**\n");

			printCommentJS(src, szRemarksPrefix, comment.szShort, suffix);

			if (comment.iDeprecated)
				fprintf(src, "%s@deprecated *%s", prefix, suffix);

			if (comment.iPrivate)
				fprintf(src, "%s@private%s", prefix, suffix);

			if (iMultiline)
				fprintf(src, "\t\t*/\n");
		}

		// vars
		fprintf(src, "\t\t");
		fprintf(src, "%s: %s", e->fieldName, e->type->cxxTypeRefInfo->className);

		collectionCounter++;
	}

	if (collectionBracketOpened > 0) {
		fprintf(src, "\n\t}");
	}

	fprintf(src, "\n");

	/* close class definition */
	fprintf(src, "});\n\n");

	fprintf(src, "module.exports = %s;", td->definedName);

	fclose(src);
	free(name);

} /* PrintCxxSeqDefCode */

static void PrintHJSListClass(TypeDef *td, ModuleList *mods, Module *mod)
{
	struct NamedType p_etemp;
	NamedType* p_e;
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);

	p_e = &p_etemp;
	p_e->type = td->type->basicType->a.setOf;

	switch (td->type->basicType->a.setOf->basicType->choiceId) {
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_UTCTIME:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(src, "//  %s [No collections of primitive Types %s]\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
			return;
		default:
			break;
	}
	if (strcmp(p_e->type->cxxTypeRefInfo->className, "AsnContactID") == 0)
	{
		fprintf(src, "//  %s [No collections of primitive Types %s]\n", td->cxxTypeDefInfo->className, p_e->type->cxxTypeRefInfo->className);
		return;
	}

	fprintf(src, "var HumanModel = require('../../lib/human-model');\n\n");
	fprintf(src, "var %s = require('./%s');\n\n", p_e->type->cxxTypeRefInfo->className, p_e->type->cxxTypeRefInfo->className);

	PrintHJSComments(src, td, mod);
	fprintf(src, "// [PrintHJSListClass]\n");

	fprintf(src, "var %s = HumanModel.defineCollection({\n", td->cxxTypeDefInfo->className);
	fprintf(src, "\ttype: '%s',\n", td->cxxTypeDefInfo->className);
	fprintf(src, "\tmodel: %s\n", p_e->type->cxxTypeRefInfo->className);
	fprintf(src, "});\n\n");

	fprintf(src, "module.exports = %s;", td->definedName);
	fclose(src);
	free(name);
}

static void PrintHJSAlias(TypeDef *td, ModuleList *mods, Module *mod)
{
	char* name = getHJSClassName(td->definedName, "");
	FILE* src = getHJSFilePointer(name, mod);
	char* className = td->type->cxxTypeRefInfo->className;
	ImportModule *impMod;

	fprintf(src, "var HumanModel = require('../../lib/human-model');\n\n");

	switch (td->type->basicType->choiceId) {
	case BASICTYPE_IMPORTTYPEREF:  /* type references */
		impMod = FindHJSModule(className, mods, mod);

		if (impMod != NULL) {
			fprintf(src, "var %s = require('../%s/%s');\n\n", className, impMod->moduleRef->hjsFileName, className);
		}
		else {
			fprintf(src, "// No module found for %s\n\n", className);
		}
		break;
	case BASICTYPE_LOCALTYPEREF:
		fprintf(src, "var %s = require('./%s');\n\n", className, className);
		break;
	default:
		break;
	}

	PrintHJSComments(src, td, mod);
	fprintf(src, "// [PrintHJSAlias]\n");

	fprintf(src, "HumanModel.register('%s', %s);\n\n", td->definedName, className);

	fprintf(src, "module.exports = %s;", className);
	fclose(src);
	free(name);
}

static void PrintHJSSetOfDefCode(TypeDef *td, ModuleList *mods, Module *mod)
{
	if (strcmp(td->cxxTypeDefInfo->className, "AsnOptionalParameters") == 0) // ESTOS special 'native Object' AsnOptionalParamaters
		return;

	PrintHJSListClass(td, mods, mod);

} /* PrintHJSSetOfDefCode */


static void PrintHJSTypeDefCode(TypeDef *td, ModuleList *mods, Module *mod)
{
	switch (td->type->basicType->choiceId)
	{
	case BASICTYPE_BOOLEAN:  /* library type */
	case BASICTYPE_REAL:  /* library type */
	case BASICTYPE_OCTETSTRING:  /* library type */
	case BASICTYPE_OCTETCONTAINING:
	case BASICTYPE_NULL:  /* library type */
	case BASICTYPE_EXTERNAL:		/* library type */
	case BASICTYPE_OID:  /* library type */
	case BASICTYPE_RELATIVE_OID:
	case BASICTYPE_INTEGER:  /* library type */
	case BASICTYPE_NUMERIC_STR:  /* 22 */
	case BASICTYPE_PRINTABLE_STR: /* 23 */
	case BASICTYPE_UNIVERSAL_STR: /* 24 */
	case BASICTYPE_IA5_STR:      /* 25 */
	case BASICTYPE_BMP_STR:      /* 26 */
	case BASICTYPE_UTF8_STR:     /* 27 */
	case BASICTYPE_UTCTIME:      /* 28 tag 23 */
	case BASICTYPE_GENERALIZEDTIME: /* 29 tag 24 */
	case BASICTYPE_GRAPHIC_STR:     /* 30 tag 25 */
	case BASICTYPE_VISIBLE_STR:     /* 31 tag 26  aka ISO646String */
	case BASICTYPE_GENERAL_STR:     /* 32 tag 27 */
	case BASICTYPE_OBJECTDESCRIPTOR:	/* 33 tag 7 */
	case BASICTYPE_VIDEOTEX_STR:	/* 34 tag 21 */
	case BASICTYPE_T61_STR:			/* 35 tag 20 */
		break;
	case BASICTYPE_BITSTRING:  /* library type */
		PrintHJSBitstringDefCode(td, mod);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		PrintHJSSetOfDefCode(td, mods, mod);
		break;
	case BASICTYPE_CHOICE:
		PrintHJSChoiceDefCode(td, mods, mod);
		break;
	case BASICTYPE_ENUMERATED:  /* library type */
		PrintHJSEnumDefCode(td, mod);
		break;
	case BASICTYPE_SEQUENCE:
		PrintHJSSeqDefCode(td, mods, mod);
		break;
	case BASICTYPE_IMPORTTYPEREF:  /* type references */
	case BASICTYPE_LOCALTYPEREF:
		PrintHJSAlias(td, mods, mod);
		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
	case BASICTYPE_SET:
	case BASICTYPE_COMPONENTSOF:
	case BASICTYPE_SELECTION:
	case BASICTYPE_UNKNOWN:
	case BASICTYPE_MACRODEF:
	case BASICTYPE_MACROTYPE:
		break;
	default:
		/* TBD: print error? */
		break;
	}
}


void PrintHJSCode(ModuleList *mods, Module *m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printJSONEncDec, int novolatilefuncs)
{
	TypeDef* td;

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		PrintHJSModuleCode(m);
		PrintHJSTypeDefCode(td, mods, m);
	}

	//PrintConditionalIncludeClose (hdr, m->cxxHdrFileName);
} /* PrintHJSCode */


/* EOF gen-code.c (for back-ends/HJS-gen) */

