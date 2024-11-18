/*
 *   compiler/back_ends/java-gen/gen_code.c - routines for printing JAVA code from type trees
 *  -RJ ENetROSEInterface.asn1 ENetUC_Admin.asn1 ENetUC_Auth.asn1 ENetUC_AV.asn1 ENetUC_Chat.asn1 ENetUC_ChatV2.asn1 ENetUC_ClientContent.asn1 ENetUC_ClientUpdate.asn1 ENetUC_Common.asn1  ENetUC_PresenceV2.asn1  ENetUC_CTI.asn1 ENetUC_Journal.asn1 ENetUC_Transport.asn1 ENetUC_UMReplicator.asn1
 *
 */

#include "gen-kotlin-code.h"

#include "../../../c-lib/include/print.h"
#include "../../core/asn_comments.h"
#include "../../core/time_helpers.h"
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

void PrintKotlinType(FILE* hdr, ModuleList* mods, Module* mod, Type* t);
void PrintKotlinTypeDefCode(ModuleList* mods, Module* mod, TypeDef* td);
void PrintKotlinCodeOneModule(ModuleList* mods, Module* m);

void handleDeprecatedSequenceKotlin(FILE* src, Module* mod, TypeDef* td)
{
	if (IsDeprecatedFlaggedSequence(mod, td->definedName))
	{
		asnsequencecomment comment;
		if (GetSequenceComment_UTF8(mod->moduleName, td->definedName, &comment))
			fprintf(src, "  // CALL DeprecatedASN1Object(%lld, \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, mod->moduleName, td->definedName, comment.szDeprecated);
		fprintf(src, "\n\n");
	}
}

FILE* getKotlinFilePointer(char* pPrefix)
{
	char* fileName = MakeFileName(pPrefix, ".kt");

	FILE* p = NULL;
#ifdef _WIN32
	fopen_s(&p, fileName, "wt");
#else
	p = fopen(fileName, "wt");
#endif

	if (!p)
	{
		snacc_exit("File open failed %s", fileName);
		free(fileName);
		return p;
	}

	fprintf(p, "/**\n");
	fprintf(p, " * esnacc compiler generated Kotlin files\n");
	fprintf(p, " *\n");
	write_snacc_header(p, " * ");
	fprintf(p, " */\n\n");

	free(fileName);
	return p;
}

char* getKotlinClassName(char* prefix, char* suffix)
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

void PrintKotlinNativeType(FILE* hdr, Type* type)
{
	switch (type->basicType->choiceId)
	{
		case BASICTYPE_BOOLEAN:
			fprintf(hdr, "Boolean");
			break;
		case BASICTYPE_BITSTRING:
		case BASICTYPE_INTEGER:
			fprintf(hdr, "Int");
			break;
		case BASICTYPE_OCTETSTRING:
		case BASICTYPE_OCTETCONTAINING:
			fprintf(hdr, "String");
			break;
		case BASICTYPE_ENUMERATED:
			fprintf(hdr, "%s?", type->typeName); // FIXME
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
			fprintf(hdr, "Any");
			break;
		default:
			break;
			// assert(0 == 1);
	};
}

void PrintKotlinNativeTypeConstructor(FILE* hdr, Type* type)
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
			fprintf(hdr, "Date()");
			break;
		case BASICTYPE_UNKNOWN:
		case BASICTYPE_NULL:
			fprintf(hdr, "Any()");
			break;
		default:
			break;
			// assert(0 == 1);
	};
}

void PrintKotlinTypeConstructor(FILE* hdr, ModuleList* mods, Module* mod, Type* t)
{
	/*if(t->basicType->choiceId == BASICTYPE_LOCALTYPEREF && t->basicType->a.localTypeRef->link != NULL && t->basicType->a.localTypeRef->link->cxxTypeDefInfo->asn1TypeId == BASICTYPE_SEQUENCEOF) {
	  //PrintKotlinArrayType(hdr, t->basicType->a.localTypeRef->link->type->basicType->a.sequence,t->basicType->a.localTypeRef->link);
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
				PrintKotlinNativeTypeConstructor(hdr, t);
				break;
			case BASICTYPE_SEQUENCEOF:
				fprintf(hdr, "ArrayList<%s>()", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf(hdr, "%s()", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

void PrintKotlinArrayType(FILE* hdr, ModuleList* mods, Module* mod, Type* t, TypeDef* innerType)
{

	fprintf(hdr, "ArrayList<");
	PrintKotlinType(hdr, mods, mod, t);
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
			PrintKotlinTypeDefCode(mods, mod, innerType);
	}
}
void PrintKotlinType(FILE* hdr, ModuleList* mods, Module* mod, Type* t)
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
				PrintKotlinNativeType(hdr, t);
				break;
			case BASICTYPE_SEQUENCEOF:
				PrintKotlinArrayType(hdr, mods, mod, t, (TypeDef*)t->cxxTypeRefInfo);
				// fprintf (hdr, "[%s]", t->cxxTypeRefInfo->className);
				break;
			default:
				fprintf(hdr, "%s", t->cxxTypeRefInfo->className);
				break;
		}
	}
}

void PrintSeqKotlinDataSequenceOf(ModuleList* mods, Module* mod, TypeDef* td)
{
	char* name = getKotlinClassName(td->definedName, "");
	char* serializerName = getKotlinClassName(td->definedName, "Serializer");
	
	int isCustomSerializer = 0;
	if (strcmp("AsnOptionalParameters", name) == 0)
			isCustomSerializer = 1;
	
	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING
	//printing main file
	fprintf(src, "package com.estos.asn\n\n");
	if (isCustomSerializer == 1)
			fprintf(src, "import com.estos.asnconnector.util.kserialize.AsnOptionalParametersSerializer\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "@Serializable(with = %s::class)\n", serializerName);
	fprintf(src, "class %s : ArrayList<", name);
	PrintKotlinType(src, mods, mod, td->type->basicType->a.setOf);
	fprintf(src, "> {\n");
	handleDeprecatedSequenceKotlin(src, mod, td);
	fprintf(src, "  constructor(values: List<");
	PrintKotlinType(src, mods, mod, td->type->basicType->a.setOf);
	fprintf(src, ">) : super(values)\n");
	fprintf(src, "  constructor() : super()\n");
	fprintf(src, "}\n");
	
	if (isCustomSerializer == 0) {
	//printing serializer for file
	FILE* serializerSrc = getKotlinFilePointer(serializerName);
	fprintf(serializerSrc, "package com.estos.asn\n\n");
	fprintf(serializerSrc, "import kotlinx.serialization.KSerializer\n");
	fprintf(serializerSrc, "import kotlinx.serialization.SerializationException\n");
	fprintf(serializerSrc, "import kotlinx.serialization.builtins.ListSerializer\n");
	fprintf(serializerSrc, "import kotlinx.serialization.builtins.serializer\n");
	fprintf(serializerSrc, "import kotlinx.serialization.descriptors.SerialDescriptor\n");
	fprintf(serializerSrc, "import kotlinx.serialization.encoding.Decoder\n");
	fprintf(serializerSrc, "import kotlinx.serialization.encoding.Encoder\n");
	fprintf(serializerSrc, "import kotlinx.serialization.json.JsonDecoder\n");
	fprintf(serializerSrc, "import kotlinx.serialization.json.jsonArray\n\n");
	fprintf(serializerSrc, "class %s : KSerializer<", serializerName);
	fprintf(serializerSrc, "%s> {\n", name);
	fprintf(serializerSrc, "    private val elementSerializer: KSerializer<");
	PrintKotlinType(serializerSrc, mods, mod, td->type->basicType->a.setOf);
	fprintf(serializerSrc, "> = ");
	PrintKotlinType(serializerSrc, mods, mod, td->type->basicType->a.setOf);
	fprintf(serializerSrc, ".serializer()\n");
	fprintf(serializerSrc, "    private val listSerializer = ListSerializer(elementSerializer)\n");
	fprintf(serializerSrc, "    override val descriptor: SerialDescriptor = listSerializer.descriptor\n\n");
	fprintf(serializerSrc, "    override fun serialize(encoder: Encoder, value: %s) {\n", name);
	fprintf(serializerSrc, "        listSerializer.serialize(encoder, value)\n");
	fprintf(serializerSrc, "    }\n\n");
	fprintf(serializerSrc, "    @Throws(SerializationException::class)\n");
	fprintf(serializerSrc, "    override fun deserialize(decoder: Decoder): %s {\n", name);
	fprintf(serializerSrc, "        val result = ArrayList(with(decoder as JsonDecoder) {\n");
	fprintf(serializerSrc, "           decodeJsonElement().jsonArray.mapNotNull {\n");
	fprintf(serializerSrc, "              json.decodeFromJsonElement(elementSerializer, it)\n");
	fprintf(serializerSrc, "           }\n");
	fprintf(serializerSrc, "        })\n");
	fprintf(serializerSrc, "        return %s(result)\n", name);
	fprintf(serializerSrc, "     }\n\n");
	fprintf(serializerSrc, "}\n");
	fclose(serializerSrc);
	}
	
	free(serializerName);
	fclose(src);
	free(name);
}

void PrintSeqKotlinDataSequence(ModuleList* mods, Module* mod, TypeDef* td)
{
	NamedType* e;
	char* name = getKotlinClassName(td->definedName, "");
	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	if (strcmp("AsnRequestError", name) == 0)
		fprintf(src, "import com.estos.asnconnector.util.kserialize.AsnRequestErrorSerializer\n");
	fprintf(src, "import kotlinx.serialization.Contextual\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n");
	fprintf(src, "\n");

	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	if (strcmp("AsnRequestError", name) == 0)
		fprintf(src, "@Serializable(with = AsnRequestErrorSerializer::class)\n");
	else
		fprintf(src, "@Serializable\n");
	fprintf(src, "open class %s : java.io.Serializable {\n", name);
	handleDeprecatedSequenceKotlin(src, mod, td);
	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.sequence)
	{
		if (e->type->basicType->choiceId == BASICTYPE_EXTENSION)
			continue;

		int isNullable = 0;
		if (e->type->optional || td->type->basicType->choiceId == BASICTYPE_CHOICE || td->type->basicType->choiceId == BASICTYPE_NULL)
			isNullable = 1;
			
		printMemberComment(src, mod, td, e->fieldName, "  ", COMMENTSTYLE_JAVA);
		if (e->type->basicType->choiceId == BASICTYPE_UNKNOWN || e->type->basicType->choiceId == BASICTYPE_NULL)
			fprintf(src, "  @Contextual\n");
		fprintf(src, "  var ");
		char* szFieldName = Dash2UnderscoreEx(e->fieldName);
		fprintf(src, " %s:", szFieldName);
		PrintKotlinType(src, mods, mod, e->type);	
		if (isNullable == 1)
			fprintf(src, "?");
		fprintf(src, " = ");
		free(szFieldName);
		if (isNullable == 1)
			fprintf(src, "null");
		else
			PrintKotlinTypeConstructor(src, mods, mod, e->type);
		fprintf(src, "\n");
	}
	fprintf(src, "  companion object {\n");
	fprintf(src, "      private const val serialVersionUID = 1L\n");
	fprintf(src, "  }\n");
	fprintf(src, "}\n");
	fclose(src);
	free(name);
}

void PrintKotlinChoiceDefCode(ModuleList* mods, Module* mod, TypeDef* td)
{
	NamedType* e;
	char* name = getKotlinClassName(td->definedName, "");
	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	fprintf(src, "import kotlinx.serialization.Contextual\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "@Serializable\n");
	fprintf(src, "open class %s : java.io.Serializable {\n\n", name);
	handleDeprecatedSequenceKotlin(src, mod, td);

	FOR_EACH_LIST_ELMT(e, td->type->basicType->a.choice)
	{
		if (e->type->basicType->choiceId == BASICTYPE_UNKNOWN || e->type->basicType->choiceId == BASICTYPE_NULL)
			fprintf(src, "  @Contextual\n");
		fprintf(src, "  var %s:", e->fieldName);
		PrintKotlinType(src, mods, mod, e->type);
		fprintf(src, "? = null\n");
	}
	fprintf(src, "\n");
	fprintf(src, "}\n");
	fclose(src);
	free(name);
}
void PrintKotlinSimpleRefDef(ModuleList* mods, Module* mod, TypeDef* td)
{

	char* name = getKotlinClassName(td->definedName, "");
	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	if (strcmp("AsnGetLocationInformationResult", name) == 0)
		fprintf(src, "import androidx.annotation.Keep\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "@Serializable\n");
	if (strcmp("AsnGetLocationInformationResult", name) == 0)
		fprintf(src, "@Keep\n");
	fprintf(src, "class %s : %s() {\n", td->definedName, td->type->cxxTypeRefInfo->className);
	fprintf(src, "}\n\n");
	fclose(src);
	free(name);
}

char* capitalize(char* string)
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
void PrintKotlinEnumDefCode(ModuleList* mods, Module* mod, TypeDef* td)
{
	char* name = getKotlinClassName(td->definedName, "");
	char* serializerName = getKotlinClassName(td->definedName, "Serializer");
	FILE* src = getKotlinFilePointer(name);
	FILE* serializerSrc = getKotlinFilePointer(serializerName);
	PRINTDEBUGGING
	char* enumValue;
	CNamedElmt* n;
	fprintf(src, "package com.estos.asn\n\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	fprintf(src, "@Serializable(with = %s::class)\n", serializerName);
	fprintf(src, "enum class %s(val value: Int) {\n", name);

	if (HasNamedElmts(td->type) != 0)
	{
		int count = 0;
		FOR_EACH_LIST_ELMT(n, td->type->cxxTypeRefInfo->namedElmts)
		{
			if (count > 0)
				fprintf(src, ",\n");
			printMemberComment(src, mod, td, n->name, "  ", COMMENTSTYLE_JAVA);
			enumValue = capitalize(n->name);
			fprintf(src, "  %s(%d)", enumValue, n->value);
			free(enumValue);
			count++;
		}
	}
	fprintf(src, ";\n\n");
	fprintf(src, "  companion object {\n");
	fprintf(src, "      fun fromInt(value: Int) = %s.values().first { it.value == value }\n", name);
	fprintf(src, "  }\n");
	fprintf(src, "}\n");
	
	//printing serializer for file
	fprintf(serializerSrc, "package com.estos.asn\n\n");
	fprintf(serializerSrc, "import kotlinx.serialization.KSerializer\n");
	fprintf(serializerSrc, "import kotlinx.serialization.descriptors.PrimitiveKind\n");
	fprintf(serializerSrc, "import kotlinx.serialization.descriptors.PrimitiveSerialDescriptor\n");
	fprintf(serializerSrc, "import kotlinx.serialization.descriptors.SerialDescriptor\n");
	fprintf(serializerSrc, "import kotlinx.serialization.encoding.Decoder\n");
	fprintf(serializerSrc, "import kotlinx.serialization.encoding.Encoder\n");
	fprintf(serializerSrc, "class %s : KSerializer<", serializerName);
	fprintf(serializerSrc, "%s> {\n", name);
	fprintf(serializerSrc, "    override val descriptor: SerialDescriptor = PrimitiveSerialDescriptor(\"%s\", PrimitiveKind.INT)\n\n", serializerName);
	fprintf(serializerSrc, "    override fun serialize(encoder: Encoder, value: %s) {\n", name);
	fprintf(serializerSrc, "        encoder.encodeInt(value.value)\n");
	fprintf(serializerSrc, "    }\n\n");
	fprintf(serializerSrc, "    override fun deserialize(decoder: Decoder): %s {\n", name);
	fprintf(serializerSrc, "        return %s.fromInt(decoder.decodeInt())\n", name);
	fprintf(serializerSrc, "    }\n\n");
	fprintf(serializerSrc, "}\n");
	
	fclose(serializerSrc);
	free(serializerName);
	free(name);
	fclose(src);
}

void PrintKotlinSimpleDef(ModuleList* mods, Module* mod, TypeDef* td)
{

	char* name = getKotlinClassName(td->definedName, "");

	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "import com.estos.asnconnector.util.kserialize.AsnSystemTimeSerializer\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import javax.annotation.Generated\n\n");
	printSequenceComment(src, mod, td, COMMENTSTYLE_JAVA);
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "@Serializable(with = AsnSystemTimeSerializer::class)\n");
	else
		fprintf(src, "@Serializable\n");
	fprintf(src, "class %s : SimpleJavaType<", name);
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "String?");
	else
		PrintKotlinNativeType(src, td->type);
	fprintf(src, ">{\n\n");
	handleDeprecatedSequenceKotlin(src, mod, td);
	fprintf(src, "  constructor() : super(");
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "String()");
	else
		PrintKotlinNativeTypeConstructor(src, td->type);

	fprintf(src, ")\n\n");
	fprintf(src, "  constructor(value:");
	if (strcmp("AsnSystemTime", name) == 0)
		fprintf(src, "String?");
	else
		PrintKotlinNativeType(src, td->type);
	fprintf(src, " ) : super(value)\n");
	fprintf(src, "}\n\n");
	free(name);
	fclose(src);
}

void PrintKotlinTypeDefCode(ModuleList* mods, Module* mod, TypeDef* td)
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
			PrintKotlinSimpleDef(mods, mod, td);
			break;
		case BASICTYPE_SEQUENCEOF: /* list types */
		case BASICTYPE_SETOF:
			PrintSeqKotlinDataSequenceOf(mods, mod, td);
			break;
		case BASICTYPE_IMPORTTYPEREF: /* type references */
		case BASICTYPE_LOCALTYPEREF:
			/*
			 * if this type has been re-tagged then
			 * must create new class instead of using a typedef
			 */
			PrintKotlinSimpleRefDef(mods, mod, td);
			break;
		case BASICTYPE_ANYDEFINEDBY: /* ANY types */
		case BASICTYPE_ANY:
			// PrintCxxAnyDefCode (src, hdr, mods, m, r, td, NULL, td->type);
			break;
		case BASICTYPE_CHOICE:
			PrintKotlinChoiceDefCode(mods, mod, td);
			break;
		case BASICTYPE_ENUMERATED: /* library type */
			PrintKotlinEnumDefCode(mods, mod, td);
			break;
		case BASICTYPE_SET:
			// PrintCxxSetDefCode (src, hdr, mods, m, r, td, NULL, td->type, novolatilefuncs);
			break;
		case BASICTYPE_SEQUENCE:
			PrintSeqKotlinDataSequence(mods, mod, td);
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

void PrintKotlinOperationClass(Module* mod, ValueDef* vd)
{
	if (IsDeprecatedNoOutputOperation(mod, vd->definedName))
		return;

	const char* pszArgument = NULL;
	const char* pszResult = NULL;
	const char* pszError = NULL;

	Type* argumentType = NULL;
	Type* resultType = NULL;
	Type* errorType = NULL;
	char* name = getKotlinClassName(vd->definedName, "Operation");
	FILE* src = getKotlinFilePointer(name);
	PRINTDEBUGGING

	fprintf(src, "package com.estos.asn\n\n");
	if (GetROSEDetails(mod, vd, &pszArgument, &pszResult, &pszError, &argumentType, &resultType, &errorType, false))
	{
		fprintf(src, "import kotlinx.serialization.Polymorphic\n");
		fprintf(src, "import kotlinx.serialization.Serializable\n");
		fprintf(src, "import kotlin.reflect.KClass\n\n");
		printOperationComment(src, mod, vd->definedName, COMMENTSTYLE_JAVA);
		fprintf(src, "@Serializable\n");
		fprintf(src, "class %s : AbstractAsnOperation<", name);

		if (pszArgument)
			fprintf(src, "%s?, ", pszArgument);
		else
			fprintf(src, "SerialVoid?, ");

		if (pszResult)
			fprintf(src, "%s?, ", pszResult);
		else
			fprintf(src, "SerialVoid?, ");

		if (pszError)
			fprintf(src, "%s?>()", pszError);
		else
			fprintf(src, "SerialVoid?>()");
		fprintf(src, " {\n");

		if (IsDeprecatedFlaggedOperation(mod, vd->definedName))
		{
//			fprintf(src, "  public %s()\n", name);
//			fprintf(src, "  {\n");
			asnoperationcomment comment;
			if (GetOperationComment_UTF8(mod->moduleName, vd->definedName, &comment))
				fprintf(src, "  // CALL DeprecatedASN1Method(%lld, \"%s\", \"%s\", \"%s\")\n", comment.i64Deprecated, mod->moduleName, vd->definedName, comment.szDeprecated);
			fprintf(src, "\n\n");
		}

		// Class Type definititon
		if (pszArgument) {
			fprintf(src, "  override val asnArgumentType: KClass<%s>\n", pszArgument);
			fprintf(src, "    get() = %s::class\n\n", pszArgument);
		}
		else {
			fprintf(src, "  override val asnArgumentType: KClass<SerialVoid>\n");
			fprintf(src, "    get() = SerialVoid::class\n\n");
		}
		if (pszResult) {
			fprintf(src, "  override val asnResultType: KClass<%s>\n", pszResult);
			fprintf(src, "    get() = %s::class\n\n", pszResult);
		}
		else {
			fprintf(src, "  override val asnResultType: KClass<SerialVoid>\n");
			fprintf(src, "    get() = SerialVoid::class\n\n");
		}
		if (pszError) {
			fprintf(src, "  override val asnErrorType: KClass<%s>\n", pszError);
			fprintf(src, "    get() = %s::class\n\n", pszError);
		}
		else {
			fprintf(src, "  override val asnErrorType: KClass<SerialVoid>\n");
			fprintf(src, "    get() = SerialVoid::class\n\n");
		}
		fprintf(src, "}\n");
	}

	free(name);
	fclose(src);
}

void PrintAbstractKotlinOperation()
{
	FILE* src = getKotlinFilePointer("AbstractAsnOperation");
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	fprintf(src, "import kotlinx.serialization.SerialName\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n");
	fprintf(src, "import kotlin.reflect.KClass\n");
	fprintf(src, "import java.util.Locale\n\n");
	fprintf(src, "@Serializable\n");
	fprintf(src, "abstract class AbstractAsnOperation<ARGUMENT_TYPE, RESULT_TYPE, ERROR_TYPE> {\n\n");
	fprintf(src, "  @SerialName(\"argument\")\n  var asnArgument: ARGUMENT_TYPE? = null\n\n");
	fprintf(src, "  @SerialName(\"result\")\n  var asnResult: RESULT_TYPE? = null\n\n");
	fprintf(src, "  @SerialName(\"error\")\n  var asnError: ERROR_TYPE? = null\n\n");

	fprintf(src, "  val operationName: String\n");
	fprintf(src, "    get() {\n");
	fprintf(src, "        val name = javaClass.simpleName\n");
	fprintf(src, "        return name[0].toString().lowercase(Locale.getDefault()) + name.substring(1, name.length - \"Operation\".length)\n");
	fprintf(src, "    }\n");

	fprintf(src, "  abstract val asnArgumentType: KClass<*>?\n\n");
	fprintf(src, "  abstract val asnResultType: KClass<*>?\n\n");
	fprintf(src, "  abstract val asnErrorType: KClass<*>?\n\n");
	
	fprintf(src, "  fun setArgument(arg: Any) {\n");
	fprintf(src, "    asnArgument = arg as ARGUMENT_TYPE\n");
	fprintf(src, "  }\n\n");
	fprintf(src, "  fun setResult(arg: Any) {\n");
	fprintf(src, "    asnResult = arg as RESULT_TYPE\n");
	fprintf(src, "  }\n\n");
	fprintf(src, "  fun setError(arg: Any) {\n");
	fprintf(src, "    asnError = arg as ERROR_TYPE\n");
	fprintf(src, "  }\n\n");

	fprintf(src, "}\n");
	fclose(src);
}

void PrintSimpleKotlinType()
{
	FILE* src = getKotlinFilePointer("SimpleJavaType");
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n\n");
	fprintf(src, "@Serializable\n");
	fprintf(src, "abstract class SimpleJavaType<T>(val value: T) : java.io.Serializable \n\n");
	fclose(src);
}

void PrintSerialVoid()
{
	FILE* src = getKotlinFilePointer("SerialVoid");
	PRINTDEBUGGING
	fprintf(src, "package com.estos.asn\n\n");
	fprintf(src, "import kotlinx.serialization.Serializable\n\n");
	fprintf(src, "@Serializable\n");
	fprintf(src, "class SerialVoid \n\n");
	fclose(src);
}

void PrintKotlinCode(ModuleList* allMods)
{
	Module* currMod;
	AsnListNode* saveMods;
	DefinedObj* fNames;
	int fNameConflict = FALSE;

	/*
	 * Make names for each module's encoder/decoder src and hdr files
	 * so import references can be made via include files
	 * check for truncation --> name conflicts & exit if nec
	 */
	fNames = NewObjList();

	if (gMajorInterfaceVersion >= 0)
	{
		char szFileName[_MAX_PATH] = {0};
		strcpy_s(szFileName, _MAX_PATH, gszOutputPath);
		strcat_s(szFileName, _MAX_PATH, "Asn1InterfaceVersion");
		FILE* src = getKotlinFilePointer(szFileName);
		if (src)
		{
			long long lMaxMinorVersion = GetMaxModuleMinorVersion();
			fprintf(src, "object Asn1InterfaceVersion {\n");
			fprintf(src, "\tconst val lastChange: String = \"%s\"\n", ConvertUnixTimeToISO(lMaxMinorVersion));
			fprintf(src, "\tconst val majorVersion: Int = %i\n", gMajorInterfaceVersion);
			fprintf(src, "\tconst val minorVersion: Long = %lld\n", lMaxMinorVersion);
			fprintf(src, "\tconst val version: String = \"%i.%lld.0\"\n", gMajorInterfaceVersion, lMaxMinorVersion);
			fprintf(src, "}\n");
			fclose(src);
		}
	}

	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (ObjIsDefined(fNames, currMod->ROSESrcJAVAFileName, StrObjCmp))
		{
			fprintf(errFileG, "Ack! ERROR---file name conflict for generated source files with names `%s'.\n\n", currMod->ROSESrcJAVAFileName);
			fprintf(errFileG, "This usually means the max file name length is truncating the file names.\n");
			fprintf(errFileG, "Try re-naming the modules with shorter names or increasing the argument to -mf option (if you are using it).\n");
			fprintf(errFileG, "This error can also be caused by 2 modules having the same name but different OBJECT IDENTIFIERs.");
			fprintf(errFileG, "Try renaming the modules to correct this.\n");
			fNameConflict = TRUE;
		}
		else
		{
			DefineObj(&fNames, currMod->ROSESrcJAVAFileName);
		}

		if (fNameConflict)
			return;

		FreeDefinedObjs(&fNames);
	}
	FOR_EACH_LIST_ELMT(currMod, allMods)
	{
		if (currMod->ImportedFlag == FALSE)
		{
			saveMods = allMods->curr;
			PrintKotlinCodeOneModule(allMods, currMod);
			allMods->curr = saveMods;
		}
	}
}

void PrintKotlinCodeOneModule(ModuleList* mods, Module* m)
{
	if (gMajorInterfaceVersion >= 0)
	{
		FILE* src = getKotlinFilePointer(m->baseFilePath);
		if (src)
		{
			long long lMinorModuleVersion = GetModuleMinorVersion(m->moduleName);
			fprintf(src, "object %s {\n", m->moduleName);
			fprintf(src, "\tconst val lastChange: String = \"%s\"\n", ConvertUnixTimeToISO(lMinorModuleVersion));
			fprintf(src, "\tconst val majorVersion: Int = %i\n", gMajorInterfaceVersion);
			fprintf(src, "\tconst val minorVersion: Long = %lld\n", lMinorModuleVersion);
			fprintf(src, "\tconst val version: String = \"%i.%lld.0\"\n", gMajorInterfaceVersion, lMinorModuleVersion);
			fprintf(src, "}\n");
			fclose(src);
		}
	}

	ValueDef* vd;
	TypeDef* td;
	PrintAbstractKotlinOperation();
	PrintSimpleKotlinType();
	PrintSerialVoid();
	FOR_EACH_LIST_ELMT(vd, m->valueDefs)
	{
		if (vd->value->type->basicType->choiceId == BASICTYPE_MACROTYPE)
			PrintKotlinOperationClass(m, vd);
	}

	FOR_EACH_LIST_ELMT(td, m->typeDefs)
	{
		PrintKotlinTypeDefCode(mods, m, td);
	}
}
