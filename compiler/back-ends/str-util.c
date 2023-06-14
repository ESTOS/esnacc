/*
 * compiler/back_ends/c_gen/str_util.c  - bunch of ASN.1/C string utilities
 *
 *
 * Mike Sample
 * 91/08/12
 * Copyright (C) 1991, 1992 Michael Sample
 *            and the University of British Columbia
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * $Header: /develop30/common/esnacc1.7/SNACC/compiler/back-ends/str-util.c,v 1.1.1.1 2005/04/14 14:59:42 \ste Exp $
 * $Log: str-util.c,v $
 * Revision 1.1.1.1  2005/04/14 14:59:42  \ste
 * no message
 *
 * Revision 1.10  2004/03/22 20:04:06  gronej
 * took IBM references out of the code (to the best of our knowledge, we don't use any of it anymore)
 *
 * Revision 1.9  2003/07/07 14:54:24  nicholar
 * Eliminated headers and cleaned up include references
 *
 * Revision 1.8  2002/09/16 16:56:31  mcphersc
 * iFixed warnings
 *
 * Revision 1.7  2002/09/05 17:43:16  vracarl
 * got rid of c++ comments
 *
 * Revision 1.6  2002/07/11 18:46:19  leonberp
 * change MakeFileName to only return filename from path
 *
 * Revision 1.5  2001/07/12 19:34:18  leonberp
 * Changed namespace to SNACC and added compiler options: -ns and -nons.  Also removed dead code.
 *
 * Revision 1.4  2001/05/07 15:05:40  mcphersc
 * Added sub argument to the -C option to allow for .cpp file extension.
 * Use -C cpp option for .cpp file extensions
 *
 * Revision 1.3  2000/11/01 17:49:52  sfl
 * UPDATED to fix removal of "char pathname[1024];" for #ifdef build on Unix
 * (removed from PC based on warning implying unused; necessary on Unix based
 * on #ifdef; replaced with #ifdef to avoid PC warning).
 *
 * Revision 1.2  2000/10/24 14:54:43  rwc
 * Updated to remove high-level warnings (level 4 on MSVC++) for an easier build.
 * SOME warnings persist due to difficulty in modifying the SNACC compiler to
 * properly build clean source; also some files are built by Lex/Yacc.
 *
 * Revision 1.1.1.1  2000/08/21 20:36:04  leonberp
 * First CVS Version of SNACC.
 *
 * Revision 1.4  1995/07/25  18:13:31  rj
 * include string(s).h
 *
 * additional filename generator for idl backend.
 *
 * changed `_' to `-' in file names.
 *
 * Revision 1.3  1994/10/08  03:48:17  rj
 * since i was still irritated by cpp standing for c++ and not the C preprocessor, i renamed them to cxx (which is one known suffix for C++ source files). since the standard #define is __cplusplus, cplusplus would have been the more obvious choice, but it is a little too long.
 *
 * Revision 1.2  1994/09/01  00:25:31  rj
 * snacc_config.h removed; more portable .h file inclusion.
 *
 * Revision 1.1  1994/08/28  09:48:37  rj
 * first check-in. for a list of changes to the snacc-1.1 distribution please refer to the ChangeLog.
 *
 */

#include "../../c-lib/include/asn-incl.h"

#include <ctype.h>
#if HAVE_UNISTD_H
#include <unistd.h> /* for pathconf (..) */
#endif
#include <string.h>
#include <stdio.h>

#include "../core/asn1module.h"
#include "c-gen/rules.h"
#include "c-gen/type-info.h"
#include "str-util.h"

#define DIGIT_TO_ASCII(d) (((d) % 10) + '0')

char gszOutputPath[100] = {0};

int IsCKeyWord PROTO((char* str));
int IsCxxKeyWord PROTO((char* str));

/*
 * allocates new and returns a copy of the given
 * string with '-'s (dashes) replaced by  '_'s (underscores)
 */
char* Asn1TypeName2CTypeName PARAMS((aName), const char* aName)
{
	char* retVal;
	if (aName == NULL)
		return NULL;

	size_t size = strlen(aName) + 1;
	retVal = Malloc(size);
	strcpy_s(retVal, size, aName);
	Dash2Underscore(retVal, strlen(retVal));

	return retVal;
} /* Asn1TypeName2CTypeName */

/*
 * allocates new str and returns a copy of the given
 * string with '-'s (dashes) replaced by  '_'s (underscores)
 */
char* Asn1FieldName2CFieldName PARAMS((aName), char* aName)
{
	char* retVal;
	if (aName == NULL)
		return NULL;

	size_t size = strlen(aName) + 1;
	retVal = Malloc(size);
	strcpy_s(retVal, size, aName);
	Dash2Underscore(retVal, strlen(retVal));

	return retVal;
} /* Asn1FieldName2CFieldName */

/*
 * allocates new str and returns a copy of the given
 * string with '-'s (dashes) replaced by  '_'s (underscores)
 */
char* Asn1ValueName2CValueName PARAMS((aName), char* aName)
{
	char* retVal;
	if (aName == NULL)
		return NULL;

	size_t size = strlen(aName) + 1;
	retVal = Malloc(size);
	strcpy_s(retVal, size, aName);
	Dash2Underscore(retVal, strlen(retVal));

	return retVal;
} /* Asn1FieldName2CFieldName */

/*
 * allocates and returns a string with all of
 * the caps from the given string
 */
char* GetCaps PARAMS((str), char* str)
{
	int i, j;
	char* retVal;

	if (str == NULL)
		return NULL;

	retVal = Malloc(strlen(str) + 1);

	for (j = 0, i = 0; i < (int)strlen(str); i++)
		if (isupper(str[i]))
			retVal[j++] = str[i];

	retVal[j] = '\0'; /* null terminate */

	return retVal;

} /* GetCaps */

/*
 * allocates and returns a string with all of
 * the caps and digits from the given string
 */
char* GetCapsAndDigits PARAMS((str), char* str)
{
	int i, j;
	char* retVal;

	if (str == NULL)
		return NULL;

	retVal = Malloc(strlen(str) + 1);

	for (j = 0, i = 0; i < (int)strlen(str); i++)
		if ((isupper(str[i])) || (isdigit(str[i])))
			retVal[j++] = str[i];

	retVal[j] = '\0'; /* null terminate */

	return retVal;

} /* GetCapsAndDigits */

/*
 * replaces lowercase chars in given str
 * with upper case version
 * NOTE: modifies given str
 */
void Str2UCase PARAMS((str, len), char* str _AND_ size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		if (islower(str[i]))
			str[i] = (char)toupper(str[i]);
} /* Str2UCase */

/*
 * replaces uppercase chars in given str
 * with lower case version
 * NOTE: modifies given str
 */
void Str2LCase PARAMS((str, len), char* str _AND_ size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		if (isupper(str[i]))
			str[i] = (char)tolower(str[i]);
} /* Str2LCase */

/*
 * replace dash chars in given str
 * with underscores
 * NOTE: modifies given str
 */
void Dash2Underscore PARAMS((str, len), char* str _AND_ size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		if (str[i] == '-')
			str[i] = '_';
} /* Dash2Underscore */

/*
 * tacks on the ascii version of the given digit
 * at the end of the given str.
 * NOTE: make sure the str you give has enough space
 * for the digits
 */
void AppendDigit(char* str, size_t bufferSize, int digit)
{
	int high = 1000000000;
	int currDigit;
	int value;
	char digitStr[20]; /* arbitrary length > max */

	if (digit < 0)
		digit *= -1;

	currDigit = 0;
	while (high > 0)
	{
		value = digit / high;
		if (value != 0)
			digitStr[currDigit++] = (char)DIGIT_TO_ASCII(value);

		digit = digit % high;
		high = high / 10;
	}

	if (currDigit == 0)
		strcat_s(str, bufferSize, "0");
	else
	{
		digitStr[currDigit] = '\0'; /* null terminate */
		strcat_s(str, bufferSize, digitStr);
	}
} /* AppendDigit */

/*
 * given a defined object list containing null termintated strs,
 * a str to be made unique wrt to the list by adding digits to the
 * end, the max number of digits to add and the digit to start
 * at, str is modified to be unique.  It is not added to the
 * defined object list.  The given str must have enough spare,
 * allocated chars after it's null terminator to hold maxDigits
 * more characters.
 * Only appends digits if the string is not unique or is a C keyword.
 *
 * Eg  MakeCStrUnique ({ "Foo", "Bar" }, "Foo\0   ", 3, 1)
 *         modifies the the Str "Foo" to "Foo1"
 */
void MakeCStrUnique PARAMS((nameList, str, bufferSize, maxDigits, startingDigit), DefinedObj* nameList _AND_ char* str _AND_ size_t bufferSize _AND_ int maxDigits _AND_ int startingDigit)
{
	int digit, maxDigitVal;
	size_t len;

	if (ObjIsDefined(nameList, str, StrObjCmp) || IsCKeyWord(str))
	{
		for (maxDigitVal = 1; maxDigits > 0; maxDigits--)
			maxDigitVal *= 10;

		len = strlen(str);
		digit = startingDigit;
		do
		{
			str[len] = '\0';
			AppendDigit(str, bufferSize, digit++);
		} while (ObjIsDefined(nameList, str, StrObjCmp) && (digit < maxDigitVal));
	}
} /* MakeCStrUnique */

/*
 * same as MakeCStrUnique except checks against C++ keywords
 */
void MakeCxxStrUnique PARAMS((nameList, str, bufferSize, maxDigits, startingDigit), DefinedObj* nameList _AND_ char* str _AND_ size_t bufferSize _AND_ int maxDigits _AND_ int startingDigit)
{
	int digit, maxDigitVal;
	size_t len;

	if (ObjIsDefined(nameList, str, StrObjCmp) || IsCxxKeyWord(str))
	{
		for (maxDigitVal = 1; maxDigits > 0; maxDigits--)
			maxDigitVal *= 10;

		len = strlen(str);
		digit = startingDigit;
		do
		{
			str[len] = '\0';
			AppendDigit(str, bufferSize, digit++);
		} while (ObjIsDefined(nameList, str, StrObjCmp) && (digit < maxDigitVal));
	}
} /* MakeCxxStrUnique */

char* str_replace(const char* string, const char* substr, const char* replacement)
{
	char* tok = NULL;
	char* newstr = NULL;
	char* oldstr = NULL;
	size_t oldstr_len = 0;
	size_t substr_len = 0;
	size_t replacement_len = 0;

	newstr = _strdup(string);
	substr_len = strlen(substr);
	replacement_len = strlen(replacement);

	if (substr == NULL || replacement == NULL)
		return newstr;

	tok = strstr(newstr, substr);
	while (tok)
	{
		oldstr = newstr;
		oldstr_len = strlen(oldstr);
		newstr = (char*)malloc(sizeof(char) * (oldstr_len - substr_len + replacement_len + 1));

		if (newstr == NULL)
		{
			free(oldstr);
			return NULL;
		}

		memcpy(newstr, oldstr, tok - oldstr);
		memcpy(newstr + (tok - oldstr), replacement, replacement_len);
		memcpy(newstr + (tok - oldstr) + replacement_len, tok + substr_len, oldstr_len - substr_len - (tok - oldstr));
		memset(newstr + oldstr_len - substr_len + replacement_len, 0, 1);

		free(oldstr);
#if defined(_MSC_VER)
#pragma warning(disable : 6001)
#endif
		tok = strstr(newstr, substr);
#if defined(_MSC_VER)
#pragma warning(default : 6001)
#endif
	}

	return newstr;
}

char* getNakedCommentDupped(const char* szString)
{
	return str_replace(szString, "\\n", "");
}

char* MakeBaseFileName PARAMS((refName), const char* refName)
{
	char *base, *dot;
	size_t stublen;
	size_t pathLen = 0;
	char* stub;

	char szSeperator;
#ifdef _WIN32
	szSeperator = '\\';
#else
	szSeperator = '/';
#endif

	if ((base = strrchr(refName, szSeperator)) != NULL)
		base++;
	else
		base = (char*)refName;

	if ((dot = strrchr(base, '.')) != NULL)
		stublen = dot - base;
	else
		stublen = strlen(base);

	pathLen = strlen(gszOutputPath);

	size_t size = pathLen + stublen + 1;
	stub = Malloc(size);
	strcpy_s(stub, size, gszOutputPath);
	memcpy(stub + pathLen, base, stublen);
	stub[pathLen + stublen] = '\0';

	return stub;
} /* MakeBaseFileName */

const char* FileNameOnly(const char* path)
{
	size_t i = strlen(path);
	for (; i != 0; i--)
	{
		if (path[i] == '\\' || path[i] == '/')
		{
			i++;
			break;
		}
	}
	return &path[i];
}

/*
 * given a module name and a suffix, the
 * suffix is appended to the module name
 * and the whole string is put into lower case
 * and underscores are inserted in likely places
 * (ie MTSAbstractSvc.h -> mts_abstract_svc.h)
 */
char* MakeFileName PARAMS((refName, suffix), const char* refName _AND_ const char* suffix)
{
	const char* fn = FileNameOnly(refName);

	size_t baselen = strlen(fn);
	size_t sufflen = strlen(suffix);
	size_t pathLen = strlen(gszOutputPath);
	size_t size = pathLen + baselen + sufflen + 1;
	char* filename = Malloc(size);

	strcpy_s(filename, size, gszOutputPath);
	strcat_s(filename, size, fn);
	strcat_s(filename, size, suffix);

	return filename;
} /* MakeFileName */

char* MakeFileNameWithoutOutputPath PARAMS((refName, suffix), const char* refName _AND_ const char* suffix)
{
	const char* fn = FileNameOnly(refName);

	size_t baselen = strlen(fn);
	size_t sufflen = strlen(suffix);
	size_t size = baselen + sufflen + 1;
	char* filename = Malloc(size);

	strcpy_s(filename, size, fn);
	strcat_s(filename, size, suffix);

	return filename;
} /* MakeFileName */

char* MakeCHdrFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".h");
}

char* MakeCSrcFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".c");
}

char* MakeCxxHdrFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".h");
}

char* MakeCxxSrcFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".cpp");
}

char* MakeSwiftFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".swift");
}

char* MakeJSFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".js");
}

char* MakeTSFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".ts");
}

char* MakeTSEncDecFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, "_Converter.ts");
}

char* MakeJsonDocFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".json");
}

char* MakeDelphiFileName PARAMS((refName), const char* refName)
{
	char* retVal = MakeFileName(refName, ".pas");
	// BUILDSYS-151: Dash2Underscore must only be applied to filename not to path:
	char* szRefFilename = RemovePathNonConst(retVal); // rely on the current (2018-07-25) implementation of RemovePath, returning just a pointer to the last portion of retVal
	Dash2Underscore(szRefFilename, strlen(szRefFilename));
	return retVal;
}

char* MakeROSEHdrFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, "ROSE.h");
}

char* MakeROSEHdrInterfaceFileName(const char* refName)
{
	return MakeFileName(refName, "ROSEInterface.h");
}

char* MakeROSEHdrForwardDeclFileName(const char* refName)
{
	return MakeFileName(refName, "Decl.h");
}

const char* RemovePath(const char* refName)
{
	const char* szFileNameWithoutPath = strrchr(refName, '\\');
	if (szFileNameWithoutPath)
		szFileNameWithoutPath++;
	else
		szFileNameWithoutPath = refName;
	return szFileNameWithoutPath;
}

char* RemovePathNonConst(char* refName)
{
	char* szFileNameWithoutPath = strrchr(refName, '\\');
	if (szFileNameWithoutPath)
		szFileNameWithoutPath++;
	else
		szFileNameWithoutPath = refName;
	return szFileNameWithoutPath;
}

char* MakeROSESrcFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, "ROSE.cpp");
}

char* MakeROSESrcCSFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, "ROSE.cs");
}

char* MakeROSESrcJAVAFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, "ROSE.java");
}

char* MakeModuleName PARAMS((refName), const char* refName)
{
	char* szClassName = MakeFileNameWithoutOutputPath(Asn1TypeName2CTypeName(refName), "");
	Dash2Underscore(szClassName, strlen(szClassName));
	return szClassName;
}

char* MakeROSEClassName PARAMS((refName), const char* refName)
{
	return MakeFileNameWithoutOutputPath(Asn1TypeName2CTypeName(refName), "ROSE");
}

char* MakeROSESwiftInterfaceFileName(const char* refName)
{
	return MakeFileName(refName, "ROSEInterface.swift");
}

// Converts sz-Name -> sz_Name
char* FixName(const char* szName)
{
	if (!szName)
		return NULL;

	char* szReturn = _strdup(szName);
	char* szWork = szReturn;
	do
		if (*szName == '-')
			*szWork++ = '_';
		else
			*szWork++ = *szName;
	while (*szName++);
	return szReturn;
}

// szName / sz-Name
// either .szName
// or ["sz-Name"]
char* GetPropertyAccessString(const char* szName)
{
	if (!szName)
		return NULL;

	int iNeedsAdoption = 0;
	const char* szCheck = szName;
	do
		if (*szCheck == '-')
		{
			iNeedsAdoption = 1;
			break;
		}
	while (*szCheck++);

	// ["szName"] OR .szName -> we need in maximum 4 chars more for [""] + nullchar
	size_t iLen = strlen(szName) + 4 + 1;

	char* szReturn = malloc(iLen);
	if (!szReturn)
	{
		snacc_exit("Out of memory");
		return NULL;
	}

	memset(szReturn, 0x00, iLen);
	if (iNeedsAdoption)
		strcat_s(szReturn, iLen, "[\"");
	else
		strcat_s(szReturn, iLen, ".");

	strcat_s(szReturn, iLen, szName);

	if (iNeedsAdoption)
		strcat_s(szReturn, iLen, "\"]");

	return szReturn;
}

// szName / sz-Name
// either szName
// or "sz-Name"
char* GetPropertyName(const char* szName)
{
	if (!szName)
		return NULL;

	int iNeedsAdoption = 0;
	const char* szCheck = szName;
	do
		if (*szCheck == '-')
		{
			iNeedsAdoption = 1;
			break;
		}
	while (*szCheck++);

	// "sz-Name" OR szName -> we need in maximum 2 chars more for "" + nullchar
	size_t iLen = strlen(szName) + 2 + 1;

	char* szReturn = malloc(iLen);
	if (!szReturn)
	{
		snacc_exit("Out of memory");
		return NULL;
	}
	memset(szReturn, 0x00, iLen);
	if (iNeedsAdoption)
		strcat_s(szReturn, iLen, "\"");

	strcat_s(szReturn, iLen, szName);

	if (iNeedsAdoption)
		strcat_s(szReturn, iLen, "\"");

	return szReturn;
}

#if IDL
char* MakeIDLFileName PARAMS((refName), const char* refName)
{
	return MakeFileName(refName, ".idl");
}
#endif
