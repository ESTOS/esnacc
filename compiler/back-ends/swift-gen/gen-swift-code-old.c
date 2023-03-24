/**   compiler/back_ends/swift_gen/gen_code.c - routines for printing C++ code from type trees
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

#include "gen-swift-code-old.h"
#include "../structure-util.h"
#include <assert.h>
// #define ADDMETHODCALLS 1

static void PrintSwiftNativeType(FILE *src, int basicTypeChoiseId)
{
	switch(basicTypeChoiseId)
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
			fprintf(src, "Int"); //FIXME
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
			exit(1);
			break;
	}
}

static void PrintSwiftArrayType(FILE *src, Type *t)
{
	int basicTypeChoiseId = t->basicType->choiceId;

	switch(basicTypeChoiseId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(src, "BoolArray");
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf(src, "IntArray");
			break;
		case BASICTYPE_OCTETSTRING:
			fprintf(src, "DataArray<NSData>");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(src, "IntArray");
			break;
		case BASICTYPE_REAL:
			fprintf(src, "DoubleArray");
			break;
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "StringArray<NSString>");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(src, "DateArray<NSDate>");
			break;
		case BASICTYPE_SEQUENCEOF:
			break;
		default:
			fprintf(src, "%sArray<%s>", t->cxxTypeRefInfo->className, t->cxxTypeRefInfo->className);
			break;
	}
}


static void PrintSwiftDefaultValue(FILE *src, Type *t, const char* szAlternateName) {
	const char* szName = t->cxxTypeRefInfo->className;
	if (!szName)
		szName = szAlternateName;
	if (szName) {
		if (strcmp(szName, "AsnSystemTime") == 0) {
			fprintf(src, "AsnSystemTime()");
			return;
		}
	}
	switch(t->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(src, "false");
			break;
		case BASICTYPE_INTEGER:
			fprintf(src, "0");
			break;
		case BASICTYPE_OCTETSTRING:
			fprintf(src, "Data()");
			break;
		case BASICTYPE_ENUMERATED: {
			ValueDef* v = (ValueDef*)FIRST_LIST_ELMT(t->basicType->a.enumerated);
			if (v)
				fprintf(src, "%s.%s.rawValue", szAlternateName, v->definedName);
			else
				fprintf(src, "0");
			break;
		}
		case BASICTYPE_SEQUENCEOF:
			PrintSwiftArrayType(src, t->basicType->a.sequenceOf);
			fprintf(src, "()");
			break;
		case BASICTYPE_REAL:
			fprintf(src, "0.0");
			break;
		case BASICTYPE_UTF8_STR:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(src, "String() as NSString");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(src, "NSDate()");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(src, "nil");
			break;
		case BASICTYPE_LOCALTYPEREF:
		case BASICTYPE_IMPORTTYPEREF:
		{
			char* szTypeName = NULL;
			t = ResolveTypeReferencesToRoot(t, &szTypeName);
			if (t->basicType->choiceId == BASICTYPE_BITSTRING) {
				fprintf(src, "%s()", szName);
			}
			else {
				PrintSwiftDefaultValue(src, t, szTypeName);
			}

			/*
				Type* t2 = t->basicType->a.importTypeRef->link->type;
				enum BasicTypeChoiceId choiceId = t2->basicType->choiceId;
				if (choiceId == BASICTYPE_SEQUENCE)
					fprintf(src, "%s()", t->basicType->a.importTypeRef->typeName);
				else if (choiceId == BASICTYPE_SEQUENCEOF) {
					t2->basicType->a.sequenceOf;
					PrintSwiftArrayType(src, t2);
				}
				else {
					PrintSwiftDefaultValue(src, t->basicType->a.importTypeRef->link->type);
				}
			*/
			break;
		}
		default:
			fprintf(src, "%s()", szName);
			break;
	}
}

static void PrintSwiftType(FILE *src, Type *t)
{
	if (t->cxxTypeRefInfo->className && strcmp(t->cxxTypeRefInfo->className, "AsnSystemTime") == 0) {
		fprintf(src, "AsnSystemTime");
		return;
	}

	switch(t->basicType->choiceId)
	{
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
			PrintSwiftNativeType(src, t->basicType->choiceId);
			break;
		case BASICTYPE_BITSTRING:
			fprintf(src, "%s", t->cxxTypeRefInfo->className);
			break;
		case BASICTYPE_SEQUENCEOF:
			PrintSwiftArrayType(src, t);
			break;
		case BASICTYPE_IMPORTTYPEREF:
			{
				Type *t2 = t->basicType->a.importTypeRef->link->type;
				if (t2->basicType->choiceId == BASICTYPE_SEQUENCE)
					fprintf(src, "%s", t->basicType->a.importTypeRef->typeName);
				else if (t2->basicType->choiceId == BASICTYPE_SEQUENCEOF) {
					const char* szName;
					Type *t3 = GetRootType(t2, &szName);
					if (t3->basicType->choiceId != BASICTYPE_SEQUENCE) {
						PrintSwiftArrayType(src, t3);
					}
					else {
						const char* szClassName = t2->basicType->a.sequenceOf->cxxTypeRefInfo->className;
						if(strcmp(szClassName, "UTF8String") == 0)
							fprintf(src, "StringArray<NSString>");
						else
							fprintf(src, "%sArray<%s>", szClassName, szClassName);
					}
				}
				else if (t2->basicType->choiceId == BASICTYPE_SETOF) {
					const char* szName;
					t2 = GetRootType(t2, &szName);
					const char* szClassName = t2->basicType->a.setOf->cxxTypeRefInfo->className;
					fprintf(src, "%sArray<%s>", szClassName, szClassName);
				}
				else
					PrintSwiftType(src, t2);
			}
			break;
		case BASICTYPE_LOCALTYPEREF:
			{
				Type *t2 = t->basicType->a.importTypeRef->link->type;
				if (t2->basicType->choiceId == BASICTYPE_SEQUENCE)
					fprintf(src, "%s", t->basicType->a.importTypeRef->typeName);
				else if (t2->basicType->choiceId == BASICTYPE_SEQUENCEOF) {
					const char* szClassName = t2->basicType->a.sequenceOf->cxxTypeRefInfo->className;
					fprintf(src, "%sArray<%s>", szClassName, szClassName);
				}
				else if (t2->basicType->choiceId == BASICTYPE_SETOF) {
					const char* szClassName = t2->basicType->a.setOf->cxxTypeRefInfo->className;
					fprintf(src, "%sArray<%s>", szClassName, szClassName);
				}
				else if (t2->basicType->choiceId == BASICTYPE_BITSTRING || t2->basicType->choiceId == BASICTYPE_CHOICE) {
					fprintf(src, "%s", t->cxxTypeRefInfo->className);
				}
				else
					PrintSwiftType(src, t2);
			}
			break;
		case BASICTYPE_CHOICE:
			fprintf(src, "%s", t->cxxTypeRefInfo->className);
			break;
		default:
			assert(FALSE);
			break;
	}
} /* PrintCxxType */

static void PrintSwiftEncoderDecoder(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *t)
{
#ifdef ADDMETHODCALLS
	fprintf(src, "// [%s] - %s\n", __FUNCTION__, td->definedName);
#endif
	NamedType *e;
	int firstArgument = 1;

	// constructors for decoding
	
	fprintf(src, "\t// default constructor\n");
	fprintf(src, "\tpublic init() {\n");
	fprintf(src, "\t}\n\n");

	// check if we have at least one non-extension type (otherwise a convenience does not make sense)
	if (t->basicType->a.sequence->count > 1)
	{
		fprintf(src, "\t// convenience constructor\n");
		fprintf(src, "\tpublic convenience init(");

		FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
		{
			char* optionalQuestionMark = "";

			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {

				if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE) {
					if (e->type->basicType->choiceId != BASICTYPE_NULL) {
						optionalQuestionMark = "?";
					}
				}

				if (firstArgument == 1) {
					firstArgument = 0;
				}
				else {
					fprintf(src, ", ");
				}

				fprintf(src, "%s : ", e->fieldName);
				PrintSwiftType(src, e->type);

				fprintf(src, "%s", optionalQuestionMark);
			}
		}

		fprintf(src, ") {\n");
		fprintf(src, "\t\tself.init()\n");

		FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
		{
			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {
				fprintf(src, "\t\tself.%s = %s\n", e->fieldName, e->fieldName);
			}
		}

		fprintf(src, "\t}\n\n\n");
	}

	fprintf(src, "\t// JSON constructor\n");
	fprintf(src, "\tpublic required init(json: String)\n");
	fprintf(src, "\t{\n");
	fprintf(src, "\t\tif let jsonData:Data = json.data(using: String.Encoding.utf8)\n");
	fprintf(src, "\t\t{\n");
	fprintf(src, "\t\t\tdo\n");
	fprintf(src, "\t\t\t{\n");
	fprintf(src, "\t\t\t\tlet jsonObject:Any = try JSONSerialization.jsonObject(with: jsonData, options:[])\n");
	fprintf(src, "\t\t\t\tself.setJSONObject(jsonObject as AnyObject)\n");
	fprintf(src, "\t\t\t}\n");
	fprintf(src, "\t\t\tcatch _\n");
	fprintf(src, "\t\t\t{\n");
	fprintf(src, "\t\t\t}\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Dictionary constructor\n");
	fprintf(src, "\tpublic required init(jsonObject: AnyObject) {\n");
	fprintf(src, "\t\tself.setJSONObject(jsonObject)\n");
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Class generator function\n");
	fprintf(src, "\topen class func fromJSONObject(_ jsonObject:AnyObject) -> AnyObject? {\n");
	fprintf(src, "\t\treturn %s(jsonObject:jsonObject)\n", td->definedName);
	fprintf(src, "\t}\n\n");

	fprintf(src, "\t// Set JSON Object \n");
	fprintf(src, "\topen func setJSONObject(_ jsonObject: AnyObject) {\n");

	int dictionaryhaselements = FALSE;

	FOR_EACH_LIST_ELMT(e, t->basicType->a.sequence)
	{
		enum BasicTypeChoiceId choiceId = e->type->basicType->choiceId;
		if (choiceId == BASICTYPE_EXTENSION)
			continue;

		if(!dictionaryhaselements)
		{
			fprintf(src, "\t\tif let dictionary:NSDictionary = jsonObject as? NSDictionary {\n");
			dictionaryhaselements = TRUE;
		}

		char* optionalQuestionMark = "";
		
		if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
		{
			if (choiceId != BASICTYPE_NULL)
			{
				optionalQuestionMark = "?";
			}
		}

		if (choiceId == BASICTYPE_SEQUENCEOF)
		{
			fprintf(src, "\t\tif let %s:AnyObject = dictionary.object(forKey: \"%s\") as AnyObject? {\n", e->fieldName, e->fieldName);

			// eg: self.optionalParams = AsnOptionalParamArray<AsnOptionalParam>(jsonObject: dictionary.objectForKey("optionalParams")!)
			fprintf(src, "\t\t\tself.%s = ", e->fieldName);
			PrintSwiftArrayType(src, e->type);
			fprintf(src, "(jsonObject: %s )\n", e->fieldName);

		}
		else if (choiceId == BASICTYPE_ENUMERATED)
		{
			fprintf(src, "\t\tif let %s:Int = dictionary.object(forKey: \"%s\") as? Int {\n", e->fieldName, e->fieldName);
			fprintf(src, "\t\t\tself.%s = %s\n", e->fieldName, e->fieldName);
		}
		else if (choiceId == BASICTYPE_LOCALTYPEREF || choiceId == BASICTYPE_IMPORTTYPEREF)
		{
			TypeDef* linkedType = e->type->basicType->a.localTypeRef->link;
			char *basicTypeName = linkedType->definedName;
			if (strcmp("AsnSystemTime", basicTypeName) == 0)
            {
                fprintf(src, "\t\tif  let asnTimeString:String        = dictionary.object(forKey: \"%s\") as? String,\n", e->fieldName);
                fprintf(src, "\t\t\tlet convertedDate:AsnSystemTime = AsnSystemTime(from:asnTimeString)\n");
                fprintf(src, "\t\t{\n");
                fprintf(src, "\t\t\tself.%s = convertedDate\n", e->fieldName);
            }
			else
			{
				int typeIdOfReferencesType = linkedType->type->basicType->choiceId;
				if (typeIdOfReferencesType == BASICTYPE_SEQUENCE || typeIdOfReferencesType == BASICTYPE_CHOICE)
				{
					fprintf(src, "\t\tif let %s:AnyObject = dictionary.object(forKey: \"%s\") as AnyObject? {\n", e->fieldName, e->fieldName);
					if(strlen(optionalQuestionMark) == 0)
						fprintf(src, "\t\t\tself.%s = self.%s.merge(%s(jsonObject: %s))\n", e->fieldName, e->fieldName, basicTypeName, e->fieldName);
					else
					{
						fprintf(src, "\t\t\tif self.%s != nil {\n", e->fieldName);
						fprintf(src, "\t\t\t\tself.%s = self.%s%s.merge(%s(jsonObject: %s))\n", e->fieldName, e->fieldName, optionalQuestionMark, basicTypeName, e->fieldName);
						fprintf(src, "\t\t\t} else {\n");
						fprintf(src, "\t\t\t\tself.%s = %s(jsonObject: %s)\n", e->fieldName, basicTypeName, e->fieldName);
						fprintf(src, "\t\t\t}\n");
					}
				}
				else if (typeIdOfReferencesType == BASICTYPE_SEQUENCEOF)
				{
					const char* szName = NULL;
					Type* type = GetRootType(linkedType->type, &szName);
					// const char* szClassName = linkedType->type->basicType->a.sequenceOf->cxxTypeRefInfo->className;
					fprintf(src, "\t\tif let %s:AnyObject = dictionary.object(forKey: \"%s\") as AnyObject? {\n", e->fieldName, e->fieldName);
					fprintf(src, "\t\t\tself.%s = ", e->fieldName);
					if(type->basicType->choiceId == BASICTYPE_SEQUENCE)
						fprintf(src, "%sArray<%s>", szName, szName);
					else
						PrintSwiftArrayType(src, type);
					fprintf(src, "(jsonObject: %s )\n", e->fieldName);
				}
				else if (typeIdOfReferencesType == BASICTYPE_ENUMERATED)
				{
					fprintf(src, "\t\tif let %s:Int = dictionary.object(forKey: \"%s\") as? Int {\n", e->fieldName, e->fieldName);
					fprintf(src, "\t\t\tself.%s = %s\n", e->fieldName, e->fieldName);
				}
				else
				{
					fprintf(src, "\t\tif let %s:%s = dictionary.object(forKey: \"%s\") as? %s {\n", e->fieldName, basicTypeName,e->fieldName, basicTypeName);
					fprintf(src, "\t\t\tself.%s = %s\n", e->fieldName, e->fieldName);
				}
			}
		}
		else if (choiceId == BASICTYPE_OCTETSTRING)
		{
			fprintf(src, "\t\tif let %sString:String = dictionary.object(forKey: \"%s\") as? String {\n", e->fieldName, e->fieldName);

			fprintf(src, "\t\t\tif let %sData:Data = Data(base64Encoded:%sString, options: [] ) {\n", e->fieldName, e->fieldName);
			fprintf(src, "\t\t\t\tself.%s = %sData\n", e->fieldName, e->fieldName);
			fprintf(src, "\t\t\t}\n");
		}
		else {

			fprintf(src, "\t\tif let %s = dictionary.object(forKey: \"%s\") as", e->fieldName, e->fieldName);

			if (choiceId != BASICTYPE_NULL) {
				fprintf(src, "?");
			}
			fprintf(src, " ");
			PrintSwiftType(src, e->type);
			if (choiceId == BASICTYPE_NULL) {
				fprintf(src, "?");
			}
			fprintf(src, " {\n");

			fprintf(src, "\t\t\tself.%s = %s\n", e->fieldName, e->fieldName);
		}
		fprintf(src, "\t\t}\n");
	}
	if(dictionaryhaselements)
		fprintf(src, "\t\t}\n");

	fprintf(src, "\t}\n\n");


	// encode as Dictionay
	fprintf(src, "\t// Encode as Dictionay\n");
	fprintf(src, "\topen func toJSONObject() -> AnyObject {\n");
	fprintf(src, "\t\tlet dictionary:NSMutableDictionary = NSMutableDictionary()\n");

	/* write out the sequence elmts */
	FOR_EACH_LIST_ELMT (e, t->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId != BASICTYPE_EXTENSION)
		{
			char* optionalQuestionMark = "";
			char* optionalExclamationMark = "";

			if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
			{
				if (t->basicType->choiceId != BASICTYPE_NULL)
				{
					optionalQuestionMark = "?";
					optionalExclamationMark = "!";
				}

				fprintf(src, "\t\tif(self.%s != nil) {\n", e->fieldName);
			}

			if (e->type->basicType->choiceId == BASICTYPE_SEQUENCEOF)
			{
				fprintf(src, "\t\t\tdictionary.setValue(self.%s%s.toJSONObject(), forKey: \"%s\")\n", e->fieldName, optionalExclamationMark, e->fieldName);
			}
			else if (e->type->basicType->choiceId  == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "\t\t\tdictionary.setValue(self.%s, forKey: \"%s\")\n", e->fieldName, e->fieldName);
			}
			else if (e->type->basicType->choiceId == BASICTYPE_LOCALTYPEREF || e->type->basicType->choiceId == BASICTYPE_IMPORTTYPEREF)
			{
				int refBasicTypeChoiceId = e->type->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId;
				if (refBasicTypeChoiceId == BASICTYPE_SEQUENCE || refBasicTypeChoiceId == BASICTYPE_CHOICE)
				{
					fprintf(src, "\t\t\tdictionary.setValue(self.%s%s.toJSONObject(), forKey: \"%s\")\n", e->fieldName, optionalQuestionMark, e->fieldName);
				}
				else if (strcmp("AsnSystemTime", e->type->basicType->a.localTypeRef->link->definedName) == 0)
                {
					fprintf(src, "\t\t\tdictionary.setValue(self.%s%s.description, forKey: \"%s\")\n", e->fieldName, optionalExclamationMark, e->fieldName);
                }
				else if (refBasicTypeChoiceId == BASICTYPE_SEQUENCEOF)
				{
					fprintf(src, "\t\t\tdictionary.setValue(self.%s%s.toJSONObject(), forKey: \"%s\")\n", e->fieldName, optionalExclamationMark, e->fieldName);
				}
				else
				{
					fprintf(src, "\t\t\tdictionary.setValue(self.%s%s, forKey: \"%s\")\n", e->fieldName, optionalExclamationMark, e->fieldName);
				}
			}
			else if (e->type->basicType->choiceId == BASICTYPE_OCTETSTRING)
			{
				fprintf(src, "\t\t\tdictionary.setValue(self.%s%s.base64EncodedString(options: []), forKey: \"%s\")\n", e->fieldName, optionalQuestionMark, e->fieldName);
			}
			else
			{
				fprintf(src, "\t\t\tdictionary.setValue(self.%s, forKey: \"%s\")\n", e->fieldName, e->fieldName);
			}

			if (e->type->optional || t->basicType->choiceId == BASICTYPE_CHOICE)
			{ //  || t->basicType->choiceId != BASICTYPE_NULL
				fprintf(src, "\t\t}\n");
			}
		}
	}
	fprintf(src, "\t\treturn dictionary\n");
	fprintf(src, "\t}\n\n\n");

	// encode as JSON
	fprintf(src, "open func toJSON() -> NSString?\n");
	fprintf(src, "{\n");
	fprintf(src, "\tlet dictionary : AnyObject = self.toJSONObject()\n");
	fprintf(src, "\tif JSONSerialization.isValidJSONObject(dictionary)\n");
	fprintf(src, "\t{\n");
	fprintf(src, "\t\tdo\n");
	fprintf(src, "\t\t{\n");
	fprintf(src, "\t\t\tlet json:Data = try JSONSerialization.data(withJSONObject: dictionary, options: JSONSerialization.WritingOptions.prettyPrinted)\n");
	fprintf(src, "\t\t\tlet jsonString:NSString? = NSString(data: json, encoding: String.Encoding.utf8.rawValue)\n");
	fprintf(src, "\t\t\treturn jsonString\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t\tcatch _\n");
	fprintf(src, "\t\t{\n");
	fprintf(src, "\t\t}\n");
	fprintf(src, "\t}\n");
	fprintf(src, "\treturn nil\n");
	fprintf(src, "}\n");
	fprintf(src, "\n");


//	fprintf(src, "  // Encode as JSON\n");
//	fprintf(src, "  public func toJSON() -> NSString? {\n");
//	fprintf(src, "    var dictionary : AnyObject = self.toJSONObject()\n");
//	fprintf(src, "    if(NSJSONSerialization.isValidJSONObject(dictionary)) {\n");
//	fprintf(src, "      var error:NSError? = nil\n");
//	fprintf(src, "      var json:NSData? = NSJSONSerialization.dataWithJSONObject(dictionary, options: NSJSONWritingOptions.PrettyPrinted, error: &error)\n");
//	fprintf(src, "      if(error == nil) {\n");
//	fprintf(src, "        var jsonString = NSString(data: json!, encoding: NSUTF8StringEncoding)\n");
//	fprintf(src, "        return jsonString\n");
//	fprintf(src, "      }\n");
//	fprintf(src, "    }\n");
//	fprintf(src, "    return nil\n");	
//	fprintf(src, "  }\n");
//	fprintf(src, "  \n");

	// copy
	fprintf(src, "\t// Copy\n");
	fprintf(src, "\tpublic final func copy() -> %s {\n", td->definedName);
	fprintf(src, "\t\treturn %s(jsonObject: self.toJSONObject())\n", td->definedName);
	fprintf(src, "\t}\n");
	fprintf(src, "\n");
	
	// merge
	fprintf(src, "\t// Merge\n");
	fprintf(src, "\tpublic final func merge(_ objectToMerge : %s) -> %s {\n", td->definedName, td->definedName);
	fprintf(src, "\t\tlet result = self.copy()\n");
	fprintf(src, "\t\tresult.setJSONObject(objectToMerge.toJSONObject())\n");
	fprintf(src, "\t\treturn result\n");
	fprintf(src, "\t}\n");

}

static void PrintSwiftEnumDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td,Type *parent, Type *enumerated, int novolatilefuncs)
{
//	NamedType *e;
//	enum BasicTypeChoiceId tmpTypeId;
	CNamedElmt *n;

	fprintf(src, "public enum %s : Int\n", td->definedName);
	fprintf(src, "{\n");

	if (HasNamedElmts (td->type) != 0)
	{
		FOR_EACH_LIST_ELMT (n, td->type->cxxTypeRefInfo->namedElmts)
		{
			fprintf(src, "  case %s = %d\n", n->name, n->value);
		}
	}

	fprintf(src, "\n");
	fprintf(src, "  static func fromJSONObject(_ jsonObject:AnyObject) -> Int? {\n");
	fprintf(src, "    return jsonObject as? Int\n");
	fprintf(src, "  }\n");
	fprintf(src, "\n");

	//PrintSwiftEncoderDecoder(src, mods, m, td, parent, enumerated);

	/* close class definition */
	fprintf(src, "}\n\n\n");
} /* PrintSwiftEnumDefCode */

static void PrintSwiftSimpleRefDef(FILE *src, Module *m, TypeDef *td)
{
#ifdef ADDMETHODCALLS
	fprintf(src, "// [%s] - %s\n", __FUNCTION__, td->definedName);
#endif

	fprintf(src, "open class %s : %s\n", td->definedName, td->type->cxxTypeRefInfo->className);
	fprintf(src, "{\n");
	fprintf(src, "  public override init() {\n");
	fprintf(src, "  }\n\n");
	fprintf(src, "  public required init(json: String) {\n");
	fprintf(src, "    super.init(json:json)\n");
	fprintf(src, "  }\n\n");
	fprintf(src, "  public required init(jsonObject: AnyObject) {\n");
	fprintf(src, "    super.init(jsonObject:jsonObject)\n");
	fprintf(src, "  }\n");
	
	fprintf(src, "}\n\n\n");
}

static void PrintSwiftSimpleDef(FILE *src, Module *m, TypeDef *td)
{
	if (strcmp("AsnSystemTime", td->definedName) == 0)
	{
		// is defined as custom class
		
	}
	else
	{
		fprintf(src, "public typealias %s = ", td->definedName);
		PrintSwiftNativeType(src, td->cxxTypeDefInfo->asn1TypeId);
	}
	fprintf(src, "\n\n\n");
}

static void PrintSwiftSetOfDefCode(FILE *src, 
								 ModuleList *mods,
								 Module *m,
								 TypeDef *td,
								 Type *parent,
								 Type *setOf,
								 int novolatilefuncs)
{
	Type* innerType = td->type->basicType->a.sequenceOf;

	switch(innerType->basicType->choiceId)
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
			if(strcmp("AsnOptionalParam",innerType->cxxTypeRefInfo->className)==0)
			{
				// is defined as custom class
			}
			else
			{
				//td->type->cxxTypeRefInfo->classNam
				fprintf(src, "open class %sArray<T:%s> : GenericArray<%s>\n", innerType->cxxTypeRefInfo->className, innerType->cxxTypeRefInfo->className, innerType->cxxTypeRefInfo->className );
				fprintf(src, "{\n");
				fprintf(src, "\tpublic override init() {\n");
				fprintf(src, "\t\tsuper.init()\n");
				fprintf(src, "\t}\n\n");
				fprintf(src, "\tpublic required init(jsonObject: AnyObject) {\n");
				fprintf(src, "\t\tsuper.init(jsonObject:jsonObject)\n");
				fprintf(src, "\t}\n\n");
				fprintf(src, "\topen override func convertFromJSONObject(_ jsonObject: AnyObject) -> AnyObject? {\n");
				fprintf(src, "\t\treturn %s.fromJSONObject(jsonObject)\n", innerType->cxxTypeRefInfo->className);
				fprintf(src, "\t}\n");
				fprintf(src, "}\n\n\n");
			}
			break;
	}
} /* PrintSwiftSetOfDefCode */

static void PrintSwiftChoiceDefCode(FILE *src, ModuleList *mods, Module *m, 
								  TypeDef *td,Type *parent, Type *choice, int novolatilefuncs)
{
#ifdef ADDMETHODCALLS
	fprintf(src, "// [%s] - %s\n", __FUNCTION__, td->definedName);
#endif

	NamedType *e;
//	enum BasicTypeChoiceId tmpTypeId;

	if (strcmp("AsnOptionalParamChoice",td->definedName) == 0) {
		// is defined as custom class
		
	} else {
		fprintf (src, "open class %s : JSONConvertable, JSONObjectConvertable\n", td->definedName);
		fprintf(src, "{\n");

		/* write out choiceId enum type */


		/* write out the choice element anonymous union */
		FOR_EACH_LIST_ELMT (e, choice->basicType->a.choice)
		{
			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {
				fprintf(src, "\tpublic final var %s : ", e->fieldName);
				PrintSwiftType(src, e->type);
				fprintf(src, "? = nil");
				fprintf(src, "\n");
			}			
		}

		fprintf(src, "\n");

		PrintSwiftEncoderDecoder(src, mods, m, td, parent, choice);


		/* close class definition */
		fprintf(src, "}\n\n\n");
	}
} /* PrintSwiftChoiceDefCode */

static void PrintSwiftSeqDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, Type *parent, Type *seq, int novolatilefuncs)
{
#ifdef ADDMETHODCALLS
	fprintf(src, "// [%s] - %s\n", __FUNCTION__, td->definedName);
#endif

	NamedType *e;
//	CxxTRI *cxxtri=NULL;
//	int inTailOptElmts;
//	enum BasicTypeChoiceId tmpTypeId;
//	int allOpt;

	// DEFINE PER encode/decode tmp vars.
	NamedType **pSeqElementNamedType=NULL;

	if(strcmp("AsnOptionalParam",td->definedName)==0) {
		// is defined as custom class
		
	} else {
		/* put class spec in hdr file */

		//fprintf(src, "class %s %s\n", td->cxxTypeDefInfo->className);
		fprintf(src, "open class %s : JSONConvertable, JSONObjectConvertable\n", td->definedName);
		fprintf(src, "{\n");

		/* write out the sequence elmts */
		FOR_EACH_LIST_ELMT (e, seq->basicType->a.sequence)
		{
			// vars
			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {
				
				fprintf(src, "\tpublic final var %s : ", e->fieldName);
				PrintSwiftType(src, e->type);
				if (e->type->optional || e->type->basicType->choiceId == BASICTYPE_NULL) {
					fprintf(src, "? = nil");
				}
				else {
					fprintf(src, " = ");
					PrintSwiftDefaultValue(src, e->type, NULL);
				}
				fprintf(src, "\n");
			}			
		}

		fprintf(src, "\n");

		PrintSwiftEncoderDecoder(src, mods, m, td, parent, seq);

		if (pSeqElementNamedType)
			free(pSeqElementNamedType);

		/* close class definition */
		fprintf(src, "}\n\n\n");
	}
} /* PrintCxxSeqDefCode */

static void PrintSwiftTypeDefCode(FILE *src, ModuleList *mods, Module *m, TypeDef *td, int novolatilefuncs)
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
		case BASICTYPE_BITSTRING:  /* library type */
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
			PrintSwiftSimpleDef (src, m, td);
			break;
		case BASICTYPE_SEQUENCEOF:  /* list types */
		case BASICTYPE_SETOF:
			PrintSwiftSetOfDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_IMPORTTYPEREF:  /* type references */
		case BASICTYPE_LOCALTYPEREF:
			/*
				* if this type has been re-tagged then
				* must create new class instead of using a typedef
			*/
			PrintSwiftSimpleRefDef (src, m, td);
			break;
		case BASICTYPE_ANYDEFINEDBY:    /* ANY types */
		case BASICTYPE_ANY:
			//PrintCxxAnyDefCode (src, src, mods, m, r, td, NULL, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintSwiftChoiceDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_ENUMERATED:  /* library type */
			PrintSwiftEnumDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SET:
			//PrintCxxSetDefCode (src, src, mods, m, r, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintSwiftSeqDefCode (src, mods, m, td, NULL, td->type, novolatilefuncs);
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
} /* PrintCxxTypeDefCode */

/*
* prints PrintROSEInvoke
*/
static void PrintSwiftROSEInvoke(FILE *src, Module *m, int bEvents, ValueDef *vd)
{
#ifdef ADDMETHODCALLS
	fprintf(src, "// [%s] - %s\n", __FUNCTION__, vd->definedName);
#endif
	
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	Type* argumentType = NULL;
	Type* resultType = NULL;
	Type* errorType = NULL;

	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
	{
		fprintf(src, "open class %s : AsnOperation\n", vd->definedName);
		fprintf(src, "{\n");

		fprintf(src, "\tpublic init() {}\n");

		// vars
		fprintf(src, "\topen class var name    : String { get { return \"%s\" } }\n", vd->definedName);
		fprintf(src, "\topen var operationName : String { get { return %s.name } }\n", vd->definedName);

		if(pszArgument)
		{
			if(argumentType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "\tpublic final var asnArgument : Int? = nil //%s\n", pszArgument);
			}
			else
			{
				fprintf(src, "\tpublic final var asnArgument : %s? = nil\n", pszArgument);
			}
		}

		if(pszResult)
		{
			if(resultType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "\tpublic final var asnResult : Int? = nil //%s\n", pszResult);
			}
			else
			{
				fprintf(src, "\tpublic final var asnResult : %s? = nil\n", pszResult);
			}
		}

		if(pszError)
		{
			if(errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "\tpublic final var asnError : Int? = nil //%s\n", pszError);
			}
			else
			{
				fprintf(src, "\tpublic final var asnError : %s? = nil\n", pszError);
			}
		}

		fprintf(src, "\n");

		// functions
		fprintf(src, "\tpublic final func getArgument() -> AnyObject? {");
		if(pszArgument)
		{
			fprintf(src, "\t\treturn self.asnArgument");
		}
		else
		{
			fprintf(src, "\t\treturn nil");
		}
		fprintf(src, "\t}\n\n");

		fprintf(src, "\tpublic final func setArgument(_ argument:AnyObject?) {");
		if(pszArgument)
		{
			fprintf(src, "\t\tself.asnArgument = argument as? ");
			if(argumentType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "%s", "Int");
			}
			else
			{
				fprintf(src, "%s", pszArgument);
			}
		}
		fprintf(src, "\t}\n\n");
	
		fprintf(src, "\tpublic final func getResult() -> AnyObject? {");
		if(pszResult)
		{
			fprintf(src, "\t\treturn self.asnResult");
		}
		else
		{
			fprintf(src, "\t\treturn nil");
		}
		fprintf(src, "  }\n\n");

		fprintf(src, "\tpublic final func setResult(_ result:AnyObject?) {");
		if(pszResult)
		{
			fprintf(src, "\t\tself.asnResult = result as? ");
			if(resultType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "%s", "Int");
			}
			else
			{
				fprintf(src, "%s", pszResult);
			}
		}
		fprintf(src, "\t}\n\n");

	
		fprintf(src, "\tpublic final func getError() -> AnyObject? {");
		if(pszError)
		{
			fprintf(src, "\t\treturn self.asnError"); 
		}
		else
		{
			fprintf(src, "\t\treturn nil"); 
		}
		fprintf(src, "\t}\n\n");

		fprintf(src, "\tpublic final func setError(_ error:AnyObject?) {");
		if(pszError)
		{
			fprintf(src, "\t\tself.asnError = error as? ");
			if(errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
			{
				fprintf(src, "%s", "Int");
			}
			else
			{
				fprintf(src, "%s", pszError);
			}
		}
		fprintf(src, "\t}\n\n");
	
		fprintf(src, "\tpublic final func isEvent() -> Bool {");
		if(pszResult)
		{
			fprintf(src, "\t\treturn false");
		}
		else
		{
			fprintf(src, "\t\treturn true");
		}
		fprintf(src, "\t}\n\n");

		fprintf(src, "}\n\n\n");
	}
} /* PrintROSEInvoke */

void PrintSwiftCommentsOld(FILE *src, Module *m)
{
	fprintf(src, "/*\n");
	fprintf(src, " * %s\n", m->swiftFileName);
	fprintf(src, " * \"%s\" ASN.1 stubs.\n", m->modId->name);
	fprintf(src, " *\n");
	write_snacc_header(src, " * ");
	fprintf(src, " */\n\n");
}

void PrintSwiftROSECodeOLD(FILE *src, ModuleList *mods, Module *m)
{
	ValueDef *vd;

	PrintSwiftCommentsOld(src, m);

	fprintf(src, "import Foundation\n\n");

	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintSwiftROSEInvoke(src, m, 0, vd);
		}
	}
}


void PrintSwiftCodeOLD(FILE *src,  ModuleList *mods, Module *m, long longJmpVal, int printTypes, int printValues, int printEncoders, int printDecoders, int printJSONEncDec, int novolatilefuncs)
{
	Module *currMod;
	AsnListNode *currModTmp;
	TypeDef *td;
//	ValueDef *vd;

	// Comments
	PrintSwiftCommentsOld(src, m);

	// Includes
	fprintf(src, "import Foundation\n\n");

	fprintf(src, "\n");    //RWC; PRINT before possible "namespace" designations.

	FOR_EACH_LIST_ELMT (currMod, mods)
	{
		if ((strcmp(m->swiftFileName, currMod->swiftFileName) == 0))
		{
			// Code to see the import module list AND load possible "namespace" refs.
			ImportModuleList *ModLists;
			ImportModule *impMod;
//			char *ImpFile = NULL;
			ModLists = currMod->imports;
			currModTmp = mods->curr;    //RWC;
			FOR_EACH_LIST_ELMT(impMod, ModLists)
			{
				/*
				ImpFile = GetImportFileName (impMod->modId->name, mods);
				if (ImpFile != NULL)
					fprintf(src, "#include \"%s\"\n", ImpFile);
				if (impMod->moduleRef == NULL)  // RWC; attempt to update...
					impMod->moduleRef = GetImportModuleRef(impMod->modId->name, mods);
				if (impMod->moduleRef &&
					impMod->moduleRef->namespaceToUse)
				{
					fprintf(src,"using namespace %s;\n", impMod->moduleRef->namespaceToUse);
				}
				*/
			}
			mods->curr = currModTmp;    // RWC;RESET loop control
		}
		// Don't duplicate header file referenced in source
		//		if ((strcmp(m->cxxHdrFileName, currMod->cxxHdrFileName) != 0))
		//		{
		//			if ((ImportedFilesG == FALSE) || (currMod->ImportedFlag == TRUE))
		//				fprintf(src, "#include \"%s\"\n", currMod->cxxHdrFileName);
		//		}
	}

	fprintf(src, "\n");

	//fprintf(src, "//------------------------------------------------------------------------------\n");
	//fprintf(src, "// class declarations:\n\n");
	//FOR_EACH_LIST_ELMT (td, m->typeDefs)
	//	PrintSwiftTypeDecl (src, td);
	//fprintf(src, "\n");


	FOR_EACH_LIST_ELMT (td, m->typeDefs)
	{
		PrintSwiftTypeDefCode (src, mods, m, td, novolatilefuncs);
	}

	//PrintConditionalIncludeClose (src, m->cxxHdrFileName);
} /* PrintSwiftCode */

void PrintSwiftOperationFactoryOLD(FILE *src, ModuleList *mods) {
	Module		*currMod;
	ValueDef *vd;

	fprintf(src, "import Foundation\n\n");

	fprintf(src, "public final class AsnOperationFactory\n");
	fprintf(src, "{\n");

	//fprintf(src, "  public init() {}\n");

	fprintf(src, "  public final class func createOperationFromJSONObject(_ operationName:String, argument:AnyObject?, result:AnyObject?, error:AnyObject?) -> AsnOperation?\n");
	fprintf(src, "  {\n");
	fprintf(src, "    switch(operationName) {\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{

			FOR_EACH_LIST_ELMT (vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
				{
					char* pszArgument = NULL;
					char* pszResult = NULL;
					char* pszError = NULL;

					Type* argumentType = NULL;
					Type* resultType = NULL;
					Type* errorType = NULL;

					fprintf(src, "      case \"%s\":\n", vd->definedName);
					fprintf(src, "        let operation:%s = %s()\n",vd->definedName,vd->definedName);

					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
					{
						if(pszArgument)
						{
							fprintf(src, "        if let val : AnyObject = argument {\n");
							fprintf(src, "          operation.setArgument( %s(jsonObject:val) )\n", pszArgument);
							fprintf(src, "        }\n");
						}

						if(pszResult)
						{
							fprintf(src, "        if let val : AnyObject = result {\n");
							fprintf(src, "          operation.setResult( %s(jsonObject:val) )\n", pszResult);
							fprintf(src, "        }\n");
						}

						if(pszError)
						{
							fprintf(src, "        if let val : AnyObject = error {\n");
							fprintf(src, "          operation.setError( %s.fromJSONObject(val) )\n", pszError);
							fprintf(src, "        }\n");
						}
					}
					fprintf(src, "        return operation\n\n");
				}
			}
		}
	}

	fprintf(src, "      default:\n");
	fprintf(src, "        return nil\n");
	fprintf(src, "    }\n");
	fprintf(src, "  }\n\n\n");

	fprintf(src, "  public final class func listOperationNames() -> [String]\n");
	fprintf(src, "  {\n");
	fprintf(src, "    var list:Array<String> = Array<String>()\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			FOR_EACH_LIST_ELMT (vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
				{
					fprintf(src, "    list.append( \"%s\" )\n", vd->definedName);
				}
			}
		}
	}
	fprintf(src, "    return list\n");
	fprintf(src, "  }\n\n\n");

	fprintf(src, "  public final class func createOperation(_ operationName:String, initializeWithDefaultProperties:Bool) -> AsnOperation?\n");
	fprintf(src, "  {\n");
	fprintf(src, "    switch(operationName) {\n");

	FOR_EACH_LIST_ELMT(currMod, mods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			FOR_EACH_LIST_ELMT (vd, currMod->valueDefs)
			{
				if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
				{
					char* pszArgument = NULL;
					char* pszResult = NULL;
					char* pszError = NULL;

					Type* argumentType = NULL;
					Type* resultType = NULL;
					Type* errorType = NULL;

					fprintf(src, "      case \"%s\":\n", vd->definedName);
					fprintf(src, "        let operation:%s = %s()\n", vd->definedName,vd->definedName);

					if (GetROSEDetails(currMod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
					{
						fprintf(src, "        if(initializeWithDefaultProperties) {\n");
						if(pszArgument)
							fprintf(src, "          operation.setArgument( %s() )\n", pszArgument);

						if(pszResult)
						{
							fprintf(src, "          operation.setResult( %s() )\n", pszResult);
						}

						if(pszError)
						{
							if(errorType->basicType->choiceId == BASICTYPE_ENUMERATED)
							{
								fprintf(src, "          operation.setError( 0 )\n");
							}
							else
							{
								fprintf(src, "          operation.setError( %s() )\n", pszError);
							}
						}
						fprintf(src, "        }\n");
					}
					fprintf(src, "        return operation\n\n");
				}
			}
		}
	}

	fprintf(src, "      default:\n");
	fprintf(src, "        return nil\n");
	fprintf(src, "    }\n");
	fprintf(src, "  }\n");
	
	fprintf(src, "}\n");
}

/* EOF gen-code.c (for back-ends/swift-gen) */

