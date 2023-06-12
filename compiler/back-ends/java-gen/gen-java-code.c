/*
 *   compiler/back_ends/java-gen/gen_code.c - routines for printing JAVA code from type trees
 *	-RJ ENetROSEInterface.asn1 ENetUC_Admin.asn1 ENetUC_Auth.asn1 ENetUC_AV.asn1 ENetUC_Chat.asn1 ENetUC_ChatV2.asn1 ENetUC_ClientContent.asn1 ENetUC_ClientUpdate.asn1 ENetUC_Common.asn1  ENetUC_PresenceV2.asn1  ENetUC_CTI.asn1 ENetUC_Journal.asn1 ENetUC_Transport.asn1 ENetUC_UMReplicator.asn1
 *
 */

#include "gen-java-code.h"

#include "../../../c-lib/include/print.h"
#include "../tag-util.h"  /* get GetTags/FreeTags/CountTags/TagByteLen */
#include "../str-util.h"
#include "../structure-util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if META
#include "meta.h"
#endif

static void PrintJavaType(FILE *hdr,Type *t);
static void PrintJavaTypeDefCode(TypeDef *td);

static FILE *getJavaFilePointer(char* pPrefix)
{
	FILE *p = NULL;
	size_t size = strlen(pPrefix) + 6;
	char* fileName = malloc(size); // strlen + .java + /0

	sprintf_s(fileName, size, "%s.java", pPrefix);

#ifdef _WIN32
	fopen_s(&p, fileName, "wt");
#else // _WIN32
	p = fopen(fileName, "wt");
#endif // _WIN32
	free(fileName);

	return p;
}

static char* getJavaClassName(char* prefix,char* suffix)
{
	size_t size = strlen(prefix) + strlen(suffix) + 1;
	char* className = malloc(size); // strlen + /0

	sprintf_s(className, size, "%s%s", prefix,suffix);
	className[0] = (char)toupper(className[0]);

	return className;
}

static void PrintJavaNativeType(FILE *hdr, Type *type) {
	switch(type->basicType->choiceId) {
		case BASICTYPE_BOOLEAN:
			fprintf (hdr, "Boolean");
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf (hdr, "Integer");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf (hdr, "String");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf (hdr, "%s", type->typeName); //FIXME
			break;
		case BASICTYPE_REAL:
			fprintf (hdr, "Double");
			break;
		case BASICTYPE_UTF8_STR:
			fprintf (hdr, "String");
			break;
		case BASICTYPE_UTCTIME:
			fprintf (hdr, "Date");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf (hdr, "Object");
			break;
		default:
			break;
			//assert(0 == 1);
	};
}

static void PrintJavaNativeTypeConstructor(FILE *hdr, Type *type) {
	switch(type->basicType->choiceId) {
		case BASICTYPE_BOOLEAN:
			fprintf (hdr, "false"); // Boolean
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf (hdr, "0");		// Integer
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf (hdr, "\"\""); // String
			break;
		case BASICTYPE_ENUMERATED:
			fprintf (hdr,"%s.values()[0]", type->typeName); //FIXME
			break;
		case BASICTYPE_REAL:
			fprintf (hdr, "0.0"); // Double
			break;
		case BASICTYPE_UTF8_STR:
			fprintf (hdr, "\"\""); // String
			break;
		case BASICTYPE_UTCTIME:
			fprintf (hdr, "new Date()");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf (hdr, "new Object()");
			break;
		default:
			break;
			//assert(0 == 1);
	};
}

static void PrintJavaTypeConstructor(FILE *hdr,Type *t)
{
	/*if(t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) {
		//PrintJavaArrayType(hdr, t->basicType->a.localTypeRef->link->type->basicType->a.sequence,t->basicType->a.localTypeRef->link);
	} else */
	if( 	t->basicType->choiceId == BASICTYPE_LOCALTYPEREF 
		&& 	t->basicType->a.localTypeRef->link != NULL 
		&& 	t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED
	)
	{
		fprintf (hdr,"%s.values()[0]",t->cxxTypeRefInfo->className); //FIXME
	} 
	else if  ( 	t->basicType->choiceId == BASICTYPE_IMPORTTYPEREF 
		&& 	t->basicType->a.importTypeRef->link != NULL 
		&& 	t->basicType->a.importTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED
	)
	{
		fprintf (hdr,"%s.values()[0]",t->cxxTypeRefInfo->className); //FIXME
	} 
	else
	{
		switch(t->basicType->choiceId) {
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
			fprintf (hdr, "new ArrayList<%s>()", t->cxxTypeRefInfo->className);
			break;
		default:
			fprintf (hdr, "new %s()", t->cxxTypeRefInfo->className);
			break;
		}
	}
}


static void PrintJavaArrayType(FILE *hdr, Type *t,TypeDef *innerType) {

	fprintf (hdr, "ArrayList<");
	PrintJavaType(hdr,t);
	fprintf (hdr, ">");

	switch(innerType->type->basicType->choiceId) {
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
			PrintJavaTypeDefCode(innerType);
	}
}
static void PrintJavaType(FILE *hdr,Type *t)
{
	/*if(t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) {
		//PrintJavaArrayType(hdr, t->basicType->a.localTypeRef->link->type->basicType->a.sequence,t->basicType->a.localTypeRef->link);
	} else */
	if( 	t->basicType->choiceId == BASICTYPE_LOCALTYPEREF 
	    && 	t->basicType->a.localTypeRef->link != NULL 
		&& 	t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_ENUMERATED) 
	{
		fprintf (hdr, "%s",t->cxxTypeRefInfo->className);
	} else 
	{
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
				PrintJavaNativeType(hdr, t);
				break;
			case BASICTYPE_SEQUENCEOF:
				PrintJavaArrayType(hdr, t, (TypeDef*)t->cxxTypeRefInfo);
				//fprintf (hdr, "[%s]", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf (hdr, "%s", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

static void PrintSeqJavaDataObjectClass(TypeDef *td)
{
	NamedType *e;
	char* name = getJavaClassName(td->definedName,"");
	char* tmpName;
	FILE* src = getJavaFilePointer (name);
	fprintf(src, "package com.estos.asn;\n\n");

	if(td->type->basicType->choiceId == BASICTYPE_SEQUENCEOF)
	{
		fprintf (src, "import java.util.ArrayList;\n");
		fprintf (src, "import java.util.List;\n");
		fprintf (src, "import javax.annotation.Generated;\n\n");
		fprintf (src, "@Generated(\"estosSNACC\")\n");

		fprintf(src, "public class %s extends ArrayList<",name);
		PrintJavaType(src,td->type->basicType->a.setOf);
		fprintf(src, ">{\n\n");
		fprintf(src, "	private static final long serialVersionUID = 1L;\n\n");
		fprintf(src, "	public %s(){\n	}\n\n",name);
		fprintf(src, "	public %s(List<",name);
		PrintJavaType(src,td->type->basicType->a.setOf);
		fprintf(src, "> values){\n\n");
		fprintf(src, "		super(values);\n");
		fprintf(src, "	}\n");

	}else
	{
		fprintf(src, "import java.io.Serializable;\n");
		fprintf(src, "import androidx.annotation.NonNull;\n");
		fprintf(src, "import androidx.annotation.Nullable;\n");
		fprintf(src, "import javax.annotation.Generated;\n");
		fprintf(src, "\n");

		fprintf(src, "@Generated(\"estosSNACC\")\n");
		fprintf(src, "public class %s implements Serializable{\n\n",name);
		fprintf(src, "	private static final long serialVersionUID = 1L;\n\n");

		FOR_EACH_LIST_ELMT (e, td->type->basicType->a.sequence)
		{

			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {

				fprintf(src, "	private ");
				PrintJavaType(src, e->type);
				fprintf(src, " %s=", e->fieldName);
				if (e->type->optional || e->type->basicType->choiceId == BASICTYPE_NULL) {
					fprintf(src, "null");
				}
				else {
					PrintJavaTypeConstructor(src, e->type);
				}
				fprintf(src, ";\n");

			}
		}

		FOR_EACH_LIST_ELMT (e, td->type->basicType->a.sequence)
		{
			if (e->type->basicType->choiceId != BASICTYPE_EXTENSION) {

				int isNullable = 0;
				if (e->type->optional || td->type->basicType->choiceId == BASICTYPE_CHOICE || td->type->basicType->choiceId == BASICTYPE_NULL) {
					isNullable = 1;

				}

				tmpName = getJavaClassName(e->fieldName, "");
				fprintf(src, "\n	");
				if(isNullable == 1) {
					fprintf(src, "@Nullable");
				} else {
					fprintf(src, "@NonNull");
				}
				fprintf(src, " public ");
				PrintJavaType(src, e->type);
				fprintf(src, " get%s(){\n", tmpName);
				fprintf(src, "		return this.%s;\n", e->fieldName);
				fprintf(src, "	}\n");

				fprintf(src, "	public void set%s(", tmpName);
				if (isNullable == 1) {
					fprintf(src, "@Nullable ");
				}
				else {
					fprintf(src, "@NonNull ");
				}
				PrintJavaType(src, e->type);
				fprintf(src, " %s){\n", e->fieldName);
				fprintf(src, "		this.%s=%s;\n", e->fieldName, e->fieldName);
				fprintf(src, "	}\n");
				free(tmpName);

			}
		}
	}
	fprintf (src, "\n");
	fprintf (src, "}\n");
	fclose (src);
	free(name);
}

static void PrintJavaChoiceDefCode(TypeDef *td)
{
	NamedType *e;
	char* name = getJavaClassName(td->definedName,"");
	char* tmpName;
	FILE* src = getJavaFilePointer (name);
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import java.io.Serializable;\n");
	fprintf(src, "import androidx.annotation.Nullable;\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	fprintf(src, "@Generated(\"estosSNACC\")\n");
	fprintf(src, "public class %s implements Serializable{\n\n",name);

	FOR_EACH_LIST_ELMT (e, td->type->basicType->a.choice)
	{

		fprintf (src, "	private ");
		PrintJavaType (src,e->type);
		fprintf (src, " %s=", e->fieldName);
		fprintf(src, "null");
		fprintf (src, ";\n");
	}
	FOR_EACH_LIST_ELMT (e, td->type->basicType->a.sequence)
	{

		tmpName=getJavaClassName(e->fieldName,"");
		fprintf (src, "\n	@Nullable public ");
		PrintJavaType (src,e->type);
		fprintf (src, " get%s(){\n", tmpName);
		fprintf (src, "		return this.%s;\n", e->fieldName);
		fprintf (src, "	}\n");

		fprintf (src, "	public void set%s(", tmpName);
		fprintf(src, "@Nullable ");
		PrintJavaType (src,e->type);
		fprintf (src, " %s){\n", e->fieldName);
		fprintf (src, "		this.%s=%s;\n", e->fieldName,e->fieldName);
		fprintf (src, "	}\n");
		free(tmpName);
	}
	fprintf (src, "\n");
	fprintf (src, "}\n");
	fclose (src);
	free(name);

}
static void PrintJavaSimpleRefDef(TypeDef *td) {

	char* name = getJavaClassName(td->definedName,"");
	FILE* src = getJavaFilePointer (name);
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	fprintf(src, "@Generated(\"estosSNACC\")\n");
	fprintf (src, "public class %s extends %s{\n", td->definedName, td->type->cxxTypeRefInfo->className);
	fprintf (src, "}\n\n");
	fclose (src);
	free(name);
}

static char* captalize(char* string)
{
	char* upper = calloc(sizeof(char),strlen(string)+1);
	size_t index;

	for(index=0; index<strlen(string); index++)
	{
		upper[index] = (char)toupper(string[index]);
	}

	return upper;

}
static void PrintJavaEnumDefCode( TypeDef *td)
{
	char* name = getJavaClassName(td->definedName,"");
	FILE* src = getJavaFilePointer (name);
	char* enumValue;
	CNamedElmt *n;
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf (src, "public enum %s{\n	", name);

	if (HasNamedElmts (td->type) != 0) {
			int count = 0;
			FOR_EACH_LIST_ELMT (n, td->type->cxxTypeRefInfo->namedElmts)
			{
				if(count>0)
					fprintf (src, ",");
				enumValue=captalize(n->name);
				fprintf (src, "%s(%d)", enumValue, n->value);
				free(enumValue);
				count++;
			}
	}

	fprintf (src, ";\n\n	private int value;");
	fprintf (src, ";\n\n	private %s(int value){\n		this.value=value;\n	}\n\n",name);

	fprintf (src, "\n\n	public int getValue(){\n		return value;\n	}\n\n");

	fprintf (src, "}\n\n");
	free(name);
	fclose (src);

}

static void PrintJavaSimpleDef( TypeDef *td) {

	char* name = getJavaClassName(td->definedName,"");

	FILE* src = getJavaFilePointer (name);
	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import javax.annotation.Generated;\n\n");
	fprintf(src, "@Generated(\"estosSNACC\")\n");
	fprintf (src, "public class %s extends SimpleJavaType<", name);
	if(strcmp("AsnSystemTime",name)==0){
		fprintf (src,"String");
	}else{
		PrintJavaNativeType(src, td->type);
	}
	fprintf (src, ">{\n\n");
	fprintf (src, "	public %s(){\n		super(",name);
	if(strcmp("AsnSystemTime",name)==0){
		fprintf (src,"new String()");
	}else{
		PrintJavaNativeTypeConstructor(src, td->type);
	}

	fprintf (src, ");\n	}\n\n");
	fprintf (src, "	public %s(",name);
	if(strcmp("AsnSystemTime",name)==0){
		fprintf (src,"String");
	}else{
		PrintJavaNativeType(src, td->type);
	}
	fprintf (src, " value){\n");
	fprintf (src, "		super(value);\n");
	fprintf (src, "	}");
	fprintf (src, "\n}\n\n");
	free(name);
	fclose (src);
}

static void PrintJavaTypeDefCode(TypeDef *td)
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
		PrintJavaSimpleDef (td);
		break;
	case BASICTYPE_SEQUENCEOF:  /* list types */
	case BASICTYPE_SETOF:
		PrintSeqJavaDataObjectClass (td);
		break;
	case BASICTYPE_IMPORTTYPEREF:  /* type references */
	case BASICTYPE_LOCALTYPEREF:
		/*
		* if this type has been re-tagged then
		* must create new class instead of using a typedef
		*/
		PrintJavaSimpleRefDef (td);
		break;
	case BASICTYPE_ANYDEFINEDBY:  /* ANY types */
	case BASICTYPE_ANY:
		//PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
		break;
	case BASICTYPE_CHOICE:
		PrintJavaChoiceDefCode(td);
		break;
	case BASICTYPE_ENUMERATED:  /* library type */
		PrintJavaEnumDefCode (td);
		break;
	case BASICTYPE_SET:
		//PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
		break;
	case BASICTYPE_SEQUENCE:
		PrintSeqJavaDataObjectClass (td);
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

static void PrintJavaOperationClass(Module *m, ValueDef *vd)
{
	char* pszArgument = NULL;
	char* pszResult = NULL;
	char* pszError = NULL;

	Type* argumentType = NULL;
	Type* resultType = NULL;
	Type* errorType = NULL;
	char* name = getJavaClassName(vd->definedName,"Operation");
	FILE* src = getJavaFilePointer (name);

	if (src == NULL)
	{
		perror ("fopen java file");
		return;
	}

	fprintf (src, "package com.estos.asn;\n\n");
	if (GetROSEDetails(m, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
	{
		// vars
		//fprintf(src, "  public class var name    : String { get { return \"%s\" } }\n", vd->definedName);
		//fprintf(src, "  public var operationName : String { get { return %s.name } }\n", vd->definedName);

		fprintf(src, "import javax.annotation.Generated;\n\n");
		fprintf(src, "@Generated(\"estosSNACC\")\n");

		fprintf(src, "public final class %s extends AbstractAsnOperation<",name);

		if(pszArgument) {
			fprintf(src, "%s,", pszArgument);
		}else{
			fprintf(src, "Void,");
		}

		if(pszResult) {
			fprintf(src, "%s,", pszResult);
		}else{
			fprintf(src, "Void,");
		}

		if(pszError) {
			fprintf(src, "%s>", pszError);
		}else{
			fprintf(src, "Void>");
		}
		fprintf(src, "{\n\n");

		fprintf(src, "	public Class<?> getAsnArgumentType(){\n		return ");
		//Class Type definititon
		if(pszArgument) {
			fprintf(src, "%s", pszArgument);
		}else{
			fprintf(src, "Void");
		}
		fprintf(src, ".class;\n	}\n\n");

		fprintf(src, "	public Class<?> getAsnResultType(){\n		return ");
		if(pszResult) {
			fprintf(src, "%s", pszResult);
		}else{
			fprintf(src, "Void");
		}
		fprintf(src, ".class;\n	}\n\n");

		fprintf(src, "	public Class<?> getAsnErrorType(){\n		return ");
		if(pszError) {
			fprintf(src, "%s", pszError);
		}else{
			fprintf(src, "Void");
		}
		fprintf(src, ".class;\n	}\n\n");

		fprintf(src, "}\n");
	}

	free(name);
	fclose (src);
}

static void PrintAbstractJavaOperation()
{
	FILE* src = getJavaFilePointer ("AbstractAsnOperation");

	if (src == NULL)
	{
		perror ("fopen java file");
		return;
	}

	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import com.google.gson.annotations.SerializedName;\n\n");
	fprintf(src, "public abstract class AbstractAsnOperation<ARGUMENT_TYPE,RESULT_TYPE,ERROR_TYPE>{\n\n");
	fprintf(src, "	@SerializedName(\"argument\")\n	private ARGUMENT_TYPE asnArgument;\n\n");
	fprintf(src, "	@SerializedName(\"result\")\n	private RESULT_TYPE asnResult;\n\n");
	fprintf(src, "	@SerializedName(\"error\")\n	private ERROR_TYPE asnError;\n\n");

	fprintf(src, "  public String getOperationName(){\n");
	fprintf(src, "		String name = getClass().getSimpleName();\n");
	fprintf(src, "		return Character.toString(name.charAt(0)).toLowerCase()+name.substring(1,name.length()-\"Operation\".length());\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnArgument(ARGUMENT_TYPE asnArgument){\n");
	fprintf(src, "		this.asnArgument = asnArgument;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public ARGUMENT_TYPE getAsnArgument(){\n");
	fprintf(src, "		return asnArgument;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnResult(RESULT_TYPE asnResult){\n");
	fprintf(src, "		this.asnResult = asnResult;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public RESULT_TYPE getAsnResult(){\n");
	fprintf(src, "		return asnResult;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public void setAsnError(ERROR_TYPE asnError){\n");
	fprintf(src, "		this.asnError = asnError;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public ERROR_TYPE getAsnError(){\n");
	fprintf(src, "		return asnError;\n");
	fprintf(src, "  }\n");

	fprintf(src, "	public abstract Class<?> getAsnArgumentType();\n\n");
	fprintf(src, "	public abstract Class<?> getAsnResultType();\n\n");
	fprintf(src, "	public abstract Class<?> getAsnErrorType();\n\n");

	fprintf(src, "}\n");
	fclose (src);
}

static void PrintSimpleJavaType()
{
	FILE* src = getJavaFilePointer ("SimpleJavaType");

	if (src == NULL)
	{
		perror ("fopen java file");
		return;
	}

	fprintf(src, "package com.estos.asn;\n\n");
	fprintf(src, "import java.io.Serializable;\n\n");
	fprintf(src, "public abstract class SimpleJavaType<T> implements Serializable{\n\n");
	fprintf(src, "  private final T value;\n\n");

	fprintf(src, "  public SimpleJavaType(T value){\n");
	fprintf(src, "		this.value=value;\n");
	fprintf(src, "  }\n");

	fprintf(src, "  public T getValue(){\n");
	fprintf(src, "		return value;\n");
	fprintf(src, "  }\n");

	fprintf(src, "}\n\n");
	fclose (src);
}



void PrintJAVACode(ModuleList *mods, Module *m) {
	ValueDef *vd;
	TypeDef* td;
	PrintAbstractJavaOperation();
	PrintSimpleJavaType();
	FOR_EACH_LIST_ELMT (vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
		{
			PrintJavaOperationClass(m, vd);
		}
	}

	FOR_EACH_LIST_ELMT (td, m->typeDefs)
	{
		PrintJavaTypeDefCode (td);
	}
}
