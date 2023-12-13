/*
 *   compiler/back_ends/java-gen/gen_code.c - routines for printing JAVA code from type trees
 *  -RJ ENetROSEInterface.asn1 ENetUC_Admin.asn1 ENetUC_Auth.asn1 ENetUC_AV.asn1 ENetUC_Chat.asn1 ENetUC_ChatV2.asn1 ENetUC_ClientContent.asn1 ENetUC_ClientUpdate.asn1 ENetUC_Common.asn1  ENetUC_PresenceV2.asn1  ENetUC_CTI.asn1 ENetUC_Journal.asn1 ENetUC_Transport.asn1 ENetUC_UMReplicator.asn1
 *
 */

#include "gen-java-code.h"

#include "../../../c-lib/include/print.h"
#include "../../core/asn_comments.h"
#include "../tag-util.h" /* get GetTags/FreeTags/CountTags/TagByteLen */
#include "../str-util.h"
#include "../structure-util.h"
#include "../comment-util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#if META
#include "meta.h"
#endif

#define PRINTDEBUGGING fprintf(src, "// [%s]\n", __FUNCTION__);

static void PrintJavaType(FILE* hdr, ModuleList* mods, Module* mod, Type* t);
static void PrintJavaTypeDefCode(ModuleList* mods, Module* mod, TypeDef* td);

static void handleDeprecatedSequence(FILE* src, Module* mod, TypeDef* td)
{
	if (IsDeprecatedFlaggedSequence(mod, td->definedName))
	{
		fprintf(src, "  public %s()\n", td->definedName);
		fprintf(src, "  {\n");
		asnsequencecomment comment;
		if (GetSequenceComment_UTF8(mod->moduleName, td->definedName, &comment))
			fprintf(src, "    // CALL DeprecatedASN1Object(%" PRId64 ", \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, mod->moduleName, td->definedName, comment.szDeprecated);
		fprintf(src, "  }\n\n");
	}
}

static FILE* getJavaFilePointer(char* pPrefix)
{
	char* fileName = MakeFileName(pPrefix, ".java");

	FILE* p = NULL;
#ifdef _WIN32
	fopen_s(&p, fileName, "wt");
#else
	p = fopen(fileName, "wt");
#endif
	free(fileName);

	if (!p)
	{
		snacc_exit("File open failed %s", fileName);
		return p;
	}

	fprintf(p, "/**\n");
	fprintf(p, " * esnacc compiler generated java files\n");
	fprintf(p, " *\n");
	write_snacc_header(p, " * ");
	fprintf(p, " */\n\n");

	return p;
}

static char* getJavaClassName(char* prefix, char* suffix)
{
	size_t size = strlen(prefix) + strlen(suffix) + 1;
	char* className = malloc(size); // strlen + /0
	if (!className)
	{
		snacc_exit("Out of memory");
		return NULL;
	}

	sprintf_s(className, size, "%s%s", prefix, suffix);
	className[0] = (char)toupper(className[0]);

	return className;
}

static void PrintJavaNativeType(FILE* hdr, Type* type)
{
	switch (type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "Boolean");
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf(hdr, "Integer");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "String");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "%s", type->typeName); // FIXME
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, "Double");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "String");
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "Date");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(hdr, "Object");
			break;
		default:
			break;
			// assert(0 == 1);
	};
}

static void PrintJavaNativeTypeConstructor(FILE* hdr, Type* type)
{
	switch (type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "false"); // Boolean
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf(hdr, "0"); // Integer
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "\"\""); // String
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "%s.values()[0]", type->typeName); // FIXME
			break;
		case BASICTYPE_REAL:
			fprintf(hdr, "0.0"); // Double
			break;
		case BASICTYPE_UTF8_STR:
			fprintf(hdr, "\"\""); // String
			break;
		case BASICTYPE_UTCTIME:
			fprintf(hdr, "new Date()");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(hdr, "new Object()");
			break;
		default:
			break;
			// assert(0 == 1);
	};
}

static void PrintJavaTypeConstructor(FILE* hdr, ModuleList* mods, Module* mod, Type* t)
{
	/*if(t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) {
	  //PrintJavaArrayType(hdr, t->basicType->a.localTypeRef->link->type->basicType->a.sequence,t->basicType->a.localTypeRef->link);
	} else */
	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "%s.values()[0]", t->cxxTypeRefInfo->className); // FIXME
	}
	else if (t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF && t->basicType->a.importTypeRef->link != NULL && t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "%s.values()[0]", t->cxxTypeRefInfo->className); // FIXME
	}
	else
	{
		switch (t->basicType->choiceId)
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
				PrintJavaNativeTypeConstructor(hdr, t);
				break;
			case BASICTYPE_SEQUENCEOF:
				fprintf(hdr, "new ArrayList<%s>()", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf(hdr, "new %s()", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

static void PrintJavaArrayType(FILE* hdr, ModuleList* mods, Module* mod, Type* t, TypeDef* innerType)
{

	fprintf(hdr, "ArrayList<");
	PrintJavaType(hdr, mods, mod, t);
	fprintf(hdr, ">");

	switch (innerType->type->basicType->choiceId)
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
			PrintJavaTypeDefCode(mods, mod, innerType);
	}
}
static void PrintJavaType(FILE* hdr, ModuleList* mods, Module* mod, Type* t)
{
	if (t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED)
	{
		fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
	}
	else
	{
		switch (t->basicType->choiceId)
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
				PrintJavaNativeType(hdr, t);
				break;
			case BASICTYPE_SEQUENCEOF:
				PrintJavaArrayType(hdr, mods, mod, t, (TypeDef*)t->cxxTypeRefInfo);
				// fprintf (hdr, "[%s]", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

static void PrintSeqJavaDataSequenceOf(ModuleList* mods, Module* mod, TypeDef* td)
{
	char* name = getJavaClassName(td->definedName, "");
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import java.util.ArrayList;\n");
	fprintf(src, "import java.util.List;\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "public class %s extends ArrayList<", name);
	PrintJavaType(src, mods, mod, td->type->basicType->a.setOf);
	fprintf(src, "> {\n");
	handleDeprecatedSequence(src, mod, td);
	fprintf(src, "  private static final long serialVersionUID = 1L;\n");
	fprintf(src, "  public %s(List<", name);
	PrintJavaType(src, mods, mod, td->type->basicType->a.setOf);
	fprintf(src, "> values){\n\n");
	fprintf(src, "    super(values);\n");
	fprintf(src, "  }\n");
	fprintf(src, "}\n");

	fprintf(src, "  public %s(){\n\n");
	fprintf(src, "    super();\n");
	fprintf(src, "  }\n");
	fprintf(src, "}\n");
	fclose(src);
	free(name);
}

static void PrintSeqJavaDataSequence(ModuleList* mods, Module* mod, TypeDef* td)
{
	NamedType* e;
	char* name = getJavaClassName(td->definedName, "");
	char* tmpName;
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");

	fprintf(src, "import java.io.Serializable;\n");
	fprintf(src, "import androidx.annotation.NonNull;\n");
	fprintf(src, "import androidx.annotation.Nullable;\n");
	fprintf(src, "import javax.annotation.Generated;\n");
	fprintf(src, "\n");

	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "public class %s implements Serializable {\n", name);
	handleDeprecatedSequence(src, mod, td);
	fprintf(src, "  private static final long serialVersionUID = 1L;\n");

	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		printMemberComment(src, mod, td, e->fieldName, "  ", COMMENTSTYLE_JAVA);
		fprintf(src, "  private ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " %s = ", e->fieldName);
		if (e->type->optional || e->type->basicType->choiceId == BASICTYPE_NULL)
			fprintf(src, "null");
		else
			PrintJavaTypeConstructor(src, mods, mod, e->type);
		fprintf(src, ";\n");
	}

	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		int isNullable = 0;
		if (e->type->optional || td->type->basicType->choiceId == BASICTYPE_CHOICE || td->type->basicType->choiceId == BASICTYPE_NULL)
			isNullable = 1;

		tmpName = getJavaClassName(e->fieldName, "");
		fprintf(src, "\n  ");
		if (isNullable == 1)
			fprintf(src, "@Nullable");
		else
			fprintf(src, "@NonNull");
		fprintf(src, " public ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " get%s()\n", tmpName);
		fprintf(src, "  {\n");
		fprintf(src, "    return this.%s;\n", e->fieldName);
		fprintf(src, "  }\n");

		fprintf(src, "  public void set%s(", tmpName);
		if (isNullable == 1)
			fprintf(src, "@Nullable ");
		else
			fprintf(src, "@NonNull ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " %s)\n", e->fieldName);
		fprintf(src, "  {\n");
		fprintf(src, "    this.%s = %s;\n", e->fieldName, e->fieldName);
		fprintf(src, "  }\n");
		free(tmpName);
	}
	fprintf(src, "}\n");
	fclose(src);
	free(name);
}

static void PrintJavaChoiceDefCode(ModuleList* mods, Module* mod, TypeDef* td)
{
	NamedType* e;
	char* name = getJavaClassName(td->definedName, "");
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import java.io.Serializable;\n");
	fprintf(src, "import androidx.annotation.Nullable;\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);

	fprintf(src, "public class %s implements Serializable{\n\n", name);
	handleDeprecatedSequence(src, mod, td);

	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.choice)
	{
		fprintf(src, "  private ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " %s=", e->fieldName);
		fprintf(src, "null");
		fprintf(src, ";\n");
	}
	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		char* tmpName = getJavaClassName(e->fieldName, "");
		fprintf(src, "\n  @Nullable public ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " get%s(){\n", tmpName);
		fprintf(src, "    return this.%s;\n", e->fieldName);
		fprintf(src, "  }\n");

		fprintf(src, "  public void set%s(", tmpName);
		fprintf(src, "@Nullable ");
		PrintJavaType(src, mods, mod, e->type);
		fprintf(src, " %s){\n", e->fieldName);
		fprintf(src, "    this.%s=%s;\n", e->fieldName, e->fieldName);
		fprintf(src, "  }\n");
		free(tmpName);
	}
	fprintf(src, "\n");
	fprintf(src, "}\n");
	fclose(src);
	free(name);
}
static void PrintJavaSimpleRefDef(ModuleList* mods, Module* mod, TypeDef* td)
{

	char* name = getJavaClassName(td->definedName, "");
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "public class %s extends %s{\n", td->definedName, td->type->cxxTypeRefInfo->className);
	fprintf(src, "}\n\n");
	fclose(src);
	free(name);
}

static char* captalize(char* string)
{
	char* upper = calloc(sizeof(char), strlen(string) + 1);
	if (!upper)
	{
		snacc_exit("Out of memory");
		return NULL;
	}

	size_t index;

	for (index = 0; index < strlen(string); index++)
		upper[index] = (char)toupper(string[index]);

	return upper;
}
static void PrintJavaEnumDefCode(ModuleList* mods, Module* mod, TypeDef* td)
{
	char* name = getJavaClassName(td->definedName, "");
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	char* enumValue;
	CNamedElmt* n;
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "public enum %s{\n", name);

	if (HasNamedElmts(td->type) != 0)
	{
		int count = 0;
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			if (count > 0)
				fprintf(src, ",\n");
			printMemberComment(src, mod, td, n->name, "  ", COMMENTSTYLE_JAVA);
			enumValue = captalize(n->name);
			fprintf(src, "  %s(%d)", enumValue, n->value);
			free(enumValue);
			count++;
		}
	}
	fprintf(src, ";\n\n");

	fprintf(src, "  private int value;\n\n");
	fprintf(src, "  private %s(int value)\n", name);
	fprintf(src, "  {\n");
	fprintf(src, "    this.value = value;\n");
	fprintf(src, "  }\n");
	fprintf(src, "  public int getValue()\n");
	fprintf(src, "  {\n");
	fprintf(src, "    return value;\n");
	fprintf(src, "  }\n");
	fprintf(src, "}\n");
	free(name);
	fclose(src);
}

static void PrintJavaSimpleDef(ModuleList* mods, Module* mod, TypeDef* td)
{

	char* name = getJavaClassName(td->definedName, "");

	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "public class %s extends SimpleJavaType<", name);
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "String");
	else
		PrintJavaNativeType(src, td->type);
	fprintf(src, ">{\n\n");
	//handleDeprecatedSequence(src, mod, td);
	fprintf(src, "  public %s() {\n", name);
	fprintf(src, "    super(");
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "new String()");
	else
		PrintJavaNativeTypeConstructor(src, td->type);

	fprintf(src, ");\n  }\n\n");
	fprintf(src, "  public %s(", name);
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "String");
	else
		PrintJavaNativeType(src, td->type);
	fprintf(src, " value) {\n");
	fprintf(src, "    super(value);\n");
	fprintf(src, "  }\n");
	fprintf(src, "}\n\n");
	free(name);
	fclose(src);
}

static void PrintJavaTypeDefCode(ModuleList* mods, Module* mod, TypeDef* td)
{
	if (IsDeprecatedNoOutputSequence(mod, td->definedName))
		return;

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
			PrintJavaSimpleDef(mods, mod, td);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			PrintSeqJavaDataSequenceOf(mods, mod, td);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */
		case BASICTYPE_LOCALTYPEREF:
			/*
			 * if this type has been re-tagged then
			 * must create new class instead of using a typedef
			 */
			PrintJavaSimpleRefDef(mods, mod, td);
			break;
		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
		case BASICTYPE_ANY:
			// PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintJavaChoiceDefCode(mods, mod, td);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintJavaEnumDefCode(mods, mod, td);
			break;
		case BASICTYPE_SET:
			// PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintSeqJavaDataSequence(mods, mod, td);
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

static void PrintJavaOperationClass(Module* mod, ValueDef* vd)
{
	if (IsDeprecatedNoOutputOperation(mod, vd->definedName))
		return;

	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	Type* argumentType = NULL;
	Type* resultType = NULL;
	Type* errorType = NULL;
	char* name = getJavaClassName(vd->definedName, "Operation");
	FILE* src = getJavaFilePointer(name);
	PRINTDEBUGGING

	fprintf(src, "package com.estos.asn;\n\n");
	if (GetROSEDetails(mod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
	{
		fprintf(src, "import javax.annotation.Generated;\n\n");
		printOperationComment(src, mod, vd->definedName, COMMENTSTYLE_JAVA);
		fprintf(src, "public final class %s extends AbstractAsnOperation<", name);

		if (pszArgument)
			fprintf(src, "%s, ", pszArgument);
		else
			fprintf(src, "Void, ");

		if (pszResult)
			fprintf(src, "%s, ", pszResult);
		else
			fprintf(src, "Void, ");

		if (pszError)
			fprintf(src, "%s>", pszError);
		else
			fprintf(src, "Void>");
		fprintf(src, " {\n");

		if (IsDeprecatedFlaggedOperation(mod, vd->definedName))
		{
			fprintf(src, "  public %s()\n", name);
			fprintf(src, "  {\n");
			asnoperationcomment comment;
			if (GetOperationComment_UTF8(mod->moduleName, vd->definedName, &comment))
				fprintf(src, "    // CALL DeprecatedASN1Method(%" PRId64 ", \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, mod->moduleName, vd->definedName, comment.szDeprecated);
			fprintf(src, "  }\n\n");
		}

		fprintf(src, "  public Class<?> getAsnArgumentType()\n");
		fprintf(src, "  {\n");
		// Class Type definititon
		if (pszArgument)
			fprintf(src, "    return %s.class;\n", pszArgument);
		else
			fprintf(src, "    return Void.class;\n");
		fprintf(src, "  }\n\n");

		fprintf(src, "  public Class<?> getAsnResultType()\n");
		fprintf(src, "  {\n");
		if (pszResult)
			fprintf(src, "    return %s.class;\n", pszResult);
		else
			fprintf(src, "    return Void.class;\n");
		fprintf(src, "  }\n\n");

		fprintf(src, "  public Class<?> getAsnErrorType()\n");
		fprintf(src, "  {\n");
		if (pszError)
			fprintf(src, "    return %s.class;\n", pszError);
		else
			fprintf(src, "    return Void.class;\n");
		fprintf(src, "  }\n");
		fprintf(src, "}\n");
	}

	free(name);
	fclose(src);
}

static void PrintAbstractJavaOperation()
{
	FILE* src = getJavaFilePointer("AbstractAsnOperation");
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import com.google.gson.annotations.SerializedName;\n\n");
	fprintf(src, "public abstract class AbstractAsnOperation<ARGUMENT_TYPE,RESULT_TYPE,ERROR_TYPE> {\n\n");
	fprintf(src, "  @SerializedName(\"argument\")\n  private ARGUMENT_TYPE asnArgument;\n\n");
	fprintf(src, "  @SerializedName(\"result\")\n  private RESULT_TYPE asnResult;\n\n");
	fprintf(src, "  @SerializedName(\"error\")\n  private ERROR_TYPE asnError;\n\n");

	fprintf(src, "  public String getOperationName() {\n");
	fprintf(src, "    String name = getClass().getSimpleName();\n");
	fprintf(src, "    return Character.toString(name.charAt(0)).toLowerCase()+name.substring(1,name.length()-\"Operation\".length());\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnArgument(ARGUMENT_TYPE asnArgument) {\n");
	fprintf(src, "    this.asnArgument = asnArgument;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public ARGUMENT_TYPE getAsnArgument(){\n");
	fprintf(src, "    return asnArgument;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnResult(RESULT_TYPE asnResult) {\n");
	fprintf(src, "    this.asnResult = asnResult;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public RESULT_TYPE getAsnResult() {\n");
	fprintf(src, "    return asnResult;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnError(ERROR_TYPE asnError) {\n");
	fprintf(src, "    this.asnError = asnError;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public ERROR_TYPE getAsnError() {\n");
	fprintf(src, "    return asnError;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public abstract Class<?> getAsnArgumentType();\n\n");
	fprintf(src, "  public abstract Class<?> getAsnResultType();\n\n");
	fprintf(src, "  public abstract Class<?> getAsnErrorType();\n\n");

	fprintf(src, "}\n");
	fclose(src);
}

static void PrintSimpleJavaType()
{
	FILE* src = getJavaFilePointer("SimpleJavaType");
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import java.io.Serializable;\n\n");
	fprintf(src, "public abstract class SimpleJavaType<T> implements Serializable{\n\n");
	fprintf(src, "  private final T value;\n\n");

	fprintf(src, "  public SimpleJavaType(T value){\n");
	fprintf(src, "    this.value=value;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public T getValue(){\n");
	fprintf(src, "    return value;\n");
	fprintf(src, "  }\n");

	fprintf(src, "}\n\n");
	fclose(src);
}

void PrintJAVACode(ModuleList* mods, Module* m)
{
	ValueDef* vd;
	TypeDef* td;
	PrintAbstractJavaOperation();
	PrintSimpleJavaType();
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
			PrintJavaOperationClass(m, vd);
	}

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		PrintJavaTypeDefCode(mods, m, td);
	}
}
