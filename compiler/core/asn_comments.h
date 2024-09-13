#ifndef ASN_COMMENT_H
#define ASN_COMMENT_H

#include <stdio.h>
#include "filetype.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct _asnmodulecomment
	{
		const char* szModuleName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		long long i64Added;
		long long i64Deprecated;
		const char* szDeprecated;
	} asnmodulecomment;

	typedef struct _asnoperationcomment
	{
		const char* szTypeName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		long long i64Added;
		long long i64Deprecated;
		const char* szDeprecated;
	} asnoperationcomment;

	typedef struct _asnsequencecomment
	{
		const char* szTypeName;
		const char* szCategory;
		const char* szShort;
		const char* szLong;
		int iPrivate;
		long long i64Added;
		long long i64Deprecated;
		const char* szDeprecated;
	} asnsequencecomment;

	typedef struct _asnmembercomment
	{
		const char* szShort;
		const char* szLinkedType; // Link to a related Type for example the enum used in the integer
		int iPrivate;
		long long i64Added;
		long long i64Deprecated;
		const char* szDeprecated;
	} asnmembercomment;

	// Parse for Comments
	extern int ParseFileForComments(FILE* fp, const char* szModuleName, const enum EFILETYPE type);
	extern void FilterFiles();

	// Get LogFilter Attributes (if any) call recurring until it returns 0
	extern const char* GetFirstModuleLogFileFilter(const char* szModuleName);
	extern const char* GetNextModuleLogFileFilter(const char* szModuleName);
	// Get Comments for Module (returns 1 for success)
	extern int GetModuleComment_UTF8(const char* szModuleName, asnmodulecomment* pcomment);
	extern int GetModuleComment_ASCII(const char* szModuleName, asnmodulecomment* pcomment);
	// Get Comments for Operation (returns 1 for success)
	extern int GetOperationComment_UTF8(const char* szModuleName, const char* szOpName, asnoperationcomment* pcomment);
	extern int GetOperationComment_ASCII(const char* szModuleName, const char* szOpName, asnoperationcomment* pcomment);
	// Get Comments for Sequence or other types (returns 1 for success)
	extern int GetSequenceComment_UTF8(const char* szModuleName, const char* szTypeName, asnsequencecomment* pcomment);
	extern int GetSequenceComment_ASCII(const char* szModuleName, const char* szTypeName, asnsequencecomment* pcomment);
	// Get Comments for member of sequence (returns 1 for success)
	extern int GetMemberComment_UTF8(const char* szModuleName, const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment);
	extern int GetMemberComment_ASCII(const char* szModuleName, const char* szTypeName, const char* szMemberName, asnmembercomment* pcomment);
	// Retrieve the minor Version of a module
	extern long long GetModuleMinorVersion(const char* szModuleName);
	// Retrieve the highest minor Version of all module
	extern long long GetMaxModuleMinorVersion();

#ifdef __cplusplus
}
#endif

#endif