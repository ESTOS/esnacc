/*
 *   compiler/back_ends/TS_gen/gen_js_code.c - routines for printing C++ code from type trees
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
 * 2016 ESTOS/stm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "gen-swift-code.h"
#include "../str-util.h"
#include "../structure-util.h"
#include "../comment-util.h"
#include "gen-swift-combined.h"
#include <assert.h>

void PrintSwiftNativeType(FILE* src, enum BasicTypeChoiceId basicTypeChoiseId)
{
	switch (basicTypeChoiseId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(src, "Bool");
			break;
		case BASICTYPE_INTEGER:
		case BASICTYPE_REAL:
		case BASICTYPE_ENUMERATED:
			fprintf(src, "Int");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "Data");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(src, "NSString");
			break;
		case BASICTYPE_NULL:
			fprintf(src, "object");
			break;
		default:
			snacc_exit("Invalid basicTypeChoiseId %d", basicTypeChoiseId);
	}
}

void PrintSwiftDefaultValue(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t)
{
	if (t->optional)
	{
		fprintf(src, "nil");
		return;
	}

	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		CNamedElmt* n;
		Type* enumType = t->basicType->a.localTypeRef->link->type;
		if (HasNamedElmts(enumType) != 0)
		{
			FOR_EACH_LIST_ELMT_NOITERATE(n, enumType->cxxTypeRefInfo->namedElmts)
			{
				// fprintf(src, ", default: %s.%s", t->cxxTypeRefInfo->className, n->name);
				fprintf(src, "%s.%s", t->cxxTypeRefInfo->className, n->name);
				break;
			}
		}
	}
	else
	{
		enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
		if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
		{
			if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
				choiceId = BASICTYPE_UTCTIME;
			else if (choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link)
			{
				if (IsSimpleType(t->basicType->a.importTypeRef->link->type->basicType->choiceId))
					choiceId = t->basicType->a.importTypeRef->link->type->basicType->choiceId;
			}
			else if (choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link)
			{
				if (IsSimpleType(t->basicType->a.localTypeRef->link->type->basicType->choiceId))
					choiceId = t->basicType->a.localTypeRef->link->type->basicType->choiceId;
			}
			else
			{
				snacc_exit("Invalid parameter, choiceID is %i but the associated link is NULL", choiceId);
				return;
			}
		}

		switch (choiceId)
		{
			case BASICTYPE_BOOLEAN:
				fprintf(src, "false");
				break;
			case BASICTYPE_INTEGER:
			case BASICTYPE_ENUMERATED:
				fprintf(src, "0");
				break;
			case BASICTYPE_OCTETSTRING:
			case BASICTYPE_OCTETCONTAINING:
				fprintf(src, "Data()");
				break;
			case BASICTYPE_UTCTIME:
				fprintf(src, "Date()");
				break;
			case BASICTYPE_REAL:
				fprintf(src, "0.0");
				break;
			case BASICTYPE_UTF8_STR:
				fprintf(src, "String() as NSString");
				break;
			case BASICTYPE_IMPORTTYPEREF:
				{
					if (!t->basicType->a.importTypeRef->link)
					{
						snacc_exit("Invalid parameter, BASICTYPE_IMPORTTYPEREF but the associated link is NULL");
						return;
					}
					char* szConverted = FixName(t->basicType->a.importTypeRef->link->definedName);
					fprintf(src, "%s()", szConverted);
					free(szConverted);
					break;
				}
			case BASICTYPE_LOCALTYPEREF:
				{
					if (!t->basicType->a.localTypeRef->link)
					{
						snacc_exit("Invalid parameter, BASICTYPE_IMPORTTYPEREF but the associated link is NULL");
						return;
					}
					char* szConverted = FixName(t->basicType->a.localTypeRef->link->definedName);
					fprintf(src, "%s()", szConverted);
					free(szConverted);
					break;
				}
			case BASICTYPE_NULL:
				fprintf(src, "null");
				break;
			case BASICTYPE_ANY:
				fprintf(src, "undefined");
				break;
			case BASICTYPE_BITSTRING:
				break;
			default:
				snacc_exit("Unknown choiceId %d", choiceId);
		}
	}
}

void PrintSwiftType(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* t, int iForceType)
{
	enum BasicTypeChoiceId choiceId = t->basicType->choiceId;
	if (choiceId == BASICTYPE_IMPORTTYPEREF || choiceId == BASICTYPE_LOCALTYPEREF)
	{
		if (strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0)
			choiceId = BASICTYPE_UTCTIME;
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
			if (iForceType)
				PrintSwiftNativeType(src, choiceId);
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "Data");
			break;
		case BASICTYPE_SEQUENCEOF:
			fprintf(src, "%s[]", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_NULL:
			fprintf(src, "null");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(src, "Date"); // AsnSystemTime is defined as REAL in the Asn1 file, but is transmitted as a string in the Swift.
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				char* szConverted = FixName(t->basicType->a.importTypeRef->link->definedName);
				fprintf(src, "%s", szConverted);
				free(szConverted);
				break;
			}
		case BASICTYPE_LOCALTYPEREF:
			{
				char* szConverted = FixName(t->basicType->a.localTypeRef->link->definedName);
				fprintf(src, "%s", szConverted);
				free(szConverted);
				break;
			}
		case BASICTYPE_ANY:
			fprintf(src, "any");
			break;
		case BASICTYPE_BITSTRING:
			break;
		default:
			snacc_exit("Unknown choiceId %d", choiceId);
	}
} /* PrintCxxType */

void PrintSwiftEnumDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* enumerated, int novolatilefuncs)
{
	//	NamedType *e;
	//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt* n;
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	{
		char* szConverted = FixName(td->definedName);
		fprintf(src, "public enum %s : Int {\n", szConverted);
		free(szConverted);
	}
	if (HasNamedElmts(td->type) != 0)
	{
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			printMemberComment(src, m, td, n->name, "\t", COMMENTSTYLE_SWIFT);
			{
				char* szConverted = FixName(n->name);
				fprintf(src, "\tcase %s = %d", szConverted, n->value);
				free(szConverted);
			}
			fprintf(src, "\n");
		}
	}

	fprintf(src, "\n\tstatic func fromJSONObject(_ jsonObject:AnyObject) -> Int? {\n");
	fprintf(src, "\t\treturn jsonObject as? Int\n");
	fprintf(src, "\t}\n");

	/* close class definition */
	fprintf(src, "}\n\n");
} /* PrintSwiftEnumDefCode */

void PrintSwiftSeqDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* seq, int novolatilefuncs)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	bool bIsChoice = seq->basicType->choiceId == BASICTYPE_CHOICE;
	char* szConverted = FixName(td->definedName);

	printSequenceComment(src, m, td, COMMENTSTYLE_SWIFT);

	/* put class spec in src file */
	fprintf(src, "open class %s : JSONConvertable, JSONObjectConvertable {\n", szConverted);
	// fprintf(src, "\ttype: \"%s\",\n", td->definedName);

	/* Write out properties */
	// fprintf(src, "\tprops: {\n");
	int propertyCounter = 0;
	NamedType* e;
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		printMemberComment(src, m, td, e->fieldName, "\t", COMMENTSTYLE_SWIFT);
		char* szConverted2 = FixName(e->fieldName);
		fprintf(src, "\tpublic final var %s : ", szConverted2);
		free(szConverted2);

		PrintSwiftType(src, mods, m, td, seq, e->type, 1);

		if (e->type->optional || bIsChoice)
			fprintf(src, "? = nil");
		else
		{
			fprintf(src, " = ");
			PrintSwiftDefaultValue(src, mods, m, td, seq, e->type);
		}
		fprintf(src, "\n");

		propertyCounter++;
	}

	if (propertyCounter)
		fprintf(src, "\n");

	/*
	int iMandatoryFields = 0;
	// Wieviele Pflicht Elemente hat die Sequence?
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION || e->type->optional)
			continue;
		iMandatoryFields++;
	}
	*/

	// Jetzt schreiben wir den Konstruktor mit dem einen Argumenten was unserer eigenen Klasse entspricht
	// Damit erzwingen wir die dedizierte Angabe der pflicht Attribute beim Konstruktor aufruf
	fprintf(src, "\t// default constructor\n");
	fprintf(src, "\tpublic init() {\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// convenience constructor\n");
	fprintf(src, "\tpublic convenience init(");
	propertyCounter = 0;
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		if (propertyCounter)
			fprintf(src, ", ");

		char* szConverted2 = FixName(e->fieldName);
		fprintf(src, "%s : ", szConverted2);
		free(szConverted2);

		PrintSwiftType(src, mods, m, td, seq, e->type, 1);
		if (e->type->optional || bIsChoice)
			fprintf(src, "?");
		propertyCounter++;
	}
	fprintf(src, ") {\n");
	fprintf(src, "\t\tself.init()\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		char* szConverted2 = FixName(e->fieldName);
		fprintf(src, "\t\tself.%s = %s\n", szConverted2, szConverted2);
		free(szConverted2);
	}
	fprintf(src, "\t}\n\n\n");

	fprintf(src, "\t// JSON constructor\n");
	fprintf(src, "\tpublic required init(json: String) {\n");
	fprintf(src, "\t\tif let jsonData:Data = json.data(using: String.Encoding.utf8) {\n");
	fprintf(src, "\t\t\tdo {\n");
	fprintf(src, "\t\t\t\tlet jsonObject:Any = try JSONSerialization.jsonObject(with: jsonData, options:[])\n");
	fprintf(src, "\t\t\t\tself.setJSONObject(jsonObject as AnyObject)\n");
	fprintf(src, "\t\t\t} catch _ {\n");
	fprintf(src, "\t\t\t}\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Dictionary constructor\n");
	fprintf(src, "\tpublic required init(jsonObject: AnyObject) {\n");
	fprintf(src, "\t\tself.setJSONObject(jsonObject)\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Class generator function\n");
	fprintf(src, "\topen class func fromJSONObject(_ jsonObject:AnyObject) -> AnyObject? {\n");
	fprintf(src, "\t\treturn %s(jsonObject:jsonObject)\n", szConverted);
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Set JSON Object\n");
	fprintf(src, "\topen func setJSONObject(_ jsonObject: AnyObject) {\n");
	fprintf(src, "\t\tif let dictionary:NSDictionary = jsonObject as? NSDictionary {\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		char* szConverted2 = FixName(e->fieldName);
		fprintf(src, "\t\t\tif let %s = dictionary.object(forKey: \"%s\") as? ", szConverted2, szConverted2);
		PrintSwiftType(src, mods, m, td, seq, e->type, 1);
		fprintf(src, "\n");
		fprintf(src, "\t\t\t\tself.%s = %s\n", szConverted2, szConverted2);
		free(szConverted2);
	}
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Encode as Dictionary\n");
	fprintf(src, "\topen func toJSONObject() -> AnyObject {\n");
	fprintf(src, "\t\tlet dictionary:NSMutableDictionary = NSMutableDictionary()\n");
	FOR_EACH_LIST_ELMT(e, seq->basicType->a.sequence)
	{
		// OptionalParams ...
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		char* szConverted2 = FixName(e->fieldName);
		if (e->type->optional || bIsChoice)
			fprintf(src, "\t\tif(self.%s != nil)\n\t", szConverted2);
		fprintf(src, "\t\tdictionary.setValue(self.%s, forKey: \"%s\")\n", szConverted2, szConverted2);
		free(szConverted2);
	}
	fprintf(src, "\t\treturn dictionary\n");
	fprintf(src, "\t}\n\n\n");

	fprintf(src, "\topen func toJSON() -> NSString?\t{\n");
	fprintf(src, "\t\tlet dictionary : AnyObject = self.toJSONObject()\n");
	fprintf(src, "\t\tif JSONSerialization.isValidJSONObject(dictionary) {\n");
	fprintf(src, "\t\t\tdo {\n");
	fprintf(src, "\t\t\t\tlet json:Data = try JSONSerialization.data(withJSONObject: dictionary, options: JSONSerialization.WritingOptions.prettyPrinted)\n");
	fprintf(src, "\t\t\t\tlet jsonString:NSString? = NSString(data: json, encoding: String.Encoding.utf8.rawValue)\n");
	fprintf(src, "\t\t\t\treturn jsonString\n");
	fprintf(src, "\t\t\t} catch _ {\n");
	fprintf(src, "\t\t\t}\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t\treturn nil\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Copy\n");
	fprintf(src, "\tpublic final func copy() -> %s {\n", szConverted);
	fprintf(src, "\t\treturn %s(jsonObject: self.toJSONObject())\n", szConverted);
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Merge\n");
	fprintf(src, "\tpublic final func merge(_ objectToMerge : %s) -> %s {\n", szConverted, szConverted);
	fprintf(src, "\t\tlet result = self.copy()\n");
	fprintf(src, "\t\tresult.setJSONObject(objectToMerge.toJSONObject())\n");
	fprintf(src, "\t\treturn result\n");
	fprintf(src, "\t}\n");

	fprintf(src, "}\n");

	free(szConverted);
} /* PrintCxxSeqDefCode */

void PrintSwiftSetOfDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	char* szName = FixName(td->cxxTypeDefInfo->className);
	const char* szBaseNameArg = NULL;
	char* szBaseName = NULL;

	BasicType* pBase = GetBaseBasicType(td->type->basicType, &szBaseNameArg);
	if (pBase->choiceId == BASICTYPE_LOCALTYPEREF || pBase->choiceId == BASICTYPE_IMPORTTYPEREF)
		szBaseName = FixName(szBaseNameArg);

	switch (pBase->choiceId)
	{
		case BASICTYPE_BOOLEAN:
		case BASICTYPE_INTEGER:
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_ENUMERATED:
		case BASICTYPE_REAL:
		case BASICTYPE_UTF8_STR:
			fprintf(src, "open class %s<T:", szName);
			PrintSwiftNativeType(src, pBase->choiceId);
			fprintf(src, "> : GenericArray<");
			PrintSwiftNativeType(src, pBase->choiceId);
			fprintf(src, "> {\n");
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
			fprintf(src, "open class %s<T:%s> : GenericArray<%s> {\n", szName, szBaseName, szBaseName);
			break;
		default:
			snacc_exit("unsupported choice %i in [PrintSwiftListClass]", pBase->choiceId);
	}
	fprintf(src, "\tpublic override init() {\n");
	fprintf(src, "\t\tsuper.init()\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\tpublic required init(jsonObject: AnyObject) {\n");
	fprintf(src, "\t\tsuper.init(jsonObject:jsonObject)\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\topen override func convertFromJSONObject(_ jsonObject: AnyObject) -> AnyObject? {\n");
	fprintf(src, "\t\treturn %s.fromJSONObject(jsonObject)\n", szBaseName);
	fprintf(src, "\t}\n");

	fprintf(src, "}\n");

	if (szBaseName)
		free(szBaseName);
	free(szName);

} /* PrintSwiftSetOfDefCode */

void PrintSwiftimpleDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, Type* parent, Type* setOf, int novolatilefuncs)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	fprintf(src, "public typealias %s = ", td->definedName);
	PrintSwiftNativeType(src, td->type->basicType->choiceId);
	fprintf(src, "\n");

	if (td->type->basicType->choiceId == BASICTYPE_INTEGER && td->type->basicType->a.integer->count)
	{
		fprintf(src, "\nexport enum %senum {\n", td->definedName);
		int iAddComma = 0;
		ValueDef* td2;
		FOR_EACH_LIST_ELMT(td2, td->type->basicType->a.integer)
		{
			if (iAddComma)
				fprintf(src, ",\n");
			fprintf(src, "\t%s = %i", td2->definedName, td2->value->basicValue->a.integer);
			iAddComma = 1;
		}
		fprintf(src, "\n}\n");
	}
}

void PrintSwiftTypeDefCode(FILE* src, ModuleList* mods, Module* m, TypeDef* td, int novolatilefuncs)
{
	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:		/* library type */
		case BASICTYPE_REAL:		/* library type */
		case BASICTYPE_OCTETSTRING: /* library type */
		case BASICTYPE_OCTETCONTAINING:
		case BASICTYPE_INTEGER:	 /* library type */
		case BASICTYPE_UTF8_STR: /* 27 */
			PrintSwiftimpleDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			PrintSwiftSetOfDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */
			{
				Module* mod = GetImportModuleRefByClassName(td->type->basicType->a.importTypeRef->typeName, mods, m);
				if (mod)
				{
					const char* szNameSpace = GetNameSpace(mod);
					fprintf(src, "\nexport class %s extends %s.%s {\n", td->definedName, szNameSpace, td->type->basicType->a.importTypeRef->typeName);
					fprintf(src, "}\n");
				}
			}
			break;
		case BASICTYPE_LOCALTYPEREF:
			fprintf(src, "\nexport { %s as %s };\n", td->type->basicType->a.localTypeRef->typeName, td->definedName);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintSwiftEnumDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
		case BASICTYPE_CHOICE:
			PrintSwiftSeqDefCode(src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_BITSTRING:
			break;
		default:
			snacc_exit("Unknown td->type->basicType->choiceId %d", td->type->basicType->choiceId);
	}
} /* PrintCxxTypeDefCode */

/*
 * prints PrintROSEInvoke
 */
void PrintSwiftROSEInvoke(FILE* src, Module* m, int bEvents, ValueDef* vd)
{
} /* PrintROSEInvoke */

void PrintSwiftTypeDecl(FILE* src, TypeDef* td)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	switch (td->type->basicType->choiceId)
	{
		case BASICTYPE_COMPONENTSOF:
		case BASICTYPE_SELECTION:
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_MACRODEF:
		case BASICTYPE_MACROTYPE:
			return; /* do nothing */

		default:
			if (IsNewType(td->type))
				fprintf(src, "// typedef %s\n", td->cxxTypeDefInfo->className);
	}
}

void PrintSwiftComments(FILE* src, Module* m)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", RemovePath(m->swiftFileName));
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");

	printModuleComment(src, RemovePath(m->baseFileName), COMMENTSTYLE_SWIFT);
}

void PrintSwiftCode(FILE* src, ModuleList* mods, Module* m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int PrintSwiftONEncDec, int novolatilefuncs)
{
	PRINTCOMMENT(src, "\n// [%s]\n", __FUNCTION__);

	// Comments
	PrintSwiftComments(src, m);

	// Includes
	PrintSwiftImports(src, mods, m, false);

	TypeDef* td;
	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		if (IsDeprecatedNoOutputSequence(m, td->definedName))
			continue;
		PrintSwiftTypeDefCode(src, mods, m, td, novolatilefuncs);
	}
} /* PrintSwiftCode */

/* EOF gen-code.c (for back-ends/TS-gen) */
