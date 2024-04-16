/*
 *   compiler/back_ends/swift_gen/gen_code.c - routines for printing C++ code from type trees
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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/swift-gen/gen-code.c,v 1.15 2008/04/15 07:29:30 \ste Exp $
 *
 */

#include "gen-swift-code.h"
#include "../../../c-lib/include/asn-incl.h"
#include "../../core/asn1module.h"
#include "../../core/asn_comments.h"
#include "../comment-util.h"
#include "../structure-util.h"
#include <inttypes.h>

// defined in normalize.c
#define SETOF_SUFFIX "SetOf"
#define SEQOF_SUFFIX "SeqOf"

#if 0
#define PRINTASN1POSITION                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
	fprintf(src, "\n// Asn1 File: %s:%d\n", m->baseFileName, td->type->lineNo);                                                                                                                                                                                                                                                                                                                                                                                                                                    \
	fflush(src);
#else
#define PRINTDEBUGGING fprintf(src, "// [%s]\n", __FUNCTION__);
#define PRINTASN1POSITION
#endif
int EndsWith(const char* str, const char* suffix)
{
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix > lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static char* kS_AsnSystemTime = "AsnSystemTime";
static char* kS_AsnOptionalParameters = "AsnOptionalParameters";
static char* kS_UTF8StringList = "UTF8StringList";
static char* kS_SEQInteger = "SEQInteger";

static void PrintSwiftNativeType(FILE* src, int basicTypeChoiseId)
{
	switch (basicTypeChoiseId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(src, "Bool");
			break;

		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf(src, "Int");
			break;

		case BASICTYPE_OCTETSTRING:
			fprintf(src, "Data");
			break;

		case BASICTYPE_ENUMERATED:
			fprintf(src, "Int"); // FIXME
			break;

		case BASICTYPE_REAL:
			fprintf(src, "Double");
			break;

		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "NSString");
			break;

		case BASICTYPE_UTCTIME:
			fprintf(src, "NSDate");
			break;

		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(src, "AnyObject");
			break;

		default:
			snacc_exit("Unknown basicTypeChoiseId %d", basicTypeChoiseId);
			break;
	}
}

static void PrintSwiftDefaultValue(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	{
		switch (t->basicType->choiceId)
		{
			case BASICTYPE_BOOLEAN:
				fprintf(src, "false");
				break;
			case BASICTYPE_INTEGER:
				fprintf(src, "0");
				break;
			case BASICTYPE_OCTETSTRING:
				fprintf(src, ".init()");
				break;
			case BASICTYPE_ENUMERATED:
				fprintf(src, "0"); // FIXME
				break;
			case BASICTYPE_SEQUENCEOF:
				fprintf(src, "%sArray", t->cxxTypeRefInfo->className);
				break;
			case BASICTYPE_REAL:
				fprintf(src, "0.0");
				break;
			case BASICTYPE_UTF8_STR:
			case BASICTYPE_OCTETCONTAINING:
				fprintf(src, "String() as NSString");
				break;
			case BASICTYPE_UTCTIME:
				fprintf(src, "AsnSystemTime()");
				break;
			case BASICTYPE_IMPORTTYPEREF:
				{
					char* className = FixName(t->basicType->a.importTypeRef->link->definedName);
					int refBasicTypeChoiceId = t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId;
					int typeIdOfReferencesType = t->basicType->a.importTypeRef->link->type->basicType->choiceId;

					if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
						fprintf(src, ".init()");
					else if (refBasicTypeChoiceId == BASICTYPE_ENUMERATED)
					{
						if (!t->basicType->a.importTypeRef->link)
						{
							snacc_exit("Invalid parameter, BASICTYPE_IMPORTTYPEREF but the associated link is NULL");
							return;
						}
						Type* enumType = t->basicType->a.importTypeRef->link->type;
						if (HasNamedElmts(enumType) != 0)
						{
							Type* pType = t->basicType->a.importTypeRef->link->type;
							CNamedElmt* pFirstEnumValue = (CNamedElmt*)pType->cxxTypeRefInfo->namedElmts->first->data;
							fprintf(src, "%s.%s.rawValue", t->cxxTypeRefInfo->className, pFirstEnumValue->name);
						}
					}
					else
					{
						fprintf(src, ".init()");
					}
					free(className);
					break;
				}
			case BASICTYPE_LOCALTYPEREF:
				{
					char* className = FixName(t->basicType->a.localTypeRef->link->definedName);
					int refBasicTypeChoiceId = t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;
					int typeIdOfReferencesType = t->basicType->a.localTypeRef->link->type->basicType->choiceId;

					if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
						fprintf(src, ".init()");
					else if (refBasicTypeChoiceId == BASICTYPE_ENUMERATED)
					{
						if (!t->basicType->a.importTypeRef->link)
						{
							snacc_exit("Invalid parameter, BASICTYPE_LOCALTYPEREF but the associated link is NULL");
							return;
						}
						Type* enumType = t->basicType->a.localTypeRef->link->type;
						if (HasNamedElmts(enumType) != 0)
						{
							Type* pType = t->basicType->a.importTypeRef->link->type;
							CNamedElmt* pFirstEnumValue = (CNamedElmt*)pType->cxxTypeRefInfo->namedElmts->first->data;
							fprintf(src, "%s.%s.rawValue", t->cxxTypeRefInfo->className, pFirstEnumValue->name);
						}
					}
					else
					{
						fprintf(src, ".init()");
					}
					free(className);
					break;
				}
			case BASICTYPE_UNKNOWN:
			case BASICTYPE_NULL:
				fprintf(src, "nil");
				break;
			default:
				fprintf(src, "%s()", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

void PrintSwiftType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
	if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (0 == strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime"))
		{
			choiceId = BASICTYPE_UTCTIME;
		}
		else if (0 == strcmp(t->cxxTypeRefInfo->className, "AsnOptionalParameters"))
		{
			fprintf(src, "%s", "AsnOptionalParamArray<AsnOptionalParam>");
			return;
		}
		else if (choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
				choiceId = t->basicType->a.importTypeRef->link->type->basicType->choiceId;
		}
		else if (choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
				choiceId = t->basicType->a.localTypeRef->link->type->basicType->choiceId;
		}
	}

	switch (choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_UTF8_STR:
			PrintSwiftNativeType(src, choiceId);
			break;
		case BASICTYPE_OCTETSTRING:
			fprintf(src, "Data");
			break;
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "NSString");
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(src, "%s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_NULL:
			fprintf(src, "AnyObject");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(src, "AsnSystemTime"); // AsnSystemTime ist im Asn1-file als REAL definiert, wird aber im TS als String übermittelt.
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				char* className = FixName(t->basicType->a.importTypeRef->link->definedName);
				int typeIdOfReferencesType = t->basicType->a.importTypeRef->link->type->basicType->choiceId;

				if (0 == strcmp(kS_UTF8StringList, className))
				{
					fprintf(src, "StringArray<NSString>");
				}
				else if (0 == strcmp(kS_SEQInteger, className))
				{
					fprintf(src, "IntArray");
				}
				else if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
				{
					char* elementName = t->basicType->a.importTypeRef->link->type->basicType->a.sequenceOf->basicType->a.importTypeRef->typeName;
					fprintf(src, "%sArray<%s>", elementName, elementName);
				}
				else
				{
					fprintf(src, "%s", className);
				}
				free(className);
				break;
			}
		case BASICTYPE_LOCALTYPEREF:
			{
				char* className = FixName(t->basicType->a.localTypeRef->link->definedName);
				int typeIdOfReferencesType = t->basicType->a.localTypeRef->link->type->basicType->choiceId;

				if (0 == strcmp(kS_UTF8StringList, className))
				{
					fprintf(src, "StringArray<NSString>");
				}
				else if (0 == strcmp(kS_SEQInteger, className))
				{
					fprintf(src, "IntArray");
				}
				else if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
				{
					char* elementName = t->basicType->a.localTypeRef->link->type->basicType->a.sequenceOf->basicType->a.localTypeRef->typeName;
					fprintf(src, "%sArray<%s>", elementName, elementName);
				}
				else
				{
					fprintf(src, "%s", className);
				}
				free(className);
				break;
			}
		case BASICTYPE_ANY:
			fprintf(src, "any");
			break;
		case BASICTYPE_BITSTRING:
			fprintf(src, "%s", t->cxxTypeRefInfo->className);
			break;
		default:
			snacc_exit("Unknown choiceId %d", choiceId);
	}
}

static void PrintSwiftEncoderDecoder(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	NamedType* e;
	int firstArgument = 1;

	// constructors for decoding
	fprintf(src, "    public init()\n");
	fprintf(src, "    {");
	if (IsDeprecatedFlaggedSequence(m, td->definedName))
	{
		asnsequencecomment comment;
		if (GetSequenceComment_UTF8(m->moduleName, td->definedName, &comment))
		{
			fprintf(src, "\n");
			fprintf(src, "        // CALL DeprecatedASN1Object(%" PRId64 ", \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, m->moduleName, td->definedName, comment.szDeprecated);
			fprintf(src, "    ");
		}
	}
	fprintf(src, "}\n");

	if (t->basicType->a.sequence->count > 1)
	{
		fprintf(src, "\n    public convenience init(");

		FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
		{
			char* optionalQuestionMark = "";

			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
			{
				if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
				{
					if (e->type->basicType->choiceId != BASICTYPE_NULL)
						optionalQuestionMark = "?";
				}

				if (firstArgument == 1)
					firstArgument = 0;
				else
					fprintf(src, ", ");

				char* szFielName = Dash2UnderscoreEx(e->fieldName);
				fprintf(src, "%s: ", szFielName);
				free(szFielName);

				PrintSwiftType(src, mods, m, td, parent, e->type);

				fprintf(src, "%s", optionalQuestionMark);
			}
		}

		fprintf(src, ")\n");
		fprintf(src, "    {\n");
		fprintf(src, "        self.init()\n");

		FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
		{

			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
			{
				char* szFielName = Dash2UnderscoreEx(e->fieldName);
				fprintf(src, "        self.%s = %s\n", szFielName, szFielName);
				free(szFielName);
			}
		}
		fprintf(src, "    }\n");
	}

	fprintf(src, "\n");
	fprintf(src, "    public required init(json: String)\n");
	fprintf(src, "    {\n");
	fprintf(src, "        if let jsonData: Data = json.data(using: String.Encoding.utf8)\n");
	fprintf(src, "        {\n");
	fprintf(src, "            do\n");
	fprintf(src, "            {\n");
	fprintf(src, "                let jsonObject: Any = try JSONSerialization.jsonObject(with: jsonData, options: [])\n");
	fprintf(src, "                setJSONObject(jsonObject as AnyObject)\n");
	fprintf(src, "            }\n");
	fprintf(src, "            catch _\n");
	fprintf(src, "            {}\n");
	fprintf(src, "        }\n");
	fprintf(src, "    }\n");
	fprintf(src, "\n");

	fprintf(src, "    public required init(jsonObject: AnyObject)\n");
	fprintf(src, "    {\n");
	fprintf(src, "        setJSONObject(jsonObject)\n");
	fprintf(src, "    }\n\n");

	fprintf(src, "    open class func fromJSONObject(_ jsonObject: AnyObject) -> AnyObject?\n");
	fprintf(src, "    {\n");
	fprintf(src, "        return %s(jsonObject: jsonObject)\n", td->definedName);
	fprintf(src, "    }\n\n");

	bool bHasElement = false;
	FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
			bHasElement = true;
	}

	fprintf(src, "    open func setJSONObject(_%s: AnyObject)\n", bHasElement ? " jsonObject" : "");
	fprintf(src, "    {");
	if (bHasElement)
		fprintf(src, "\n");

	int dictionaryhaselements = FALSE;

	FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			char* szFieldName = Dash2UnderscoreEx(e->fieldName);

			if (!dictionaryhaselements)
			{
				fprintf(src, "        if let dictionary: NSDictionary = jsonObject as? NSDictionary\n");
				fprintf(src, "        {\n");
				dictionaryhaselements = TRUE;
			}

			char* optionalQuestionMark = "";
			// char* optionalExclamationMark = "";

			if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
			{
				if (e->type->basicType->choiceId != BASICTYPE_NULL)
				{
					optionalQuestionMark = "?";
					//	optionalExclamationMark = "!";
				}
			}

			{
				if (e->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
				{

					int typeIdOfReferencesType = e->type->basicType->a.localTypeRef->link->type->basicType->choiceId;

					if (typeIdOfReferencesType == BASICTYPE_SEQUENCE || typeIdOfReferencesType == BASICTYPE_CHOICE)
					{
						fprintf(src, "            if let %s: AnyObject = dictionary.object(forKey: \"%s\") as AnyObject?\n", szFieldName, szFieldName);
						fprintf(src, "            {\n");

						if (strlen(optionalQuestionMark) == 0)
						{
							fprintf(src, "                self.%s = self.%s.merge(%s(jsonObject: %s))\n", szFieldName, szFieldName, e->type->basicType->a.localTypeRef->link->definedName, szFieldName);
						}
						else
						{
							fprintf(src, "                if self.%s != nil\n", szFieldName);
							fprintf(src, "                {\n");
							fprintf(src, "                    self.%s = self.%s%s.merge(%s(jsonObject: %s))\n", szFieldName, szFieldName, optionalQuestionMark, e->type->basicType->a.localTypeRef->link->definedName, szFieldName);
							fprintf(src, "                }\n");
							fprintf(src, "                else\n");
							fprintf(src, "                {\n");
							fprintf(src, "                    self.%s = %s(jsonObject: %s)\n", szFieldName, e->type->basicType->a.localTypeRef->link->definedName, szFieldName);
							fprintf(src, "                }\n");
						}
					}
					else
					{
						char* basicTypeName = e->type->basicType->a.localTypeRef->link->definedName;

						if (0 == strcmp(kS_AsnSystemTime, basicTypeName))
						{
							fprintf(src, "            if let asnTimeString: String = dictionary.object(forKey: \"%s\") as? String,\n", szFieldName);
							fprintf(src, "               let convertedDate = AsnSystemTime(from: asnTimeString)\n");
							fprintf(src, "            {\n");
							fprintf(src, "                %s = convertedDate\n", szFieldName);
						}
						else if (0 == strcmp(kS_UTF8StringList, basicTypeName))
						{
							fprintf(src, "            if let %s: AnyObject = dictionary.object(forKey: \"%s\") as AnyObject?\n", szFieldName, szFieldName);
							fprintf(src, "            {\n");
							fprintf(src, "                self.%s = StringArray<NSString>(jsonObject: %s)\n", szFieldName, szFieldName);
						}
						else if (0 == strcmp(kS_SEQInteger, basicTypeName))
						{
							fprintf(src, "            if let %s: AnyObject = dictionary.object(forKey: \"%s\") as AnyObject?\n", szFieldName, szFieldName);
							fprintf(src, "            {\n");
							fprintf(src, "                self.%s = IntArray(jsonObject: %s)\n", szFieldName, szFieldName);
						}
						else if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
						{
							char* elementName = e->type->basicType->a.localTypeRef->link->type->basicType->a.sequenceOf->basicType->a.localTypeRef->typeName;

							fprintf(src, "            if let %s: AnyObject = dictionary.object(forKey: \"%s\") as AnyObject?\n", szFieldName, szFieldName);
							fprintf(src, "            {\n");
							fprintf(src, "                self.%s = %sArray<%s>(jsonObject: %s)\n", szFieldName, elementName, elementName, szFieldName);
						}
						else if (typeIdOfReferencesType == BASICTYPE_ENUMERATED)
						{
							fprintf(src, "            if let %s: Int = dictionary.object(forKey: \"%s\") as? Int\n", szFieldName, szFieldName);
							fprintf(src, "            {\n");
							fprintf(src, "                self.%s = %s\n", szFieldName, szFieldName);
						}
						else
						{
							fprintf(src, "            if let %s: %s = dictionary.object(forKey: \"%s\") as? %s\n", szFieldName, e->type->basicType->a.localTypeRef->link->definedName, szFieldName, e->type->basicType->a.localTypeRef->link->definedName);
							fprintf(src, "            {\n");
							fprintf(src, "                self.%s = %s\n", szFieldName, szFieldName);
						}
					}
				}
				else if (e->type->basicType->choiceId == BASICTYPE_OCTETSTRING)
				{
					fprintf(src, "            if let %sString: String = dictionary.object(forKey: \"%s\") as? String\n", szFieldName, szFieldName);
					fprintf(src, "            {\n");
					fprintf(src, "                if let %sData = Data(base64Encoded: %sString, options: [])\n", szFieldName, szFieldName);
					fprintf(src, "                {\n");
					fprintf(src, "                    %s = %sData\n", szFieldName, szFieldName);
					fprintf(src, "                }\n");
				}
				else
				{
					fprintf(src, "            if let %s = dictionary.object(forKey: \"%s\") as", szFieldName, szFieldName);

					if (e->type->basicType->choiceId != BASICTYPE_NULL)
						fprintf(src, "?");
					fprintf(src, " ");
					PrintSwiftType(src, mods, m, td, parent, e->type);
					if (e->type->basicType->choiceId == BASICTYPE_NULL)
						fprintf(src, "?");
					fprintf(src, "\n");
					fprintf(src, "            {\n");
					fprintf(src, "                self.%s = %s\n", szFieldName, szFieldName);
				}
			}
			free(szFieldName);
			fprintf(src, "            }\n");
		}
	}
	if (dictionaryhaselements)
		fprintf(src, "        }\n");
	if (bHasElement)
		fprintf(src, "    }\n\n");
	else
		fprintf(src, "}\n\n");

	// encode as Dictionay
	fprintf(src, "    open func toJSONObject() -> AnyObject\n");
	fprintf(src, "    {\n");
	fprintf(src, "        let dictionary = NSMutableDictionary()\n");

	/* write out the sequence elmts */
	FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			char* szFieldName = Dash2UnderscoreEx(e->fieldName);

			char* optionalQuestionMark = "";
			char* optionalExclamationMark = "";
			const char* szIndent = "        ";

			if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
			{
				if (t->basicType->choiceId != BASICTYPE_NULL)
				{
					optionalQuestionMark = "?";
					optionalExclamationMark = "!";
				}
				szIndent = "            ";
				fprintf(src, "        if %s != nil\n", szFieldName);
				fprintf(src, "        {\n");
			}

			if (0 == strcmp("contactID", szFieldName))
			{
				//
			}
			{
				if (e->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
				{
					int refBasicTypeChoiceId = e->type->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;

					if (0 == strcmp(kS_AsnSystemTime, e->type->basicType->a.localTypeRef->link->definedName))
						fprintf(src, "%sdictionary.setValue(%s%s.description, forKey: \"%s\")\n", szIndent, szFieldName, optionalExclamationMark, szFieldName);
					else if (0 == strcmp(kS_AsnOptionalParameters, e->type->basicType->a.localTypeRef->link->definedName))
						fprintf(src, "%sdictionary.setValue(%s%s.toJSONObject(), forKey: \"%s\")\n", szIndent, szFieldName, optionalExclamationMark, szFieldName);
					else if (refBasicTypeChoiceId == BASICTYPE_BITSTRING || refBasicTypeChoiceId == BASICTYPE_ENUMERATED || refBasicTypeChoiceId == BASICTYPE_UTF8_STR)

						fprintf(src, "%sdictionary.setValue(%s, forKey: \"%s\")\n", szIndent, szFieldName, szFieldName);
					else if (refBasicTypeChoiceId == BASICTYPE_SEQUENCE || refBasicTypeChoiceId == BASICTYPE_CHOICE)
						fprintf(src, "%sdictionary.setValue(%s%s.toJSONObject(), forKey: \"%s\")\n", szIndent, szFieldName, optionalQuestionMark, szFieldName);
					else
						fprintf(src, "%sdictionary.setValue(%s%s.toJSONObject(), forKey: \"%s\")\n", szIndent, szFieldName, optionalExclamationMark, szFieldName);
				}
				else if (e->type->basicType->choiceId == BASICTYPE_OCTETSTRING)
				{
					fprintf(src, "%sdictionary.setValue(%s%s.base64EncodedString(options: []), forKey: \"%s\")\n", szIndent, szFieldName, optionalQuestionMark, szFieldName);
				}
				else
				{
					fprintf(src, "%sdictionary.setValue(%s, forKey: \"%s\")\n", szIndent, szFieldName, szFieldName);
				}
			}

			if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
				fprintf(src, "        }\n");

			free(szFieldName);
		}
	}
	fprintf(src, "        return dictionary\n");
	fprintf(src, "    }\n\n");

	// encode as JSON
	fprintf(src, "    open func toJSON() -> NSString?\n");
	fprintf(src, "    {\n");
	fprintf(src, "        let dictionary: AnyObject = toJSONObject()\n");
	fprintf(src, "        if JSONSerialization.isValidJSONObject(dictionary)\n");
	fprintf(src, "        {\n");
	fprintf(src, "            do\n");
	fprintf(src, "            {\n");
	fprintf(src, "                let json: Data = try JSONSerialization.data(withJSONObject: dictionary, options: JSONSerialization.WritingOptions.prettyPrinted)\n");
	fprintf(src, "                let jsonString: NSString? = NSString(data: json, encoding: String.Encoding.utf8.rawValue)\n");
	fprintf(src, "                return jsonString\n");
	fprintf(src, "            }\n");
	fprintf(src, "            catch _\n");
	fprintf(src, "            {}\n");
	fprintf(src, "        }\n");
	fprintf(src, "        return nil\n");
	fprintf(src, "    }\n\n");

	fprintf(src, "    public final func copy() -> %s\n", td->definedName);
	fprintf(src, "    {\n");
	fprintf(src, "        return %s(jsonObject: toJSONObject())\n", td->definedName);
	fprintf(src, "    }\n\n");
	// merge
	fprintf(src, "    public final func merge(_ objectToMerge: %s) -> %s\n", td->definedName, td->definedName);
	fprintf(src, "    {\n");
	fprintf(src, "        let result = copy()\n");
	fprintf(src, "        result.setJSONObject(objectToMerge.toJSONObject())\n");
	fprintf(src, "        return result\n");
	fprintf(src, "    }\n");
}

static void PrintSwiftEnumDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* enumerated, int novolatilefuncs)
{
	PRINTDEBUGGING

	fprintf(src, "public enum %s: Int\n", td->definedName);
	fprintf(src, "{\n");

	if (HasNamedElmts(td->type) != 0)
	{
		CNamedElmt* n;
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			printMemberComment(src, m, td, n->name, "  ", COMMENTSTYLE_SWIFT);
			fprintf(src, "    case %s = %d\n", n->name, n->value);
		}
	}

	fprintf(src, "\n");
	fprintf(src, "    static func fromJSONObject(_ jsonObject: AnyObject) -> Int?\n");
	fprintf(src, "    {\n");
	fprintf(src, "        return jsonObject as? Int\n");
	fprintf(src, "    }\n");
	fflush(src);

	fprintf(src, "}\n");
}

static void PrintSwiftSimpleRefDef(FILE* src, Module* m, TypeDef* td)
{
	PRINTDEBUGGING

	fprintf(src, "open class %s: %s\n", td->definedName, td->type->cxxTypeRefInfo->className);
	fprintf(src, "{\n");
	fprintf(src, "    public required init(json: String)\n");
	fprintf(src, "    {\n");
	fprintf(src, "        super.init(json: json)\n");
	fprintf(src, "    }\n\n");
	fprintf(src, "    public required init(jsonObject: AnyObject)\n");
	fprintf(src, "    {\n");
	fprintf(src, "        super.init(jsonObject: jsonObject)\n");
	fprintf(src, "    }\n");

	fprintf(src, "}\n");
}

static void PrintSwiftSimpleDef(FILE* src, ModuleList* mods, Module* m, TypeDef* td)
{
	if (0 == strcmp(kS_AsnSystemTime, td->definedName))
	{
		// is defined as custom class
	}
	else
	{
		PRINTDEBUGGING

		fprintf(src, "public typealias %s = ", td->definedName);
		// PrintSwiftType(src, NULL,m, NULL, NULL,td->type);
		// PrintSwiftArrayType(src,td->cxxTypeDefInfo->a);

		switch (td->type->basicType->choiceId)
		{
				// case BASICTYPE_SEQUENCEOF:	//fprintf(src,"GenericArray<%s>",td->type->basicType->a.sequenceOf->basicType->a.localTypeRef->typeName);
				// 							// PrintSwiftArrayType(src,td->cxxTypeDefInfo->);
				// 							break;

			default:
				PrintSwiftNativeType(src, td->cxxTypeDefInfo->asn1TypeId);
				break;
		}
	}
	fprintf(src, "\n");
}

static void PrintSwiftSeqOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	Type* innerType = td->type->basicType->a.sequenceOf;

	switch (innerType->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
			// primitive types, predefined in the lib
			break;

		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_UTCTIME:
			// class types, predefined in the lib
			break;
		default:
			// class types, defined in a .ASN file
			if (strcmp("AsnOptionalParam", innerType->cxxTypeRefInfo->className) == 0)
				break; // is defined as custom class

			PRINTDEBUGGING

			if (!EndsWith(td->definedName, SEQOF_SUFFIX) && !EndsWith(td->definedName, SETOF_SUFFIX) && td->type->basicType->choiceId == BASICTYPE_SEQUENCEOF && innerType->basicType->choiceId == BASICTYPE_LOCALTYPEREF)
			{
				fprintf(src, "public typealias %s = %sArray\n", td->definedName, innerType->cxxTypeRefInfo->className);

				fprintf(src, "open class %sArray<T: %s>: GenericArray<%s>\n", innerType->cxxTypeRefInfo->className, innerType->cxxTypeRefInfo->className, innerType->cxxTypeRefInfo->className);
				fprintf(src, "{\n");
				fprintf(src, "    override public init()\n");
				fprintf(src, "    {\n");
				fprintf(src, "        super.init()\n");
				fprintf(src, "    }\n\n");
				fprintf(src, "    public required init(jsonObject: AnyObject)\n");
				fprintf(src, "    {\n");
				fprintf(src, "        super.init(jsonObject: jsonObject)\n");
				fprintf(src, "    }\n\n");
				fprintf(src, "    override open func convertFromJSONObject(_ jsonObject: AnyObject) -> AnyObject?\n");
				fprintf(src, "    {\n");
				fprintf(src, "        return %s.fromJSONObject(jsonObject)\n", innerType->cxxTypeRefInfo->className);
				fprintf(src, "    }\n");
				fprintf(src, "}\n");
			}

			break;
	}
}

static void PrintSwiftChoiceDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* choice, int novolatilefuncs)
{
	if (strcmp("AsnOptionalParamChoice", td->definedName) == 0)
		return; // is defined as custom class

	PRINTDEBUGGING

	fprintf(src, "open class %s: JSONConvertable, JSONObjectConvertable\n", td->definedName);
	fprintf(src, "{\n");

	NamedType* e;
	FOR_EACH_LIST_ELMT(e, choice->basicType->a.choice)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			printMemberComment(src, m, td, e->fieldName, "  ", COMMENTSTYLE_SWIFT);
			char* szFieldName = Dash2UnderscoreEx(e->fieldName);
			fprintf(src, "    public final var %s: ", szFieldName);
			free(szFieldName);
			PrintSwiftType(src, mods, m, td, choice, e->type);
			fprintf(src, "? = nil");
			fprintf(src, "\n");
		}
	}

	fprintf(src, "\n");

	PrintSwiftEncoderDecoder(src, mods, m, td, parent, choice);

	fprintf(src, "}\n");
}

static void PrintSwiftSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* seq, int novolatilefuncs)
{
	NamedType* e;
	//	CxxTRI *cxxtri=NULL;
	//	int inTailOptElmts;
	//	enum BasicTypeChoiceId tmpTypeId;
	//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType** pSeqElementNamedType = NULL;

	if (strcmp("AsnOptionalParam", td->definedName) == 0)
	{
		// is defined as custom class
	}
	else
	{
		PRINTDEBUGGING

		/* put class spec in src file */

		// fprintf (src, "class %s %s\n", td->cxxTypeDefInfo->className);
		fprintf(src, "open class %s: JSONConvertable, JSONObjectConvertable\n", td->definedName);
		fprintf(src, "{\n");

		bool bAddedSomething = false;

		/* write out the sequence elmts */
		FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
		{
			// vars
			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
			{
				printMemberComment(src, m, td, e->fieldName, "  ", COMMENTSTYLE_SWIFT);
				char* szFielName = Dash2UnderscoreEx(e->fieldName);
				fprintf(src, "    public final var %s: ", szFielName);
				free(szFielName);
				PrintSwiftType(src, mods, m, td, seq, e->type);
				if (e->type->optional || e->type->basicType->choiceId == BASICTYPE_NULL)
				{
					fprintf(src, "? = nil");
				}
				else
				{
					fprintf(src, " = ");
					PrintSwiftDefaultValue(src, mods, m, td, seq, e->type);
				}
				fprintf(src, "\n");
				bAddedSomething = true;
			}
		}

		if (bAddedSomething)
			fprintf(src, "\n");

		PrintSwiftEncoderDecoder(src, mods, m, td, parent, seq);

		if (pSeqElementNamedType)
			free(pSeqElementNamedType);

		/* close class definition */
		fprintf(src, "}\n");
	}
}

static void PrintSwiftTypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	printSequenceComment(src, m, td, COMMENTSTYLE_SWIFT);

	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:		/* library type */
		case BASICTYPE_REAL:		/* library type */
		case BASICTYPE_OCTETSTRING: /* library type */
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_NULL:	 /* library type */
		case BASICTYPE_EXTERNAL: /* library type */
		case BASICTYPE_OID:		 /* library type */
		case BASICTYPE_RELATIVE_OID:
		case BASICTYPE_INTEGER:			 /* library type */
		case BASICTYPE_BITSTRING:		 /* library type */
		case BASICTYPE_NUMERIC_STR:		 /* 22 */
		case BASICTYPE_PRINTABLE_STR:	 /* 23 */
		case BASICTYPE_UNIVERSAL_STR:	 /* 24 */
		case BASICTYPE_IA5_STR:			 /* 25 */
		case BASICTYPE_BMP_STR:			 /* 26 */
		case BASICTYPE_UTF8_STR:		 /* 27 */
		case BASICTYPE_UTCTIME:			 /* 28 tag 23 */
		case BASICTYPE_GENERALIZEDTIME:	 /* 29 tag 24 */
		case BASICTYPE_GRAPHIC_STR:		 /* 30 tag 25 */
		case BASICTYPE_VISIBLE_STR:		 /* 31 tag 26  aka ISO646String */
		case BASICTYPE_GENERAL_STR:		 /* 32 tag 27 */
		case BASICTYPE_OBJECTDESCRIPTOR: /* 33 tag 7 */
		case BASICTYPE_VIDEOTEX_STR:	 /* 34 tag 21 */
		case BASICTYPE_T61_STR:			 /* 35 tag 20 */
			PrintSwiftSimpleDef(src, mods, m, td);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			PrintSwiftSeqOfDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */
		case BASICTYPE_LOCALTYPEREF:
			/*
			 * if this type has been re-tagged then
			 * must create new class instead of using a typedef
			 */
			PrintSwiftSimpleRefDef(src, m, td);
			break;
		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
		case BASICTYPE_ANY:
			// PrintCxxAnyDefCode (src, src, mods, m, r, td, NULL, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintSwiftChoiceDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintSwiftEnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SET:
			// PrintCxxSetDefCode (src, src, mods, m, r, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintSwiftSeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_SELECTION:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_MACRODEF:
		case BASICTYPE_MACROTYPE:
			/* do nothing */
			break;
		default:
			/* TBD: print error? */
			break;
	}
}

/*
 * prints PrintROSEInvoke
 */
static void PrintSwiftROSEInvoke(FILE* src, Module* m, int bEvents, ValueDef* vd)
{
	if (IsDeprecatedNoOutputOperation(m, vd->definedName))
		return;

	PRINTDEBUGGING

	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;

	Type* argumentType = NULL;
	Type* resultType = NULL;
	Type* errorType = NULL;

	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, true))
	{
		printOperationComment(src, m, vd->definedName, COMMENTSTYLE_SWIFT);
		fprintf(src, "open class %s: AsnOperation\n", vd->definedName);
		fprintf(src, "{\n");

		fprintf(src, "    public init()\n");
		fprintf(src, "    {");
		if (IsDeprecatedFlaggedOperation(m, vd->definedName))
		{
			asnoperationcomment comment;
			if (GetOperationComment_UTF8(m->moduleName, vd->definedName, &comment))
			{
				fprintf(src, "\n");
				fprintf(src, "        // CALL DeprecatedASN1Method(%" PRId64 ", \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, m->moduleName, vd->definedName, comment.szDeprecated);
				fprintf(src, "    ");
			}
		}
		fprintf(src, "}\n\n");

		fprintf(src, "    open class var name: String { return \"%s\" }\n", vd->definedName);
		fprintf(src, "    open var operationName: String { return %s.name }\n", vd->definedName);

		if (pszArgument)
		{
			if (argumentType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "    public final var asnArgument: Int? = nil //%s\n", pszArgument);
			else
				fprintf(src, "    public final var asnArgument: %s? = nil\n", pszArgument);
		}

		if (pszResult)
		{
			if (resultType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "    public final var asnResult: Int? = nil //%s\n", pszResult);
			else
				fprintf(src, "    public final var asnResult: %s? = nil\n", pszResult);
		}

		if (pszError)
		{
			if (errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "    public final var asnError: Int? = nil //%s\n", pszError);
			else
				fprintf(src, "    public final var asnError: %s? = nil\n", pszError);
		}

		fprintf(src, "\n");

		// functions
		fprintf(src, "    public final func getArgument() -> AnyObject?\n");
		fprintf(src, "    {\n");
		if (pszArgument)
			fprintf(src, "        return asnArgument\n");
		else
			fprintf(src, "        return nil\n");
		fprintf(src, "    }\n\n");

		fprintf(src, "    public final func setArgument(_ argument: AnyObject?)\n");
		fprintf(src, "    {\n");
		if (pszArgument)
		{
			fprintf(src, "        asnArgument = argument as? ");
			if (argumentType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "%s\n", "Int");
			else
				fprintf(src, "%s\n", pszArgument);
		}
		fprintf(src, "    }\n\n");

		fprintf(src, "    public final func getResult() -> AnyObject?\n");
		fprintf(src, "    {\n");
		if (pszResult)
			fprintf(src, "        return asnResult\n");
		else
			fprintf(src, "        return nil\n");
		fprintf(src, "    }\n\n");

		fprintf(src, "    public final func setResult(_%s: AnyObject?)\n", pszResult ? " result" : "");
		fprintf(src, "    {");
		if (pszResult)
		{
			fprintf(src, "\n");
			fprintf(src, "        asnResult = result as? ");
			if (resultType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "%s\n", "Int");
			else
				fprintf(src, "%s\n", pszResult);
			fprintf(src, "    ");
		}
		fprintf(src, "}\n\n");

		fprintf(src, "    public final func getError() -> AnyObject?\n");
		fprintf(src, "    {\n");
		if (pszError)
			fprintf(src, "        return asnError\n");
		else
			fprintf(src, "        return nil\n");
		fprintf(src, "    }\n\n");

		fprintf(src, "    public final func setError(_%s: AnyObject?)\n", pszError ? " error" : "");
		fprintf(src, "    {");
		if (pszError)
		{
			fprintf(src, "\n");
			fprintf(src, "        asnError = error as? ");
			if (errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
				fprintf(src, "%s\n", "Int");
			else
				fprintf(src, "%s\n", pszError);
			fprintf(src, "    ");
		}
		fprintf(src, "}\n\n");

		fprintf(src, "    public final func isEvent() -> Bool\n");
		fprintf(src, "    {\n");
		if (pszResult)
			fprintf(src, "        return false\n");
		else
			fprintf(src, "        return true\n");
		fprintf(src, "    }\n");

		fprintf(src, "}\n");
	}
}

void PrintSwiftComments(FILE* src, Module* m)
{
	PRINTDEBUGGING

	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", m->swiftFileName);
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
}

void PrintSwiftROSECode(FILE* src, ModuleList* mods, Module* m)
{
	PRINTDEBUGGING

	ValueDef* vd;

	PrintSwiftComments(src, m);

	fprintf(src, "import Foundation\n\n");
	bool bFirst = true;

	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			if (!bFirst)
				fprintf(src, "\n");
			PrintSwiftROSEInvoke(src, m, 0, vd);
			bFirst = false;
		}
	}
}

void PrintSwiftCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printJSONEncDec, int novolatilefuncs)
{
	PRINTDEBUGGING

	PrintSwiftComments(src, m);

	printModuleComment(src, m->moduleName, COMMENTSTYLE_SWIFT);

	fprintf(src, "import Foundation\n\n");

	bool bFirst = true;
	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			continue;
		if (!bFirst)
			fprintf(src, "\n");
		PrintSwiftTypeDefCode(src, mods, m, td, novolatilefuncs);
		bFirst = false;
	}
}

void PrintSwiftOperationFactory(FILE* src, ModuleList* mods)
{
	PRINTDEBUGGING

	Module* currMod;
	ValueDef* vd;

	fprintf(src, "import Foundation\n\n");

	fprintf(src, "public final class AsnOperationFactory\n");
	fprintf(src, "{\n");

	// fprintf(src, "  public init() {}\n");

	fprintf(src, "    public final class func createOperationFromJSONObject(_ operationName: String, argument: AnyObject?, result: AnyObject?, error: AnyObject?) -> AsnOperation?\n");
	fprintf(src, "    {\n");
	fprintf(src, "        switch operationName\n");
	fprintf(src, "        {\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{

			FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
				{
					if (IsDeprecatedNoOutputOperation(currMod, vd->definedName))
						continue;

					const char* pszArgument = NULL;
					const char* pszResult = NULL;
					const char* pszError = NULL;

					Type* argumentType = NULL;
					Type* resultType = NULL;
					Type* errorType = NULL;

					fprintf(src, "            case \"%s\":\n", vd->definedName);
					fprintf(src, "                let operation: %s = .init()\n", vd->definedName);

					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, true))
					{
						if (pszArgument)
						{
							fprintf(src, "                if let val: AnyObject = argument\n");
							fprintf(src, "                {\n");
							fprintf(src, "                    operation.setArgument(%s(jsonObject: val))\n", pszArgument);
							fprintf(src, "                }\n");
						}

						if (pszResult)
						{
							fprintf(src, "                if let val: AnyObject = result\n");
							fprintf(src, "                {\n");
							fprintf(src, "                    operation.setResult(%s(jsonObject: val))\n", pszResult);
							fprintf(src, "                }\n");
						}

						if (pszError)
						{
							fprintf(src, "                if let val: AnyObject = error\n");
							fprintf(src, "                {\n");
							fprintf(src, "                    operation.setError(%s.fromJSONObject(val))\n", pszError);
							fprintf(src, "                }\n");
						}
					}
					fprintf(src, "                return operation\n\n");
				}
			}
		}
	}

	fprintf(src, "            default:\n");
	fprintf(src, "                return nil\n");
	fprintf(src, "        }\n");
	fprintf(src, "    }\n\n");

	fprintf(src, "    public final class func listOperationNames() -> [String]\n");
	fprintf(src, "    {\n");
	fprintf(src, "        var list = [String]()\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
					fprintf(src, "        list.append(\"%s\")\n", vd->definedName);
			}
		}
	}
	fprintf(src, "        return list\n");
	fprintf(src, "    }\n\n");

	fprintf(src, "    public final class func createOperation(_ operationName: String, initializeWithDefaultProperties: Bool) -> AsnOperation?\n");
	fprintf(src, "    {\n");
	fprintf(src, "        switch operationName\n");
	fprintf(src, "        {\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			FOR_EACH_LIST_ELMT(vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
				{
					const char* pszArgument = NULL;
					const char* pszResult = NULL;
					const char* pszError = NULL;

					Type* argumentType = NULL;
					Type* resultType = NULL;
					Type* errorType = NULL;

					fprintf(src, "            case \"%s\":\n", vd->definedName);
					fprintf(src, "                let operation: %s = .init()\n", vd->definedName);

					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, true))
					{
						fprintf(src, "                if initializeWithDefaultProperties\n");
						fprintf(src, "                {\n");
						if (pszArgument)
							fprintf(src, "                    operation.setArgument(%s())\n", pszArgument);

						if (pszResult)
							fprintf(src, "                    operation.setResult(%s())\n", pszResult);

						if (pszError)
						{
							if (errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
								fprintf(src, "                    operation.setError(0)\n");
							else
								fprintf(src, "                    operation.setError(%s())\n", pszError);
						}
						fprintf(src, "                }\n");
					}
					fprintf(src, "                return operation\n\n");
				}
			}
		}
	}

	fprintf(src, "            default:\n");
	fprintf(src, "                return nil\n");
	fprintf(src, "        }\n");
	fprintf(src, "    }\n");

	fprintf(src, "}\n");
}